/* main.c — point d'entrée : options globales, résolution des dossiers,
 * distribution vers les sous-commandes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "app.h"
#include "error.h"
#include "fs.h"
#include "strbuf.h"
#include "term.h"
#include "xalloc.h"

#ifndef TRAINER_VERSION
#define TRAINER_VERSION "dev"
#endif
#ifndef TRAINER_DATADIR /* fourni par le Makefile : $(PREFIX)/share/trainer */
#define TRAINER_DATADIR "/usr/local/share/trainer"
#endif

typedef struct {
    const char *name;
    int (*fn)(const app_ctx *, int, char **);
    const char *summary;
} command;

static const command commands[] = {
    { "list",  cmd_list,  "liste les exercices      [--lang LANG] [--topic THÈME]" },
    { "show",  cmd_show,  "affiche l'énoncé         <id>" },
    { "start", cmd_start, "prépare l'exercice       <id> [--force] [--open]" },
    { "check", cmd_check, "compile et teste         <id>" },
};

static void usage(FILE *f)
{
    fprintf(f, "trainer %s — entraînement C++ / Git / C# / Rust en ligne de commande\n\n"
               "usage : trainer [options] <commande> [arguments]\n\nCommandes :\n",
            TRAINER_VERSION);
    for (size_t i = 0; i < sizeof commands / sizeof commands[0]; i++)
        fprintf(f, "  %-7s %s\n", commands[i].name, commands[i].summary);
    fprintf(f, "\nOptions :\n"
               "  --exercises DIR   dossier des exercices   (ou $TRAINER_EXERCISES)\n"
               "  --workspace DIR   dossier de votre code   (ou $TRAINER_WORKSPACE,\n"
               "                                            défaut ~/trainer-workspace)\n"
               "  --no-color        désactive les couleurs  (ou $NO_COLOR)\n"
               "  -h, --help        cette aide\n"
               "  -V, --version     version\n\n"
               "Exemple : trainer list && trainer start cpp.basics.hello\n");
}

/* Chemin absolu de l'exécutable : /proc/self/exe sous Linux, sinon argv[0]. */
static char *find_self(const char *argv0)
{
    char buf[4096];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof buf - 1);
    if (n > 0) {
        buf[n] = '\0';
        return xstrdup(buf);
    }
    strbuf sb = STRBUF_INIT;
    if (strchr(argv0, '/') && fs_absolute(argv0, &sb) == 0)
        return sb_release(&sb);
    sb_free(&sb);
    return xstrdup("trainer"); /* trouvé via le PATH */
}

/* Dossier d'exercices, par ordre de priorité :
 *   1. --exercises   2. $TRAINER_EXERCISES   3. <dossier de l'exécutable>/exercises
 *   (cas du développement : ./trainer dans le dépôt)
 *   4. dossier d'installation   5. ./exercises */
static char *resolve_exercises(const char *opt, const char *self)
{
    const char *env = getenv("TRAINER_EXERCISES");
    const char *explicit_dir = opt ? opt : (env && *env ? env : NULL);
    strbuf sb = STRBUF_INIT;

    if (explicit_dir) {
        if (fs_absolute(explicit_dir, &sb) != 0)
            die("%s", err_msg());
        return sb_release(&sb); /* inexistant : le catalogue le signalera */
    }

    const char *slash = strrchr(self, '/');
    if (slash) {
        sb_append_n(&sb, self, (size_t)(slash - self));
        sb_append(&sb, "/exercises");
        if (fs_is_dir(sb.data))
            return sb_release(&sb);
    }
    sb_clear(&sb);
    sb_append(&sb, TRAINER_DATADIR "/exercises");
    if (fs_is_dir(sb.data))
        return sb_release(&sb);
    if (fs_absolute("exercises", &sb) != 0)
        die("%s", err_msg());
    return sb_release(&sb);
}

static char *resolve_workspace(const char *opt)
{
    const char *env = getenv("TRAINER_WORKSPACE");
    const char *dir = opt ? opt : (env && *env ? env : NULL);
    strbuf sb = STRBUF_INIT;
    if (dir) {
        if (fs_absolute(dir, &sb) != 0)
            die("%s", err_msg());
        return sb_release(&sb);
    }
    const char *home = getenv("HOME");
    if (!home || !*home)
        die("variable HOME non définie : utilisez --workspace DIR");
    path_join(&sb, home, "trainer-workspace");
    return sb_release(&sb);
}

/* Reconnaît "--opt valeur" et "--opt=valeur". Renvoie la valeur ou NULL. */
static const char *option_value(int argc, char **argv, int *i, const char *name)
{
    size_t n = strlen(name);
    if (strncmp(argv[*i], name, n) != 0)
        return NULL;
    if (argv[*i][n] == '=')
        return argv[*i] + n + 1;
    if (argv[*i][n] == '\0') {
        if (*i + 1 >= argc)
            die("l'option %s attend une valeur", name);
        return argv[++*i];
    }
    return NULL;
}

int main(int argc, char **argv)
{
    const char *opt_exercises = NULL, *opt_workspace = NULL;
    int color = -1;
    /* Arguments restants (commande + ses arguments). argc + 1 places : la
     * convention argv[argc] == NULL est conservée. */
    char **rest = xcalloc((size_t)argc + 1, sizeof *rest);
    int nrest = 0;

    for (int i = 1; i < argc; i++) {
        const char *v;
        if (strcmp(argv[i], "--") == 0) {
            while (++i < argc)
                rest[nrest++] = argv[i];
            break;
        } else if ((v = option_value(argc, argv, &i, "--exercises"))) {
            opt_exercises = v;
        } else if ((v = option_value(argc, argv, &i, "--workspace"))) {
            opt_workspace = v;
        } else if (strcmp(argv[i], "--no-color") == 0) {
            color = 0;
        } else if (strcmp(argv[i], "--color") == 0) {
            color = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(stdout);
            free(rest);
            return TR_EXIT_OK;
        } else if (strcmp(argv[i], "-V") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("trainer %s\n", TRAINER_VERSION);
            free(rest);
            return TR_EXIT_OK;
        } else {
            rest[nrest++] = argv[i];
        }
    }

    term_init(color);
    if (nrest == 0) {
        usage(stderr);
        free(rest);
        return TR_EXIT_ERROR;
    }

    app_ctx app;
    app.self_path = find_self(argv[0]);
    app.exercises_dir = resolve_exercises(opt_exercises, app.self_path);
    app.workspace_dir = resolve_workspace(opt_workspace);

    int rc = -1;
    for (size_t i = 0; i < sizeof commands / sizeof commands[0]; i++)
        if (strcmp(rest[0], commands[i].name) == 0)
            rc = commands[i].fn(&app, nrest, rest);
    if (rc == -1) {
        if (strcmp(rest[0], "help") == 0) {
            usage(stdout);
            rc = TR_EXIT_OK;
        } else {
            fprintf(stderr, "trainer: commande inconnue « %s »\n\n", rest[0]);
            usage(stderr);
            rc = TR_EXIT_ERROR;
        }
    }

    free(app.self_path);
    free(app.exercises_dir);
    free(app.workspace_dir);
    free(rest);
    return rc;
}
