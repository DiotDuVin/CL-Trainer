/* runner_cpp.c — compilation et exécution des exercices C++ avec g++. */
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "fs.h"
#include "runner.h"
#include "strvec.h"

#define BUILD_TIMEOUT_MS 120000L
#define MAX_OUTPUT       (1024 * 1024) /* 1 Mo par flux */
#define MEM_LIMIT_MB     512L
#define BINARY           ".build/prog"
#define DEFAULT_FILE     "main.cpp"

/* Options de compilation, partagées avec compile_flags.txt (clangd) pour
 * que l'éditeur signale exactement ce que signalera le correcteur.
 * -std=c++17 est explicite : le standard par défaut change selon la version
 * de g++, et on veut le même comportement sur toutes les machines. */
static const char *const base_flags[] = {
    "-std=c++17", "-Wall", "-Wextra", "-pedantic", "-g", "-O0",
};
/* AddressSanitizer + UndefinedBehaviorSanitizer : détectent à l'exécution
 * les fuites, débordements de tableau, use-after-free, débordements
 * d'entiers signés... Par défaut UBSan affiche l'erreur puis CONTINUE (code
 * de retour 0) : -fno-sanitize-recover=all le fait s'arrêter. */
static const char *const sanitize_flags[] = {
    "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
    "-fno-omit-frame-pointer",
};

#define COUNT(a) (sizeof (a) / sizeof (a)[0])

static const char *compiler(void)
{
    const char *cxx = getenv("TRAINER_CXX");
    return (cxx && *cxx) ? cxx : "g++";
}

static int is_cpp_source(const char *name)
{
    const char *dot = strrchr(name, '.');
    return dot && (strcmp(dot, ".cpp") == 0 || strcmp(dot, ".cc") == 0 ||
                   strcmp(dot, ".cxx") == 0);
}

static int cpp_prepare_workspace(const exercise *ex, const char *workdir)
{
    (void)ex;
    strbuf path = STRBUF_INIT, content = STRBUF_INIT;
    for (size_t i = 0; i < COUNT(base_flags); i++)
        sb_appendf(&content, "%s\n", base_flags[i]);
    sb_append(&content, "-xc++\n"); /* les .h sont aussi du C++ */
    path_join(&path, workdir, "compile_flags.txt");
    int rc = fs_write_file(path.data, content.data, content.len);
    sb_free(&path);
    sb_free(&content);
    return rc;
}

static int cpp_build(const exercise *ex, const char *workdir, strbuf *diag)
{
    strvec argv = STRVEC_INIT;
    strbuf path = STRBUF_INIT;
    proc_result res;
    int rc = BUILD_ERROR;
    memset(&res, 0, sizeof res);

    sv_push(&argv, compiler());
    for (size_t i = 0; i < COUNT(base_flags); i++)
        sv_push(&argv, base_flags[i]);
    if (ex->sanitize)
        for (size_t i = 0; i < COUNT(sanitize_flags); i++)
            sv_push(&argv, sanitize_flags[i]);
    sv_push(&argv, "-o");
    sv_push(&argv, BINARY);

    /* Fichiers à compiler : ceux de `files` qui sont des sources C++
     * (les .hpp listés sont modifiables mais pas compilés à part). */
    size_t nsrc = 0;
    if (ex->files.len == 0) {
        sv_push(&argv, DEFAULT_FILE);
        nsrc = 1;
    }
    for (size_t i = 0; i < ex->files.len; i++) {
        if (is_cpp_source(ex->files.items[i])) {
            sv_push(&argv, ex->files.items[i]);
            nsrc++;
        }
    }
    if (nsrc == 0) {
        err_set("%s : aucun fichier .cpp dans « files »", ex->id);
        goto out;
    }
    for (size_t i = argv.len - nsrc; i < argv.len; i++) {
        path_join(&path, workdir, argv.items[i]);
        if (!fs_is_file(path.data)) {
            err_set("fichier introuvable : %s", path.data);
            goto out;
        }
    }

    path_join(&path, workdir, ".build");
    if (fs_mkdir_p(path.data) != 0)
        goto out;

    proc_opts opts = { 0 };
    opts.cwd = workdir;
    opts.timeout_ms = BUILD_TIMEOUT_MS;
    opts.max_output = MAX_OUTPUT;
    int exec_errno;
    /* Un char ** ne se convertit pas implicitement en const char *const *
     * en C (contrairement au C++) : le cast explicite est nécessaire. */
    if (proc_run((const char *const *)argv.items, &opts, &res, &exec_errno) != 0) {
        if (exec_errno == ENOENT)
            err_set("compilateur « %s » introuvable. Installez-le "
                    "(Arch : sudo pacman -S gcc ; Debian/Ubuntu : sudo apt install g++)",
                    compiler());
        goto out;
    }

    sb_append(diag, sb_cstr(&res.out));
    sb_append(diag, sb_cstr(&res.err));
    if (res.timed_out)
        sb_append(diag, "compilation interrompue : plus de 120 s.\n");
    rc = (res.exited && res.exit_code == 0) ? BUILD_OK : BUILD_FAILED;

out:
    proc_result_free(&res);
    sv_free(&argv);
    sb_free(&path);
    return rc;
}

static int cpp_run(const exercise *ex, const char *workdir, const test_case *tc,
                   proc_result *res)
{
    strvec argv = STRVEC_INIT;
    sv_push(&argv, "./" BINARY);
    for (size_t i = 0; i < tc->args.len; i++)
        sv_push(&argv, tc->args.items[i]);

    proc_opts opts = { 0 };
    opts.cwd = workdir;
    opts.input = tc->input;
    opts.input_len = strlen(tc->input);
    opts.timeout_ms = ex->timeout_ms;
    opts.max_output = MAX_OUTPUT;
    /* AddressSanitizer réserve d'énormes plages d'adresses virtuelles : une
     * limite RLIMIT_AS l'empêcherait de démarrer. */
    opts.mem_limit_mb = ex->sanitize ? 0 : MEM_LIMIT_MB;

    int rc = proc_run((const char *const *)argv.items, &opts, res, NULL);
    sv_free(&argv);
    return rc;
}

const runner runner_cpp = {
    .lang = "cpp",
    .display_name = "C++",
    .default_file = DEFAULT_FILE,
    .prepare_workspace = cpp_prepare_workspace,
    .build = cpp_build,
    .run = cpp_run,
};
