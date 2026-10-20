/* progress.c — voir progress.h. */
#include "progress.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h> /* rename */
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "error.h"
#include "fs.h"
#include "strbuf.h"
#include "text.h"
#include "xalloc.h"

#define FORMAT_VERSION 1L
#define HEADER_PREFIX "# trainer-progress v"

static const char *const status_names[] = { "new", "started", "failed", "passed" };

const char *progress_status_name(exercise_status s)
{
    return status_names[s];
}

static void now_iso(char out[PROGRESS_DATE_LEN])
{
    time_t t = time(NULL);
    struct tm tm;
    gmtime_r(&t, &tm); /* _r : version sûre entre threads de gmtime */
    strftime(out, PROGRESS_DATE_LEN, "%Y-%m-%dT%H:%M:%SZ", &tm);
}

static progress_entry *find_mut(progress *p, const char *id)
{
    for (size_t i = 0; i < p->len; i++)
        if (strcmp(p->items[i].id, id) == 0)
            return &p->items[i];
    return NULL;
}

const progress_entry *progress_find(const progress *p, const char *id)
{
    return find_mut((progress *)p, id);
}

exercise_status progress_status(const progress *p, const char *id)
{
    const progress_entry *e = progress_find(p, id);
    return e ? e->status : STATUS_NEW;
}

static progress_entry *get_or_create(progress *p, const char *id)
{
    progress_entry *e = find_mut(p, id);
    if (e)
        return e;
    p->items = grow_array(p->items, &p->cap, p->len + 1, sizeof *p->items);
    e = &p->items[p->len++];
    memset(e, 0, sizeof *e);
    e->id = xstrdup(id);
    e->extra = xstrdup("");
    e->status = STATUS_NEW;
    return e;
}

/* ------------------------------------------------------------ lecture */

static int parse_int_field(const char *v, size_t n, long max, int *out)
{
    long x;
    if (parse_long(v, n, 0, max, &x) != 0)
        return -1;
    *out = (int)x;
    return 0;
}

static int parse_date_field(const char *v, size_t n, char out[PROGRESS_DATE_LEN])
{
    if (n == 0 || n >= PROGRESS_DATE_LEN)
        return -1;
    memcpy(out, v, n);
    out[n] = '\0';
    return 0;
}

/* Une ligne : « id clé=valeur clé=valeur ... » */
static int parse_line(progress *p, const char *line, size_t len)
{
    const char *tok = line, *end = line + len;
    progress_entry *e = NULL;
    strbuf extra = STRBUF_INIT;

    while (tok < end) {
        while (tok < end && (*tok == ' ' || *tok == '\t'))
            tok++;
        const char *t_end = tok;
        while (t_end < end && *t_end != ' ' && *t_end != '\t')
            t_end++;
        size_t n = (size_t)(t_end - tok);
        if (n == 0)
            break;

        if (!e) { /* premier mot : l'identifiant */
            char *id = xstrndup(tok, n);
            int dup = find_mut(p, id) != NULL;
            if (!dup)
                e = get_or_create(p, id);
            free(id);
            if (dup) {
                err_set("exercice « %.*s » en double", (int)n, tok);
                goto fail;
            }
        } else {
            const char *eq = memchr(tok, '=', n);
            if (!eq) {
                err_set("« %.*s » : « clé=valeur » attendu", (int)n, tok);
                goto fail;
            }
            const char *k = tok, *v = eq + 1;
            size_t kn = (size_t)(eq - tok), vn = n - kn - 1;
            int bad = 0;
            if (span_eq(k, kn, "status")) {
                bad = 1;
                for (int s = STATUS_NEW; s <= STATUS_PASSED; s++)
                    if (span_eq(v, vn, status_names[s])) {
                        e->status = (exercise_status)s;
                        bad = 0;
                    }
            } else if (span_eq(k, kn, "attempts")) {
                bad = parse_int_field(v, vn, INT_MAX, &e->attempts);
            } else if (span_eq(k, kn, "build_failures")) {
                bad = parse_int_field(v, vn, INT_MAX, &e->build_failures);
            } else if (span_eq(k, kn, "hints")) {
                bad = parse_int_field(v, vn, INT_MAX, &e->hints);
            } else if (span_eq(k, kn, "solution_seen")) {
                bad = parse_int_field(v, vn, 1, &e->solution_seen);
            } else if (span_eq(k, kn, "first")) {
                bad = parse_date_field(v, vn, e->first);
            } else if (span_eq(k, kn, "last")) {
                bad = parse_date_field(v, vn, e->last);
            } else if (span_eq(k, kn, "passed_at")) {
                bad = parse_date_field(v, vn, e->passed_at);
            } else { /* champ inconnu : conservé tel quel */
                if (extra.len)
                    sb_append_char(&extra, ' ');
                sb_append_n(&extra, tok, n);
            }
            if (bad) {
                err_set("valeur invalide « %.*s »", (int)n, tok);
                goto fail;
            }
        }
        tok = t_end;
    }
    if (e) {
        free(e->extra);
        e->extra = sb_release(&extra);
    }
    sb_free(&extra);
    return 0;
fail:
    sb_free(&extra);
    return -1;
}

