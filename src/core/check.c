/* check.c — voir check.h. */
#include "check.h"

#include <stdlib.h>
#include <string.h>

#include "compare.h"
#include "error.h"
#include "fs.h"
#include "runner.h"
#include "strbuf.h"
#include "xalloc.h"

const char *verdict_label(verdict v)
{
    switch (v) {
    case VERDICT_PASS:            return "réussi";
    case VERDICT_TIMEOUT:         return "temps dépassé";
    case VERDICT_TOO_MUCH_OUTPUT: return "sortie trop longue";
    case VERDICT_SANITIZER:       return "erreur mémoire ou comportement indéfini";
    case VERDICT_CRASH:           return "plantage";
    case VERDICT_WRONG_OUTPUT:    return "sortie incorrecte";
    case VERDICT_WRONG_EXIT:      return "code de retour incorrect";
    }
    return "?";
}

static verdict judge(const exercise *ex, const test_case *tc, const proc_result *res)
{
    if (res->timed_out)
        return VERDICT_TIMEOUT;
    if (res->truncated)
        return VERDICT_TOO_MUCH_OUTPUT;
    /* Un sanitizer arrête le programme dès l'erreur, souvent avant que la
     * sortie soit vidée : son rapport est la vraie cause de l'échec. */
    if (ex->sanitize && (strstr(sb_cstr(&res->err), "==ERROR: ") ||
                         strstr(sb_cstr(&res->err), "runtime error:")))
        return VERDICT_SANITIZER;
    if (res->signaled)
        return VERDICT_CRASH;
    if (!compare_outputs(tc->expected, sb_cstr(&res->out), ex->compare))
        return VERDICT_WRONG_OUTPUT;
    if (res->exit_code != tc->expected_exit)
        return VERDICT_WRONG_EXIT;
    return VERDICT_PASS;
}

int check_run(const exercise *ex, const char *workdir, check_report *rep)
{
    memset(rep, 0, sizeof *rep);
    rep->build_log = xstrdup("");

    const runner *r = runner_for(ex->lang);
    if (!r) {
        err_set("le langage « %s » n'est pas encore pris en charge", ex->lang);
        return -1;
    }
    if (!fs_is_dir(workdir)) {
        err_set("exercice pas encore commencé (dossier %s absent)", workdir);
        return -1;
    }

    strbuf path = STRBUF_INIT;
    path_join(&path, ex->dir, "tests.txt");
    int rc = testspec_load(path.data, &rep->tests);
    sb_free(&path);
    if (rc != 0)
        return -1;

    strbuf diag = STRBUF_INIT;
    int b = r->build(ex, workdir, &diag);
    free(rep->build_log);
    rep->build_log = sb_release(&diag);
    if (b == BUILD_ERROR)
        return -1;
    if (b == BUILD_FAILED) {
        rep->status = CHECK_BUILD_FAILED;
        return 0;
    }

    /* xcalloc : les proc_result sont à zéro, donc libérables même si on
     * s'arrête avant d'avoir exécuté tous les tests. */
    rep->outcomes = xcalloc(rep->tests.len, sizeof *rep->outcomes);
    for (size_t i = 0; i < rep->tests.len; i++) {
        const test_case *tc = &rep->tests.items[i];
        test_outcome *o = &rep->outcomes[i];
        if (r->run(ex, workdir, tc, &o->res) != 0)
            return -1;
        o->result = judge(ex, tc, &o->res);
        if (o->result == VERDICT_PASS)
            rep->passed++;
    }
    rep->status = rep->passed == rep->tests.len ? CHECK_PASSED : CHECK_TESTS_FAILED;
    return 0;
}

void check_report_free(check_report *rep)
{
    if (rep->outcomes)
        for (size_t i = 0; i < rep->tests.len; i++)
            proc_result_free(&rep->outcomes[i].res);
    free(rep->outcomes);
    free(rep->build_log);
    testspec_free(&rep->tests);
    memset(rep, 0, sizeof *rep);
}
