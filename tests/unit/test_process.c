/* test_process.c — tests de proc_run avec de vrais programmes système. */
#include <errno.h>
#include <signal.h>

#include "process.h"
#include "test.h"

static proc_opts default_opts(void)
{
    proc_opts o = { 0 };
    o.timeout_ms = 5000;
    o.max_output = 1 << 20;
    return o;
}

void test_process_suite(void)
{
    proc_result res;
    proc_opts o = default_opts();

    /* Sortie standard et code de retour */
    const char *echo[] = { "echo", "bonjour", NULL };
    CHECK(proc_run(echo, &o, &res, NULL) == 0);
    CHECK(res.exited && res.exit_code == 0);
    CHECK_STR(sb_cstr(&res.out), "bonjour\n");
    proc_result_free(&res);

    /* stdin -> stdout */
    const char *cat[] = { "cat", NULL };
    o.input = "ligne 1\nligne 2\n";
    o.input_len = strlen(o.input);
    CHECK(proc_run(cat, &o, &res, NULL) == 0);
    CHECK_STR(sb_cstr(&res.out), "ligne 1\nligne 2\n");
    proc_result_free(&res);

    /* Grosse entrée (> taille d'un tube, 64 Kio) : vérifie qu'il n'y a pas
     * d'interblocage entre l'écriture de stdin et la lecture de stdout. */
    strbuf big = STRBUF_INIT;
    for (int i = 0; i < 50000; i++)
        sb_append(&big, "0123456789\n");
    o.input = big.data;
    o.input_len = big.len;
    CHECK(proc_run(cat, &o, &res, NULL) == 0);
    CHECK_INT(res.out.len, big.len);
    proc_result_free(&res);
    sb_free(&big);
    o = default_opts();

    /* Programme qui ne lit pas son stdin : pas de SIGPIPE pour nous. */
    const char *truecmd[] = { "true", NULL };
    o.input = "ignoré\n";
    o.input_len = strlen(o.input);
    CHECK(proc_run(truecmd, &o, &res, NULL) == 0);
    CHECK(res.exited && res.exit_code == 0);
    proc_result_free(&res);
    o = default_opts();

    /* stderr séparé et code de retour non nul */
    const char *sh_err[] = { "sh", "-c", "echo oups >&2; exit 3", NULL };
    CHECK(proc_run(sh_err, &o, &res, NULL) == 0);
    CHECK_INT(res.exit_code, 3);
    CHECK_STR(sb_cstr(&res.err), "oups\n");
    CHECK_STR(sb_cstr(&res.out), "");
    proc_result_free(&res);

    /* Plantage par signal */
    const char *segv[] = { "sh", "-c", "kill -SEGV $$", NULL };
    CHECK(proc_run(segv, &o, &res, NULL) == 0);
    CHECK(res.signaled && res.signal == SIGSEGV);
    proc_result_free(&res);

    /* Délai dépassé, y compris pour un petit-enfant (sleep lancé par sh) */
    const char *slow[] = { "sh", "-c", "sleep 5; echo trop tard", NULL };
    o.timeout_ms = 200;
    CHECK(proc_run(slow, &o, &res, NULL) == 0);
    CHECK(res.timed_out);
    CHECK(res.elapsed_ms < 2000);
    CHECK_STR(sb_cstr(&res.out), "");
    proc_result_free(&res);
    o = default_opts();

    /* Sortie infinie : coupée à max_output */
    const char *yes[] = { "yes", NULL };
    o.max_output = 4096;
    CHECK(proc_run(yes, &o, &res, NULL) == 0);
    CHECK(res.truncated);
    CHECK_INT(res.out.len, 4096);
    proc_result_free(&res);
    o = default_opts();

    /* Commande introuvable : erreur de lancement avec errno */
    const char *missing[] = { "commande-qui-n-existe-pas", NULL };
    int eno = 0;
    CHECK(proc_run(missing, &o, &res, &eno) != 0);
    CHECK_INT(eno, ENOENT);
    proc_result_free(&res);

    /* Dossier de travail */
    const char *pwd[] = { "pwd", NULL };
    o.cwd = "/";
    CHECK(proc_run(pwd, &o, &res, NULL) == 0);
    CHECK_STR(sb_cstr(&res.out), "/\n");
    proc_result_free(&res);
    /* Chemin relatif : résolu depuis le dossier du FILS (cwd) */
    const char *rel[] = { "./bin/sh", "-c", "echo relatif", NULL };
    o.cwd = "/";
    CHECK(proc_run(rel, &o, &res, NULL) == 0);
    CHECK_STR(sb_cstr(&res.out), "relatif\n");
    proc_result_free(&res);
    o.cwd = "/dossier/inexistant";
    CHECK(proc_run(pwd, &o, &res, &eno) != 0);
    proc_result_free(&res);
}
