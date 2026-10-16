/* check.c — sous-commande check : compile le code de l'élève, exécute les
 * tests et affiche un retour pédagogique. */
#include <stdio.h>
#include <string.h>

#include "app.h"
#include "compare.h"
#include "error.h"
#include "fs.h"
#include "runner.h"
#include "term.h"
#include "testspec.h"
#include "text.h"

#define PREVIEW_LINES 12 /* lignes affichées au maximum par bloc */
#define REPORT_LINES  40 /* pour un rapport de sanitizer, plus long */

typedef enum {
    V_PASS, V_WRONG_OUTPUT, V_WRONG_EXIT, V_CRASH, V_TIMEOUT, V_TOO_MUCH_OUTPUT,
    V_SANITIZER
} verdict;

static const char *verdict_label(verdict v)
{
    switch (v) {
    case V_PASS:            return "réussi";
    case V_WRONG_OUTPUT:    return "sortie incorrecte";
    case V_WRONG_EXIT:      return "code de retour incorrect";
    case V_CRASH:           return "plantage";
    case V_TIMEOUT:         return "temps dépassé";
    case V_TOO_MUCH_OUTPUT: return "sortie trop longue";
    case V_SANITIZER:       return "erreur mémoire ou comportement indéfini";
    }
    return "?";
}

/* L'ordre des tests compte : un programme tué par un délai a forcément une
 * sortie incomplète, on signale donc la cause (délai) et pas le symptôme. */
static verdict judge(const exercise *ex, const test_case *tc, const proc_result *res)
{
    if (res->timed_out)
        return V_TIMEOUT;
    if (res->truncated)
        return V_TOO_MUCH_OUTPUT;
    /* Un sanitizer arrête le programme dès l'erreur, souvent avant que la
     * sortie soit vidée : son rapport est la vraie cause de l'échec. */
    if (ex->sanitize && (strstr(sb_cstr(&res->err), "==ERROR: ") ||
                         strstr(sb_cstr(&res->err), "runtime error:")))
        return V_SANITIZER;
    if (res->signaled)
        return V_CRASH;
    if (!compare_outputs(tc->expected, sb_cstr(&res->out), ex->compare))
        return V_WRONG_OUTPUT;
    if (res->exit_code != tc->expected_exit)
        return V_WRONG_EXIT;
    return V_PASS;
}

/* Affiche un texte en retrait, tronqué à max_lines lignes. */
static void print_block(const char *title, const char *text, int max_lines)
{
    printf("    %s :\n", title);
    if (!text[0]) {
        printf("      %s(vide)%s\n", term(T_DIM), term(T_RESET));
        return;
    }
    const char *cursor = text, *line;
    size_t len;
    int n = 0;
    while (text_next_line(&cursor, &line, &len)) {
        if (n == max_lines) {
            int rest = 1;
            while (text_next_line(&cursor, &line, &len))
                rest++;
            printf("      %s… (%d ligne(s) de plus)%s\n", term(T_DIM), rest, term(T_RESET));
            break;
        }
        printf("      %s│%s %.*s\n", term(T_DIM), term(T_RESET), (int)len, line);
        n++;
    }
}

static void print_failure_details(const exercise *ex, const test_case *tc,
                                  const proc_result *res, verdict v)
{
    switch (v) {
    case V_TIMEOUT:
        printf("    Le programme a dépassé %ld ms : boucle infinie, ou attente d'une "
               "entrée qui n'arrive pas ?\n", ex->timeout_ms);
        break;
    case V_TOO_MUCH_OUTPUT:
        printf("    Le programme a écrit plus de 1 Mo : probablement une boucle "
               "infinie qui affiche.\n");
        break;
    case V_CRASH:
        printf("    Le programme s'est arrêté brutalement : %s.\n",
               proc_signal_description(res->signal));
        break;
    case V_SANITIZER:
        printf("    Le sanitizer a détecté une erreur (fuite mémoire, accès hors limites, "
               "use-after-free, dépassement d'entier...).\n"
               "    Dans le rapport ci-dessous, cherche la première ligne qui cite "
               "ton fichier (ex. « main.cpp:12 »).\n");
        break;
    case V_WRONG_EXIT:
        printf("    Code de retour %d, attendu %d (valeur renvoyée par main ou "
               "passée à exit).\n", res->exit_code, tc->expected_exit);
        break;
    default:
        break;
    }

    if (tc->hidden) {
        /* Ne pas montrer l'entrée d'un test caché : on veut un programme
         * correct en général, pas adapté à des cas particuliers. */
        printf("    %sTest caché : pense aux cas limites (valeurs extrêmes, entrée "
               "vide, négatifs, espaces...).%s\n", term(T_DIM), term(T_RESET));
        return;
    }
    if (tc->args.len > 0) {
        printf("    Arguments :");
        for (size_t i = 0; i < tc->args.len; i++)
            printf(" «%s»", tc->args.items[i]);
        printf("\n");
    }
    print_block("Entrée", tc->input, PREVIEW_LINES);
    if (v == V_WRONG_OUTPUT)
        compare_print_diff(stdout, tc->expected, sb_cstr(&res->out), ex->compare);
    if (res->err.len > 0)
        print_block("Erreurs (stderr)", sb_cstr(&res->err),
                    v == V_SANITIZER ? REPORT_LINES : PREVIEW_LINES);
}

