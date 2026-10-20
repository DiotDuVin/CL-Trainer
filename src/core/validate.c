/* validate.c — voir validate.h. */
#include "validate.h"

#include <string.h>

#include "check.h"
#include "error.h"
#include "fs.h"
#include "strbuf.h"
#include "testspec.h"

/* Copie <ex->dir>/<variant> dans un dossier temporaire et le vérifie. */
static int check_variant(const exercise *ex, const char *variant, check_report *rep)
{
    strbuf src = STRBUF_INIT, tmp = STRBUF_INIT;
    int rc = -1;
    memset(rep, 0, sizeof *rep);
    path_join(&src, ex->dir, variant);
    if (fs_make_temp_dir("trainer-validate", &tmp) != 0)
        goto out;
    if (fs_copy_tree(src.data, tmp.data, 1) == 0)
        rc = check_run(ex, tmp.data, rep);
    /* Nettoyage même en cas d'échec ; on garde la première erreur. */
    if (fs_remove_tree(tmp.data) != 0 && rc == 0)
        rc = -1;
out:
    sb_free(&src);
    sb_free(&tmp);
    return rc;
}

static void add_problem(strvec *problems, strbuf *msg)
{
    sv_push(problems, sb_cstr(msg));
    sb_clear(msg);
}

/* Première ligne non vide d'un texte, pour résumer un journal de compilation. */
static void first_error_line(const char *log, strbuf *out)
{
    const char *p = strstr(log, "error");
    const char *start = p ? p : log;
    while (start > log && start[-1] != '\n')
        start--;
    const char *end = strchr(start, '\n');
    sb_append_n(out, start, end ? (size_t)(end - start) : strlen(start));
}

int validate_exercise(const exercise *ex, strvec *problems)
{
    strbuf path = STRBUF_INIT, msg = STRBUF_INIT;
    test_suite ts = TEST_SUITE_INIT;
    check_report rep;
    int rc = -1;
    memset(&rep, 0, sizeof rep);

    /* 1. Fichiers et tests */
    path_join(&path, ex->dir, "tests.txt");
    if (testspec_load(path.data, &ts) != 0) {
        sb_append(&msg, err_msg());
        add_problem(problems, &msg);
        rc = 0;
        goto out;
    }
    size_t hidden = 0;
    for (size_t i = 0; i < ts.len; i++)
        hidden += (size_t)ts.items[i].hidden;
    if (hidden == 0) {
        sb_append(&msg, "aucun test caché ([hidden]) pour les cas limites");
        add_problem(problems, &msg);
    }
    static const char *const variants[] = { "starter", "solution" };
    for (size_t i = 0; i < 2; i++) {
        path_join(&path, ex->dir, variants[i]);
        if (!fs_is_dir(path.data)) {
            sb_appendf(&msg, "dossier %s/ manquant", variants[i]);
            add_problem(problems, &msg);
            rc = 0;
            goto out;
        }
    }

    /* 2. La solution réussit tout */
    if (check_variant(ex, "solution", &rep) != 0)
        goto out;
    if (rep.status == CHECK_BUILD_FAILED) {
        sb_append(&msg, "la solution ne compile pas : ");
        first_error_line(rep.build_log, &msg);
        add_problem(problems, &msg);
    } else {
        for (size_t i = 0; i < rep.tests.len; i++) {
            if (rep.outcomes[i].result != VERDICT_PASS) {
                sb_appendf(&msg, "la solution échoue au test « %s » (%s)",
                           rep.tests.items[i].name, verdict_label(rep.outcomes[i].result));
                add_problem(problems, &msg);
            }
        }
    }
    check_report_free(&rep);

    /* 3. Le starter compile mais ne réussit pas tout */
    if (check_variant(ex, "starter", &rep) != 0)
        goto out;
    if (rep.status == CHECK_BUILD_FAILED) {
        sb_append(&msg, "le code de départ ne compile pas : ");
        first_error_line(rep.build_log, &msg);
        add_problem(problems, &msg);
    } else if (rep.status == CHECK_PASSED) {
        sb_append(&msg, "le code de départ réussit déjà tous les tests");
        add_problem(problems, &msg);
    }
    rc = 0;

out:
    check_report_free(&rep);
    testspec_free(&ts);
    sb_free(&path);
    sb_free(&msg);
    return rc;
}
