/* main.c — point d'entrée : options globales, configuration, distribution
 * vers les sous-commandes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "term.h"
#include "xalloc.h"

#ifndef TRAINER_VERSION
#define TRAINER_VERSION "dev"
#endif

typedef struct {
    const char *name;
    int (*fn)(const trainer_config *, int, char **);
    const char *summary;
} command;

static const command commands[] = {
    { "list",     cmd_list,     "liste les exercices et leur état  [--lang L] [--topic T] [--json]" },
    { "show",     cmd_show,     "affiche l'énoncé                  <id>" },
    { "start",    cmd_start,    "prépare l'exercice                <id> [--force] [--open]" },
    { "check",    cmd_check,    "compile et teste                  <id> [--json]" },
    { "hint",     cmd_hint,     "révèle l'indice suivant           <id>" },
    { "solution", cmd_solution, "solution et explication           <id> [--yes]" },
    { "next",     cmd_next,     "exercice recommandé               [--lang L]" },
    { "stats",    cmd_stats,    "progression et points faibles" },
    { "validate", cmd_validate, "contrôle les exercices            [<id>...]" },
    { "doctor",   cmd_doctor,   "vérifie les outils installés" },
};

static void usage(FILE *f)
{
    fprintf(f, "trainer %s — entraînement C++ / Git / C# / Rust\n\n"
               "usage : trainer [options] <commande> [arguments]\n\nCommandes :\n",
            TRAINER_VERSION);
    for (size_t i = 0; i < sizeof commands / sizeof commands[0]; i++)
        fprintf(f, "  %-9s %s\n", commands[i].name, commands[i].summary);
    fprintf(f, "\nOptions :\n"
               "  --exercises DIR   dossier des exercices   (ou $TRAINER_EXERCISES)\n"
               "  --workspace DIR   dossier de votre code   (ou $TRAINER_WORKSPACE,\n"
               "                                            défaut ~/trainer-workspace)\n"
               "  --no-color        désactive les couleurs  (ou $NO_COLOR)\n"
               "  -h, --help        cette aide\n"
               "  -V, --version     version\n\n"
               "Pour commencer : trainer next\n");
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

    trainer_config cfg;
    if (config_resolve(opt_exercises, opt_workspace, argv[0], &cfg) != 0) {
        fprintf(stderr, "trainer: %s\n", err_msg());
        config_free(&cfg);
        free(rest);
        return TR_EXIT_ERROR;
    }

    int rc = -1;
    for (size_t i = 0; i < sizeof commands / sizeof commands[0]; i++)
        if (strcmp(rest[0], commands[i].name) == 0)
            rc = commands[i].fn(&cfg, nrest, rest);
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

    config_free(&cfg);
    free(rest);
    return rc;
}
