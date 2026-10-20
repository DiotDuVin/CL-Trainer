/* cmd_check.c — sous-commande check : lance la vérification (check_run, dans
 * libtrainer), enregistre la progression et présente le rapport. */
#include <stdio.h>
#include <string.h>

#include "cli.h"
#include "json.h"
#include "term.h"
#include "text.h"

#define PREVIEW_LINES 12 /* lignes affichées au maximum par bloc */
#define REPORT_LINES  40 /* pour un rapport de sanitizer, plus long */

static void print_line(const char *label, const char *line)
{
    printf("      %s : ", label);
    if (!line) {
        printf("%s(fin de la sortie)%s\n", term(T_DIM), term(T_RESET));
        return;
    }
    strbuf vis = STRBUF_INIT;
    escape_visible(&vis, line, strlen(line));
    /* Les guillemets « » montrent où la ligne commence et finit : un
     * espace en trop devient ainsi visible. */
    printf("«%s»\n", sb_cstr(&vis));
    sb_free(&vis);
}

static void print_diff(const exercise *ex, const test_case *tc, const proc_result *res)
{
    output_diff d;
    compare_first_diff(tc->expected, sb_cstr(&res->out), ex->compare, &d);
    if (d.empty_output)
        printf("    Le programme n'a rien affiché.\n");
    else if (d.same_lines)
        printf("    Les lignes sont identiques mais les fins de ligne diffèrent "
               "(retour à la ligne final manquant ou en trop, \\r\\n ?).\n");
    else {
        printf("    Première différence à la ligne %d :\n", d.line);
        print_line("attendu", d.expected_line);
        print_line("obtenu ", d.actual_line);
    }
    output_diff_free(&d);
}

