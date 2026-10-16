/* process.c — exécution de processus avec timeout (voir process.h).
 *
 * Schéma :
 *
 *   père (trainer)                      fils
 *   ───────────────                     ────
 *   pipe() x4
 *   fork() ───────────────────────────► setpgid(0,0)  (son propre groupe)
 *                                       dup2 -> stdin/stdout/stderr
 *                                       chdir, setrlimit
 *   read(tube_exec) ... bloque          execvp(...)
 *     EOF        => exec réussi         (le tube_exec est CLOEXEC : exec le ferme)
 *     un entier  => errno de l'échec    (sinon le fils écrit errno dans le tube)
 *   boucle poll() : écrit stdin, lit stdout/stderr, surveille le délai
 *   délai dépassé => kill(-pid, SIGKILL) (tout le groupe, petits-enfants compris)
 *   waitpid()      => code de retour ou signal
 */
#include "process.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "error.h"

/* Horloge monotone : contrairement à l'heure système, elle ne recule jamais
 * (changement d'heure, synchronisation NTP...). C'est la bonne horloge pour
 * mesurer une durée. */
static long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long)ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

static void close_fd(int *fd)
{
    if (*fd >= 0) {
        close(*fd);
        *fd = -1;
    }
}

static void set_nonblock(int fd)
{
    int fl = fcntl(fd, F_GETFL);
    if (fl >= 0)
        fcntl(fd, F_SETFL, fl | O_NONBLOCK);
}

/* FD_CLOEXEC : le descripteur sera fermé automatiquement lors d'un exec.
 * Sans cela, le programme de l'élève hériterait de nos tubes. */
static void set_cloexec(int fd)
{
    int fl = fcntl(fd, F_GETFD);
    if (fl >= 0)
        fcntl(fd, F_SETFD, fl | FD_CLOEXEC);
}

/* Dans le fils, après fork : transmet errno au père puis quitte.
 * _exit et pas exit : exit viderait les tampons stdio hérités du père
 * (texte affiché deux fois) et lancerait ses fonctions atexit. */
ATTR_NORETURN static void child_fail(int fd, int code)
{
    ssize_t r = write(fd, &code, sizeof code);
    (void)r;
    _exit(127);
}

/* Lit ce qui est disponible sur *fd et l'ajoute à sb. Ferme *fd en fin de
 * flux. Renvoie 1 si la taille dépasse max (0 = pas de limite). Le nombre de
 * lectures est borné pour revenir régulièrement vérifier le délai même si le
 * programme écrit sans arrêt. */
static int drain(int *fd, strbuf *sb, size_t max)
{
    char buf[65536];
    for (int i = 0; i < 16; i++) {
        ssize_t n = read(*fd, buf, sizeof buf);
        if (n > 0) {
            sb_append_n(sb, buf, (size_t)n);
            if (max && sb->len > max) {
                sb_truncate(sb, max);
                return 1;
            }
            continue;
        }
        if (n < 0 && errno == EINTR)
            continue;
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            return 0;
        close_fd(fd); /* n == 0 : fin de flux, ou erreur de lecture */
        return 0;
    }
    return 0;
}

static void kill_group(pid_t pid)
{
    /* pid négatif = tout le groupe de processus. Le fils a créé son propre
     * groupe : on tue aussi ce qu'il aurait lui-même lancé. */
    kill(-pid, SIGKILL);
    kill(pid, SIGKILL);
}