int progress_load(const char *path, progress *p)
{
    memset(p, 0, sizeof *p);
    if (!fs_exists(path))
        return 0; /* première utilisation */

    strbuf text = STRBUF_INIT;
    if (fs_read_file(path, &text) != 0) {
        sb_free(&text);
        return -1;
    }
    const char *cursor = sb_cstr(&text), *line;
    size_t len;
    int lineno = 0, rc = 0;
    while (rc == 0 && text_next_line(&cursor, &line, &len)) {
        lineno++;
        span_trim(&line, &len);
        if (len == 0)
            continue;
        if (line[0] == '#') {
            long version;
            if (span_starts_with(line, len, HEADER_PREFIX) &&
                parse_long(line + strlen(HEADER_PREFIX), len - strlen(HEADER_PREFIX),
                           1, 1000000, &version) == 0 &&
                version > FORMAT_VERSION) {
                err_set("%s : créé par une version plus récente de trainer "
                        "(format v%ld) : mettez trainer à jour", path, version);
                rc = -1;
            }
            continue; /* autres commentaires ignorés */
        }
        if (parse_line(p, line, len) != 0) {
            err_set("%s:%d: %s", path, lineno, err_msg());
            rc = -1;
        }
    }
    sb_free(&text);
    if (rc != 0)
        progress_free(p);
    return rc;
}

/* ------------------------------------------------------------ écriture */

