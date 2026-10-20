/* cmd_admin.c — sous-commandes de maintenance : validate et doctor. */
#include <stdio.h>
#include <string.h>

#include "cli.h"
#include "term.h"

/* ---------------------------------------------------------------- validate */

int cmd_validate(const trainer_config *cfg, int argc, char **argv)
{
    catalog cat;
    int rc = catalog_load(cfg->exercises_dir, &cat);
    if (rc != 0) {
        fprintf(stderr, "trainer: %s\n", err_msg());
        return TR_EXIT_ERROR;
    }

    size_t checked = 0, rejected = 0;
    /* Les erreurs du catalogue (exercice illisible, id en double, prérequis
     * inconnu, cycle) font échouer la validation. */
    if (argc == 1 && cat.errors.len > 0) {
        printf("%s✗ catalogue%s\n", term(T_RED), term(T_RESET));
        for (size_t i = 0; i < cat.errors.len; i++)
            printf("    %s\n", cat.errors.items[i]);
        rejected++;
    }

    rc = TR_EXIT_OK;
    /* Les cours d'abord (leurs exemples), puis les exercices. */
    const size_t total = cat.nlessons + cat.len;
    for (size_t i = 0; i < total && rc == TR_EXIT_OK; i++) {
        const lesson *l = i < cat.nlessons ? &cat.lessons[i] : NULL;
        const exercise *ex = l ? NULL : &cat.items[i - cat.nlessons];
        const char *id = l ? l->id : ex->id;
        if (argc > 1) { /* ids donnés en argument : seulement ceux-là */
            int wanted = 0;
            for (int a = 1; a < argc; a++)
                wanted |= strcmp(argv[a], id) == 0;
            if (!wanted)
                continue;
        }
        printf("  %s%s ... ", l ? "📖 " : "", id);
        fflush(stdout); /* la compilation prend un moment */
        strvec problems = STRVEC_INIT;
        int vrc = l ? validate_lesson(l, &problems) : validate_exercise(ex, &problems);
        if (vrc != 0) {
            printf("%simpossible%s\n", term(T_RED), term(T_RESET));
            fprintf(stderr, "trainer: %s\n", err_msg());
            rc = TR_EXIT_ERROR;
        } else if (problems.len == 0) {
            printf("%sok%s\n", term(T_GREEN), term(T_RESET));
        } else {
            printf("%sREJETÉ%s\n", term(T_RED), term(T_RESET));
            for (size_t k = 0; k < problems.len; k++)
                printf("      - %s\n", problems.items[k]);
            rejected++;
        }
        checked++;
        sv_free(&problems);
    }
    if (argc > 1 && checked != (size_t)(argc - 1)) {
        fprintf(stderr, "trainer: cours ou exercice(s) introuvable(s) parmi les arguments\n");
        rc = TR_EXIT_ERROR;
    }
    if (rc == TR_EXIT_OK) {
        printf("\n%zu cours et exercice(s) vérifié(s), %zu rejeté(s).\n", checked, rejected);
        rc = rejected ? TR_EXIT_FAIL : TR_EXIT_OK;
    }
    catalog_free(&cat);
    return rc;
}

/* ---------------------------------------------------------------- doctor */

int cmd_doctor(const trainer_config *cfg, int argc, char **argv)
{
    (void)argv;
    if (argc != 1) {
        fprintf(stderr, "usage : trainer doctor\n");
        return TR_EXIT_ERROR;
    }
    printf("%sDossiers%s\n", term(T_BOLD), term(T_RESET));
    printf("  exercices   : %s\n", cfg->exercises_dir);
    printf("  vos fichiers: %s\n", cfg->workspace_dir);
    printf("  progression : %s\n\n", cfg->progress_path);

    doctor_report rep;
    doctor_run(&rep);
    printf("%sOutils%s %s(système : %s)%s\n", term(T_BOLD), term(T_RESET), term(T_DIM),
           rep.distro[0] ? rep.distro : "inconnu", term(T_RESET));
    int missing_essential = 0;
    for (size_t i = 0; i < rep.len; i++) {
        const tool_status *t = &rep.tools[i];
        if (t->found) {
            printf("  %s✓%s %-9s %s%s%s\n", term(T_GREEN), term(T_RESET), t->command,
                   term(T_DIM), t->version ? t->version : "", term(T_RESET));
        } else {
            missing_essential |= t->essential;
            printf("  %s✗%s %-9s %smanquant%s — %s\n", t->essential ? term(T_RED) : term(T_YELLOW),
                   term(T_RESET), t->command, t->essential ? term(T_RED) : term(T_YELLOW),
                   term(T_RESET), t->purpose);
            printf("              → %s\n", t->install_hint);
        }
    }
    printf("\n%s\n", missing_essential
                         ? "Des outils indispensables manquent : installez-les ci-dessus."
                         : "Tout est prêt pour les exercices C++.");
    doctor_free(&rep);
    return missing_essential ? TR_EXIT_FAIL : TR_EXIT_OK;
}
