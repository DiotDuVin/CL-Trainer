/* cmd_start.c — sous-commande start. */
#include <stdio.h>
#include <string.h>

#include "cli.h"
#include "term.h"

int cmd_start(const trainer_config *cfg, int argc, char **argv)
{
    const char *id = NULL;
    int force = 0, open_editor = 0, bad = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--force") == 0)
            force = 1;
        else if (strcmp(argv[i], "--open") == 0)
            open_editor = 1;
        else if (!id && argv[i][0] != '-')
            id = argv[i];
        else
            bad = 1;
    }
    if (!id || bad) {
        fprintf(stderr, "usage : trainer start <id> [--force] [--open]\n"
                        "  --force  recopie le code de départ (écrase vos fichiers)\n"
                        "  --open   ouvre le dossier dans VS Code\n");
        return TR_EXIT_ERROR;
    }

    catalog cat;
    int rc = cli_load_catalog(cfg, &cat);
    if (rc != TR_EXIT_OK)
        return rc;
    strbuf workdir = STRBUF_INIT;
    rc = TR_EXIT_ERROR;
    const exercise *ex = cli_find(&cat, id);
    if (!ex)
        goto out;

    start_result result;
    if (workspace_start(cfg, ex, force, &result) != 0) {
        fprintf(stderr, "trainer: %s\n", err_msg());
        goto out;
    }
    cli_record(cfg, ex->id, EVENT_START, 0);
    workspace_dir(cfg, ex, &workdir);
    switch (result) {
    case START_CREATED: printf("Exercice créé dans %s\n", workdir.data); break;
    case START_RESET:   printf("Code de départ recopié dans %s\n", workdir.data); break;
    case START_EXISTED: printf("Exercice déjà commencé : vos fichiers sont conservés.\n"); break;
    }
    printf("\n  Dossier  : %s%s%s\n", term(T_CYAN), workdir.data, term(T_RESET));
    printf("  Énoncé   : trainer show %s\n", ex->id);
    printf("  Vérifier : trainer check %s   (ou Ctrl+Maj+B dans VS Code)\n", ex->id);
    if (open_editor && workspace_open_editor(workdir.data) != 0)
        fprintf(stderr, "trainer: %s\n", err_msg());
    rc = TR_EXIT_OK;
out:
    sb_free(&workdir);
    catalog_free(&cat);
    return rc;
}