static int write_all(int fd, const char *data, size_t len)
{
    while (len > 0) {
        ssize_t w = write(fd, data, len);
        if (w < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        data += w;
        len -= (size_t)w;
    }
    return 0;
}

/* Dossier parent de path (« . » si aucun '/'). */
static void parent_dir(const char *path, strbuf *out)
{
    const char *slash = strrchr(path, '/');
    sb_clear(out);
    if (!slash)
        sb_append(out, ".");
    else if (slash == path)
        sb_append(out, "/");
    else
        sb_append_n(out, path, (size_t)(slash - path));
}

int progress_save(const char *path, const progress *p)
{
    strbuf dir = STRBUF_INIT, tmp = STRBUF_INIT, content = STRBUF_INIT;
    int rc = -1, fd = -1;

    parent_dir(path, &dir);
    if (fs_mkdir_p(dir.data) != 0)
        goto out;

    sb_appendf(&content, HEADER_PREFIX "%ld\n", FORMAT_VERSION);
    for (size_t i = 0; i < p->len; i++) {
        const progress_entry *e = &p->items[i];
        sb_appendf(&content, "%s status=%s attempts=%d build_failures=%d hints=%d "
                             "solution_seen=%d",
                   e->id, status_names[e->status], e->attempts, e->build_failures,
                   e->hints, e->solution_seen);
        if (e->first[0])
            sb_appendf(&content, " first=%s", e->first);
        if (e->last[0])
            sb_appendf(&content, " last=%s", e->last);
        if (e->passed_at[0])
            sb_appendf(&content, " passed_at=%s", e->passed_at);
        if (e->extra && e->extra[0])
            sb_appendf(&content, " %s", e->extra);
        sb_append_char(&content, '\n');
    }

    /* 1. Écrire le contenu complet dans un fichier temporaire du MÊME
     *    dossier (rename n'est atomique qu'au sein d'un même système de
     *    fichiers). */
    sb_appendf(&tmp, "%s.tmp", path);
    fd = open(tmp.data, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        err_set_errno("impossible de créer %s", tmp.data);
        goto out;
    }
    /* 2. fsync : forcer l'écriture sur le disque. Sans lui, les données
     *    peuvent rester en mémoire (cache du noyau) et être perdues en cas
     *    de coupure, alors que le renommage, lui, aurait été enregistré. */
    if (write_all(fd, content.data, content.len) != 0 || fsync(fd) != 0) {
        err_set_errno("écriture de %s", tmp.data);
        goto out;
    }
    if (close(fd) != 0) {
        fd = -1;
        err_set_errno("écriture de %s", tmp.data);
        goto out;
    }
    fd = -1;
    /* 3. Remplacer l'ancien fichier d'un seul coup. */
    if (rename(tmp.data, path) != 0) {
        err_set_errno("impossible de remplacer %s", path);
        goto out;
    }
    /* 4. fsync du dossier : rend le renommage lui-même durable. */
    int dfd = open(dir.data, O_RDONLY);
    if (dfd >= 0) {
        fsync(dfd);
        close(dfd);
    }
    rc = 0;

out:
    if (fd >= 0)
        close(fd);
    if (rc != 0 && tmp.data)
        unlink(tmp.data);
    sb_free(&dir);
    sb_free(&tmp);
    sb_free(&content);
    return rc;
}

void progress_free(progress *p)
{
    for (size_t i = 0; i < p->len; i++) {
        free(p->items[i].id);
        free(p->items[i].extra);
    }
    free(p->items);
    memset(p, 0, sizeof *p);
}

/* ------------------------------------------------------------ événements */

void progress_apply(progress *p, const char *id, progress_event ev, int arg,
                    const char *now)
{
    char stamp[PROGRESS_DATE_LEN];
    if (now) {
        size_t n = strlen(now);
        if (n >= sizeof stamp)
            n = sizeof stamp - 1;
        memcpy(stamp, now, n);
        stamp[n] = '\0';
    } else {
        now_iso(stamp);
    }

    progress_entry *e = get_or_create(p, id);
    if (!e->first[0])
        memcpy(e->first, stamp, sizeof stamp);
    memcpy(e->last, stamp, sizeof stamp);

    switch (ev) {
    case EVENT_START:
        if (e->status == STATUS_NEW)
            e->status = STATUS_STARTED;
        break;
    case EVENT_CHECK_BUILD_FAILED:
    case EVENT_CHECK_FAILED:
        e->attempts++;
        if (ev == EVENT_CHECK_BUILD_FAILED)
            e->build_failures++;
        /* Un exercice réussi le reste : un échec ultérieur (en le
         * retravaillant) n'efface pas la réussite. */
        if (e->status != STATUS_PASSED)
            e->status = STATUS_FAILED;
        break;
    case EVENT_CHECK_PASSED:
        e->attempts++;
        e->status = STATUS_PASSED;
        if (!e->passed_at[0])
            memcpy(e->passed_at, stamp, sizeof stamp);
        break;
    case EVENT_HINT:
        if (arg > e->hints)
            e->hints = arg;
        break;
    case EVENT_SOLUTION:
        e->solution_seen = 1;
        break;
    case EVENT_LESSON_READ:
        e->status = STATUS_PASSED;
        if (!e->passed_at[0])
            memcpy(e->passed_at, stamp, sizeof stamp);
        break;
    }
}

int progress_record(const char *path, const char *id, progress_event ev, int arg)
{
    strbuf dir = STRBUF_INIT, lock_path = STRBUF_INIT;
    progress p = PROGRESS_INIT;
    int rc = -1, fd = -1;

    parent_dir(path, &dir);
    if (fs_mkdir_p(dir.data) != 0)
        goto out;

    /* Verrou consultatif POSIX (fcntl) sur un fichier à part : on ne
     * verrouille pas progress.txt lui-même, puisqu'il est remplacé par
     * rename(). Le verrou est libéré automatiquement à la fermeture, même
     * si le programme plante. Attention : ces verrous sont par PROCESSUS ;
     * au sein d'un même programme, les appels doivent rester séquentiels. */
    sb_appendf(&lock_path, "%s.lock", path);
    fd = open(lock_path.data, O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
        err_set_errno("impossible d'ouvrir %s", lock_path.data);
        goto out;
    }
    struct flock fl;
    memset(&fl, 0, sizeof fl);
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET; /* l_start = l_len = 0 : tout le fichier */
    while (fcntl(fd, F_SETLKW, &fl) != 0) {
        if (errno != EINTR) {
            err_set_errno("verrouillage de %s", lock_path.data);
            goto out;
        }
    }

    if (progress_load(path, &p) != 0)
        goto out;
    progress_apply(&p, id, ev, arg, NULL);
    rc = progress_save(path, &p);

out:
    if (fd >= 0)
        close(fd); /* libère le verrou */
    progress_free(&p);
    sb_free(&dir);
    sb_free(&lock_path);
    return rc;
}