/* Compilation. Renvoie TR_EXIT_OK si on peut passer aux tests. */
static int do_build(const runner *r, const exercise *ex, const char *workdir)
{
    strbuf diag = STRBUF_INIT;
    printf("Compilation (%s)... ", r->display_name);
    fflush(stdout); /* afficher avant que la compilation ne commence */
    int b = r->build(ex, workdir, &diag);
    int rc = TR_EXIT_OK;

    if (b == BUILD_ERROR) {
        printf("%simpossible%s\n", term(T_RED), term(T_RESET));
        fprintf(stderr, "trainer: %s\n", err_msg());
        rc = TR_EXIT_ERROR;
    } else if (b == BUILD_FAILED) {
        printf("%séchec%s\n\n", term(T_RED), term(T_RESET));
        /* Messages bruts : le problem matcher de VS Code les rend cliquables. */
        fputs(sb_cstr(&diag), stdout);
        printf("\n%sConseil : corrige d'abord la PREMIÈRE erreur, les suivantes en "
               "découlent souvent.%s\n", term(T_DIM), term(T_RESET));
        rc = TR_EXIT_FAIL;
    } else if (diag.len > 0) {
        printf("%sok, avec avertissements%s\n\n", term(T_YELLOW), term(T_RESET));
        fputs(sb_cstr(&diag), stdout);
        printf("\n%sUn avertissement signale souvent un vrai bug : en entreprise, on "
               "compile généralement avec -Werror.%s\n\n", term(T_DIM), term(T_RESET));
    } else {
        printf("%sok%s\n\n", term(T_GREEN), term(T_RESET));
    }
    sb_free(&diag);
    return rc;
}

/* Exécute tous les tests ; renvoie le nombre de tests réussis. */
static size_t run_tests(const runner *r, const exercise *ex, const char *workdir,
                        const test_suite *ts, int *internal_error)
{
    size_t passed = 0;
    for (size_t i = 0; i < ts->len; i++) {
        const test_case *tc = &ts->items[i];
        proc_result res;
        if (r->run(ex, workdir, tc, &res) != 0) {
            fprintf(stderr, "trainer: %s\n", err_msg());
            proc_result_free(&res);
            *internal_error = 1;
            return passed;
        }
        verdict v = judge(ex, tc, &res);
        if (v == V_PASS) {
            passed++;
            printf("  %s✓%s test %zu/%zu : %s %s(%ld ms)%s\n", term(T_GREEN), term(T_RESET),
                   i + 1, ts->len, tc->name, term(T_DIM), res.elapsed_ms, term(T_RESET));
        } else {
            printf("  %s✗%s test %zu/%zu : %s — %s%s%s\n", term(T_RED), term(T_RESET),
                   i + 1, ts->len, tc->name, term(T_RED), verdict_label(v), term(T_RESET));
            print_failure_details(ex, tc, &res, v);
            printf("\n");
        }
        proc_result_free(&res);
    }
    return passed;
}

static void print_correction(const exercise *ex)
{
    printf("\n%s── Correction ──%s\n\n%s\n", term(T_BOLD), term(T_RESET), ex->explanation);
    if (ex->common_mistakes)
        printf("\n%s── Erreurs fréquentes ──%s\n\n%s\n", term(T_BOLD), term(T_RESET),
               ex->common_mistakes);
}

int cmd_check(const app_ctx *app, int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage : trainer check <id>\n");
        return TR_EXIT_ERROR;
    }
    catalog cat;
    int rc = app_load_catalog(app, &cat);
    if (rc != TR_EXIT_OK)
        return rc;

    strbuf workdir = STRBUF_INIT, path = STRBUF_INIT;
    test_suite ts = TEST_SUITE_INIT;
    rc = TR_EXIT_ERROR;
    const exercise *ex = app_find(&cat, argv[1]);
    if (!ex)
        goto out;
    const runner *r = runner_for(ex->lang);
    if (!r) {
        fprintf(stderr, "trainer: le langage « %s » n'est pas encore pris en charge.\n",
                ex->lang);
        goto out;
    }
    app_workdir(app, ex, &workdir);
    if (!fs_is_dir(workdir.data)) {
        fprintf(stderr, "trainer: exercice pas encore commencé. Lance d'abord :\n"
                        "  trainer start %s\n", ex->id);
        goto out;
    }
    path_join(&path, ex->dir, "tests.txt");
    if (testspec_load(path.data, &ts) != 0) {
        fprintf(stderr, "trainer: tests de l'exercice invalides : %s\n", err_msg());
        goto out;
    }

    printf("%s▶ %s%s %s(%s)%s\n", term(T_BOLD), ex->title, term(T_RESET), term(T_DIM),
           ex->id, term(T_RESET));
    rc = do_build(r, ex, workdir.data);
    if (rc != TR_EXIT_OK)
        goto out;

    int internal_error = 0;
    size_t passed = run_tests(r, ex, workdir.data, &ts, &internal_error);
    if (internal_error) {
        rc = TR_EXIT_ERROR;
    } else if (passed == ts.len) {
        printf("\n%s✓ Tous les tests passent (%zu/%zu).%s\n", term(T_GREEN), passed,
               ts.len, term(T_RESET));
        print_correction(ex);
        rc = TR_EXIT_OK;
    } else {
        printf("%s✗ %zu/%zu tests réussis.%s Relis l'énoncé : trainer show %s\n",
               term(T_RED), passed, ts.len, term(T_RESET), ex->id);
        rc = TR_EXIT_FAIL;
    }

out:
    testspec_free(&ts);
    sb_free(&workdir);
    sb_free(&path);
    catalog_free(&cat);
    return rc;
}