static void print_failure(const exercise *ex, const test_case *tc, const test_outcome *o)
{
    const proc_result *res = &o->res;
    switch (o->result) {
    case VERDICT_TIMEOUT:
        printf("    Le programme a dépassé %ld ms : boucle infinie, ou attente d'une "
               "entrée qui n'arrive pas ?\n", ex->timeout_ms);
        break;
    case VERDICT_TOO_MUCH_OUTPUT:
        printf("    Le programme a écrit plus de 1 Mo : probablement une boucle "
               "infinie qui affiche.\n");
        break;
    case VERDICT_SANITIZER:
        printf("    Le sanitizer a détecté une erreur (fuite mémoire, accès hors limites, "
               "use-after-free, dépassement d'entier...).\n"
               "    Dans le rapport ci-dessous, cherche la première ligne qui cite "
               "ton fichier (ex. « main.cpp:12 »).\n");
        break;
    case VERDICT_CRASH:
        printf("    Le programme s'est arrêté brutalement : %s.\n",
               proc_signal_description(res->signal));
        break;
    case VERDICT_WRONG_EXIT:
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
    if (o->result == VERDICT_WRONG_OUTPUT)
        print_diff(ex, tc, res);
    if (res->err.len > 0)
        print_block("Erreurs (stderr)", sb_cstr(&res->err),
                    o->result == VERDICT_SANITIZER ? REPORT_LINES : PREVIEW_LINES);
}

static void print_build(const check_report *rep)
{
    if (rep->status == CHECK_BUILD_FAILED) {
        printf("%séchec%s\n\n", term(T_RED), term(T_RESET));
        /* Messages bruts : le problem matcher de VS Code les rend cliquables. */
        fputs(rep->build_log, stdout);
        printf("\n%sConseil : corrige d'abord la PREMIÈRE erreur, les suivantes en "
               "découlent souvent.%s\n", term(T_DIM), term(T_RESET));
    } else if (rep->build_log[0]) {
        printf("%sok, avec avertissements%s\n\n", term(T_YELLOW), term(T_RESET));
        fputs(rep->build_log, stdout);
        printf("\n%sUn avertissement signale souvent un vrai bug : en entreprise, on "
               "compile généralement avec -Werror.%s\n\n", term(T_DIM), term(T_RESET));
    } else {
        printf("%sok%s\n\n", term(T_GREEN), term(T_RESET));
    }
}

static void print_tests(const exercise *ex, const check_report *rep)
{
    for (size_t i = 0; i < rep->tests.len; i++) {
        const test_case *tc = &rep->tests.items[i];
        const test_outcome *o = &rep->outcomes[i];
        if (o->result == VERDICT_PASS) {
            printf("  %s✓%s test %zu/%zu : %s %s(%ld ms)%s\n", term(T_GREEN), term(T_RESET),
                   i + 1, rep->tests.len, tc->name, term(T_DIM), o->res.elapsed_ms,
                   term(T_RESET));
        } else {
            printf("  %s✗%s test %zu/%zu : %s — %s%s%s\n", term(T_RED), term(T_RESET),
                   i + 1, rep->tests.len, tc->name, term(T_RED), verdict_label(o->result),
                   term(T_RESET));
            print_failure(ex, tc, o);
            printf("\n");
        }
    }
}

static void print_json(const exercise *ex, const check_report *rep)
{
    static const char *const status_names[] = { "passed", "tests_failed", "build_failed" };
    strbuf out = STRBUF_INIT;
    json_writer w;
    json_init(&w, &out);
    json_begin_object(&w);
    json_key(&w, "id");        json_string(&w, ex->id);
    json_key(&w, "status");    json_string(&w, status_names[rep->status]);
    json_key(&w, "build_log"); json_string(&w, rep->build_log);
    json_key(&w, "passed");    json_int(&w, (long)rep->passed);
    json_key(&w, "total");     json_int(&w, (long)rep->tests.len);
    json_key(&w, "tests");
    json_begin_array(&w);
    for (size_t i = 0; rep->outcomes && i < rep->tests.len; i++) {
        const test_case *tc = &rep->tests.items[i];
        const test_outcome *o = &rep->outcomes[i];
        json_begin_object(&w);
        json_key(&w, "name");       json_string(&w, tc->name);
        json_key(&w, "hidden");     json_bool(&w, tc->hidden);
        json_key(&w, "verdict");    json_string(&w, verdict_label(o->result));
        json_key(&w, "passed");     json_bool(&w, o->result == VERDICT_PASS);
        json_key(&w, "elapsed_ms"); json_int(&w, o->res.elapsed_ms);
        if (!tc->hidden) { /* même règle qu'à l'écran */
            json_key(&w, "stdout"); json_string(&w, sb_cstr(&o->res.out));
            json_key(&w, "stderr"); json_string(&w, sb_cstr(&o->res.err));
        }
        json_end(&w);
    }
    json_end(&w);
    json_end(&w);
    puts(sb_cstr(&out));
    sb_free(&out);
}

int cmd_check(const trainer_config *cfg, int argc, char **argv)
{
    const char *id = NULL;
    int json = 0, bad = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--json") == 0)
            json = 1;
        else if (!id && argv[i][0] != '-')
            id = argv[i];
        else
            bad = 1;
    }
    if (!id || bad) {
        fprintf(stderr, "usage : trainer check <id> [--json]\n");
        return TR_EXIT_ERROR;
    }

    catalog cat;
    int rc = cli_load_catalog(cfg, &cat);
    if (rc != TR_EXIT_OK)
        return rc;
    strbuf workdir = STRBUF_INIT;
    check_report rep;
    memset(&rep, 0, sizeof rep);
    rc = TR_EXIT_ERROR;
    const exercise *ex = cli_find(&cat, id);
    if (!ex)
        goto out;
    if (!workspace_exists(cfg, ex)) {
        fprintf(stderr, "trainer: exercice pas encore commencé. Lance d'abord :\n"
                        "  trainer start %s\n", ex->id);
        goto out;
    }
    workspace_dir(cfg, ex, &workdir);

    if (!json) {
        printf("%s▶ %s%s %s(%s)%s\n", term(T_BOLD), ex->title, term(T_RESET), term(T_DIM),
               ex->id, term(T_RESET));
        printf("Compilation (%s)... ", lang_name(ex->lang));
        fflush(stdout); /* afficher avant que la compilation ne commence */
    }
    if (check_run(ex, workdir.data, &rep) != 0) {
        if (!json)
            printf("%simpossible%s\n", term(T_RED), term(T_RESET));
        fprintf(stderr, "trainer: %s\n", err_msg());
        goto out;
    }

    progress_event ev = rep.status == CHECK_PASSED       ? EVENT_CHECK_PASSED
                        : rep.status == CHECK_BUILD_FAILED ? EVENT_CHECK_BUILD_FAILED
                                                           : EVENT_CHECK_FAILED;
    cli_record(cfg, ex->id, ev, 0);
    rc = rep.status == CHECK_PASSED ? TR_EXIT_OK : TR_EXIT_FAIL;

    if (json) {
        print_json(ex, &rep);
        goto out;
    }
    print_build(&rep);
    if (rep.status == CHECK_BUILD_FAILED)
        goto out;
    print_tests(ex, &rep);
    if (rep.status == CHECK_PASSED) {
        printf("\n%s✓ Tous les tests passent (%zu/%zu).%s\n", term(T_GREEN), rep.passed,
               rep.tests.len, term(T_RESET));
        printf("\n%s── Correction ──%s\n\n%s\n", term(T_BOLD), term(T_RESET), ex->explanation);
        if (ex->common_mistakes)
            printf("\n%s── Erreurs fréquentes ──%s\n\n%s\n", term(T_BOLD), term(T_RESET),
                   ex->common_mistakes);
        printf("\nEnsuite : %strainer next%s\n", term(T_CYAN), term(T_RESET));
    } else {
        printf("%s✗ %zu/%zu tests réussis.%s ", term(T_RED), rep.passed, rep.tests.len,
               term(T_RESET));
        if (ex->hints.len > 0)
            printf("Besoin d'aide ? trainer hint %s\n", ex->id);
        else
            printf("Relis l'énoncé : trainer show %s\n", ex->id);
    }

out:
    check_report_free(&rep);
    sb_free(&workdir);
    catalog_free(&cat);
    return rc;
}