int proc_run(const char *const argv[], const proc_opts *opts, proc_result *res,
             int *exec_errno)
{
    memset(res, 0, sizeof *res);
    if (exec_errno)
        *exec_errno = 0;

    int p_in[2] = { -1, -1 }, p_out[2] = { -1, -1 };
    int p_err[2] = { -1, -1 }, p_exec[2] = { -1, -1 };
    int rc = -1;

    /* Si le programme se termine sans lire tout stdin, écrire dans le tube
     * enverrait SIGPIPE au père, qui serait tué. On ignore donc SIGPIPE le
     * temps de l'exécution (write renverra EPIPE à la place). */
    struct sigaction ign, old_pipe;
    memset(&ign, 0, sizeof ign);
    ign.sa_handler = SIG_IGN;
    sigemptyset(&ign.sa_mask);
    sigaction(SIGPIPE, &ign, &old_pipe);

    if (pipe(p_in) != 0 || pipe(p_out) != 0 || pipe(p_err) != 0 || pipe(p_exec) != 0) {
        err_set_errno("création des tubes");
        goto done;
    }
    int *all[] = { &p_in[0], &p_in[1], &p_out[0], &p_out[1],
                   &p_err[0], &p_err[1], &p_exec[0], &p_exec[1] };
    for (size_t i = 0; i < sizeof all / sizeof all[0]; i++)
        set_cloexec(*all[i]); /* dup2 vers 0/1/2 retire ce drapeau : OK */

    long start = now_ms();
    fflush(stdout); /* sinon le texte en attente serait dupliqué dans le fils */
    fflush(stderr);
    pid_t pid = fork();
    if (pid < 0) {
        err_set_errno("fork");
        goto done;
    }

    if (pid == 0) { /* ===== fils ===== */
        setpgid(0, 0);
        /* Un signal ignoré le reste après exec : on rétablit le comportement
         * normal pour ne pas modifier le programme de l'élève. */
        signal(SIGPIPE, SIG_DFL);
        if (dup2(p_in[0], STDIN_FILENO) < 0 || dup2(p_out[1], STDOUT_FILENO) < 0 ||
            dup2(p_err[1], STDERR_FILENO) < 0)
            child_fail(p_exec[1], errno);
        if (opts->cwd && chdir(opts->cwd) != 0)
            child_fail(p_exec[1], errno);
        if (opts->mem_limit_mb > 0) {
            struct rlimit rl;
            rl.rlim_cur = rl.rlim_max = (rlim_t)opts->mem_limit_mb * 1024 * 1024;
            setrlimit(RLIMIT_AS, &rl);
        }
        /* Le prototype historique d'execvp prend char *const[] : le cast est
         * sans danger, execvp ne modifie pas les chaînes. */
        execvp(argv[0], (char *const *)argv);
        child_fail(p_exec[1], errno);
    }

    /* ===== père ===== */
    setpgid(pid, pid); /* fait aussi dans le fils : évite une course */
    close_fd(&p_in[0]);
    close_fd(&p_out[1]);
    close_fd(&p_err[1]);
    close_fd(&p_exec[1]);

    int child_errno = 0;
    ssize_t r;
    do
        r = read(p_exec[0], &child_errno, sizeof child_errno);
    while (r < 0 && errno == EINTR);
    close_fd(&p_exec[0]);
    if (r == (ssize_t)sizeof child_errno) { /* exec a échoué */
        while (waitpid(pid, NULL, 0) < 0 && errno == EINTR)
            ;
        errno = child_errno;
        if (opts->cwd)
            err_set_errno("impossible de lancer « %s » dans %s", argv[0], opts->cwd);
        else
            err_set_errno("impossible de lancer « %s »", argv[0]);
        if (exec_errno)
            *exec_errno = child_errno;
        goto done;
    }

    size_t in_len = opts->input ? opts->input_len : 0, in_off = 0;
    if (in_len == 0)
        close_fd(&p_in[1]); /* EOF immédiat sur le stdin du fils */
    else
        set_nonblock(p_in[1]);
    set_nonblock(p_out[0]);
    set_nonblock(p_err[0]);

    long deadline = opts->timeout_ms > 0 ? start + opts->timeout_ms : -1;
    int failure = 0;

    while (p_out[0] >= 0 || p_err[0] >= 0) {
        int wait_ms = -1;
        if (deadline >= 0) {
            long left = deadline - now_ms();
            if (left <= 0) {
                res->timed_out = 1;
                break;
            }
            wait_ms = (int)left;
        }

        struct pollfd pfd[3];
        int n = 0, i_out = -1, i_err = -1, i_in = -1;
        if (p_out[0] >= 0) { i_out = n; pfd[n].fd = p_out[0]; pfd[n++].events = POLLIN; }
        if (p_err[0] >= 0) { i_err = n; pfd[n].fd = p_err[0]; pfd[n++].events = POLLIN; }
        if (p_in[1] >= 0)  { i_in = n;  pfd[n].fd = p_in[1];  pfd[n++].events = POLLOUT; }
        for (int i = 0; i < n; i++)
            pfd[i].revents = 0;

        if (poll(pfd, (nfds_t)n, wait_ms) < 0) {
            if (errno == EINTR)
                continue;
            err_set_errno("poll");
            failure = 1;
            break;
        }
        if (i_out >= 0 && pfd[i_out].revents && drain(&p_out[0], &res->out, opts->max_output)) {
            res->truncated = 1;
            break;
        }
        if (i_err >= 0 && pfd[i_err].revents && drain(&p_err[0], &res->err, opts->max_output)) {
            res->truncated = 1;
            break;
        }
        if (i_in >= 0 && pfd[i_in].revents) {
            ssize_t w = write(p_in[1], opts->input + in_off, in_len - in_off);
            if (w > 0) {
                in_off += (size_t)w;
                if (in_off == in_len)
                    close_fd(&p_in[1]); /* tout envoyé : le fils verra EOF */
            } else if (w < 0 && errno != EAGAIN && errno != EINTR) {
                close_fd(&p_in[1]); /* EPIPE : le programme ne lit plus stdin */
            }
        }
    }

    int killed = 0;
    if (res->timed_out || res->truncated || failure) {
        kill_group(pid);
        killed = 1;
    }
    close_fd(&p_in[1]);
    close_fd(&p_out[0]);
    close_fd(&p_err[0]);

    /* Le fils peut avoir fermé stdout et continuer à tourner : on attend sa
     * fin sans dépasser le délai. */
    int status = 0;
    for (;;) {
        pid_t w = waitpid(pid, &status, (deadline >= 0 && !killed) ? WNOHANG : 0);
        if (w == pid)
            break;
        if (w < 0) {
            if (errno == EINTR)
                continue;
            err_set_errno("waitpid");
            failure = 1;
            break;
        }
        if (now_ms() >= deadline) {
            res->timed_out = 1;
            kill_group(pid);
            killed = 1;
            continue;
        }
        struct timespec pause = { 0, 2 * 1000000L }; /* 2 ms */
        nanosleep(&pause, NULL);
    }
    /* Nettoie d'éventuels petits-enfants encore vivants dans le groupe. */
    kill(-pid, SIGKILL);

    res->elapsed_ms = now_ms() - start;
    if (!failure) {
        if (WIFEXITED(status)) {
            res->exited = 1;
            res->exit_code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            res->signaled = 1;
            res->signal = WTERMSIG(status);
        }
        rc = 0;
    }

done:
    close_fd(&p_in[0]);
    close_fd(&p_in[1]);
    close_fd(&p_out[0]);
    close_fd(&p_out[1]);
    close_fd(&p_err[0]);
    close_fd(&p_err[1]);
    close_fd(&p_exec[0]);
    close_fd(&p_exec[1]);
    sigaction(SIGPIPE, &old_pipe, NULL);
    return rc;
}

void proc_result_free(proc_result *res)
{
    sb_free(&res->out);
    sb_free(&res->err);
}

const char *proc_signal_description(int sig)
{
    switch (sig) {
    case SIGSEGV:
        return "erreur de segmentation (accès mémoire invalide : pointeur nul, "
               "dépassement de tableau, pointeur libéré...)";
    case SIGABRT:
        return "arrêt anormal (abort : assertion échouée, exception non "
               "rattrapée, double free détecté...)";
    case SIGFPE:
        return "erreur arithmétique (division entière par zéro...)";
    case SIGBUS:
        return "erreur de bus (accès mémoire mal aligné ou hors fichier)";
    case SIGILL:
        return "instruction illégale (par exemple : fonction non-void qui atteint sa fin sans return)";
    case SIGKILL:
        return "tué (SIGKILL : délai ou mémoire dépassés)";
    case SIGTERM:
        return "terminé (SIGTERM)";
    default:
        return "signal inattendu";
    }
}
