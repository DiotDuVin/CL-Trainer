# Architecture (à coller dans un nouveau chat pour reprendre le projet)

## Contexte

`trainer` est un outil CLI d'entraînement (C++, puis Git, C#, Rust) avec correction
automatique. Il est écrit en C11 + POSIX (`-D_XOPEN_SOURCE=700`) et cible Linux, macOS
et WSL. Environnement de développement : Arch Linux, VS Code (build Microsoft), clangd.
CI : Ubuntu, avec gcc et clang.

Feuille de route :
1. Conception ✅
2. Moteur C++ minimal ✅ (v0.2.0)
3. Progression, `hint`, `next`, `stats`, `validate`, `doctor`, `review` (Leitner)
4. Environ 30 exercices C++ + quiz
5. Git, puis C#, puis Rust

## Conventions

- Retour `0` / `-1`, avec le message dans `err_msg()` (`err_set`, `err_set_errno`).
  Les commandes renvoient `TR_EXIT_OK` (0), `TR_EXIT_FAIL` (1) ou `TR_EXIT_ERROR` (2).
- Allocation via `xmalloc` et compagnie : en cas de mémoire épuisée, `die()`.
- Propriété explicite : chaque `x_load` / `x_parse` a son `x_free`. Une structure `*_INIT`
  est utilisable sans initialisation supplémentaire.
- Nettoyage par `goto out` / `goto fail` (un seul chemin de libération).
- Zéro avertissement avec `-Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes
  -Wmissing-prototypes -Wformat=2 -Wvla -Wundef`, sous gcc et clang. Zéro erreur
  valgrind et ASan.

## Arborescence

```
include/*.h            interfaces publiques (une par module)
src/main.c             options globales, résolution des dossiers, dispatch
src/app.c              helpers des commandes (catalogue, workdir)
src/commands.c         list, show, start (+ génération .vscode/tasks.json)
src/check.c            check : build, tests, verdicts, correction
src/exercise.c         parser exercise.txt
src/testspec.c         parser tests.txt
src/catalog.c          scan récursif, tri, ids en double, prérequis inconnus
src/process.c          fork/exec/poll, timeout, kill du groupe, limites
src/compare.c          normalisation + diff lisible
src/runner.c           registre des runners
src/runners/runner_cpp.c   g++ -std=c++17 -Wall -Wextra -pedantic -g -O0 [-fsanitize=...]
src/util/{error,xalloc,strbuf,strvec,text,fs,term}.c
tests/unit/*.c         mini-framework (CHECK, CHECK_STR, CHECK_INT)
tests/e2e.sh           tests de bout en bout (vrai binaire)
tests/fixtures/exercises/cpp/*   exercices pièges : timeout, crash, leak, ubsan...
exercises/cpp/01-basics/{hello,sum}
docs/exercise-format.md
```

Dossiers résolus au démarrage :
- **exercices** : `--exercises` > `$TRAINER_EXERCISES` > `<dossier du binaire>/exercises`
  > `$(PREFIX)/share/trainer/exercises` > `./exercises`.
- **workspace** : `--workspace` > `$TRAINER_WORKSPACE` > `~/trainer-workspace`.
  Le code de l'élève va dans `<workspace>/<lang>/<id>/`, le binaire compilé dans
  `.build/prog`.

## Signatures

```c
/* error.h */
void err_set(const char *fmt, ...);
void err_set_errno(const char *fmt, ...);
const char *err_msg(void);
void die(const char *fmt, ...);               /* exit(2) */

/* xalloc.h */
void *xmalloc(size_t); void *xcalloc(size_t, size_t); void *xrealloc(void *, size_t);
char *xstrdup(const char *); char *xstrndup(const char *, size_t);
void *grow_array(void *ptr, size_t *cap, size_t needed, size_t elem_size);

/* strbuf.h — invariant data[len] == '\0' */
typedef struct { char *data; size_t len, cap; } strbuf;   /* STRBUF_INIT */
void sb_append(strbuf *, const char *); void sb_append_n(strbuf *, const char *, size_t);
void sb_append_char(strbuf *, char); void sb_appendf(strbuf *, const char *fmt, ...);
void sb_truncate(strbuf *, size_t); void sb_clear(strbuf *); void sb_free(strbuf *);
const char *sb_cstr(const strbuf *); char *sb_release(strbuf *);

/* strvec.h — invariant items[len] == NULL (utilisable comme argv) */
typedef struct { char **items; size_t len, cap; } strvec;  /* STRVEC_INIT */
void sv_push(strvec *, const char *); void sv_push_n(strvec *, const char *, size_t);
void sv_push_owned(strvec *, char *); int sv_contains(const strvec *, const char *);
void sv_free(strvec *);

/* text.h */
int  text_next_line(const char **cursor, const char **line, size_t *len);
int  span_eq(const char *, size_t, const char *lit);
int  span_starts_with(const char *, size_t, const char *prefix);
int  span_is_blank(const char *, size_t); void span_trim(const char **, size_t *);
int  parse_long(const char *, size_t, long min, long max, long *out);
void split_list(const char *, size_t, char sep, strvec *out);
void json_escape(strbuf *, const char *); void escape_visible(strbuf *, const char *, size_t);

/* fs.h */
int fs_read_file(const char *, strbuf *); int fs_write_file(const char *, const char *, size_t);
int fs_exists(const char *); int fs_is_dir(const char *); int fs_is_file(const char *);
int fs_mkdir_p(const char *); int fs_copy_file(const char *, const char *);
int fs_copy_tree(const char *src, const char *dst, int overwrite);
void path_join(strbuf *out, const char *a, const char *b);
int fs_absolute(const char *, strbuf *);

/* term.h */
void term_init(int force);  /* -1 auto (isatty && !NO_COLOR) */
const char *term(term_style); /* T_RESET, T_BOLD, T_DIM, T_RED, T_GREEN, T_YELLOW, T_CYAN */

/* exercise.h */
typedef enum { COMPARE_NORMALIZED, COMPARE_EXACT } compare_mode;
typedef enum { KIND_EXERCISE, KIND_PROJECT } exercise_kind;
typedef struct {
    char *id, *title, *lang, *topic; int level; exercise_kind kind;
    strvec tags, requires, files; long timeout_ms; int estimated_min;
    compare_mode compare; int sanitize;
    char *statement, *explanation, *common_mistakes; strvec hints; char *dir;
} exercise;
int  exercise_parse(const char *text, const char *origin, exercise *);
int  exercise_load(const char *dir, exercise *);
void exercise_free(exercise *);

/* testspec.h */
typedef struct { char *name; int hidden, line; char *input, *expected;
                 strvec args; int expected_exit; } test_case;
typedef struct { test_case *items; size_t len, cap; } test_suite;  /* TEST_SUITE_INIT */
int  testspec_parse(const char *text, const char *origin, test_suite *);
int  testspec_load(const char *path, test_suite *);
void testspec_free(test_suite *);

/* catalog.h */
typedef struct { exercise *items; size_t len, cap; strvec errors; } catalog;
int  catalog_load(const char *root, catalog *);   /* trié par dossier */
const exercise *catalog_find(const catalog *, const char *id);
void catalog_free(catalog *);

/* process.h */
typedef struct { const char *cwd; const char *input; size_t input_len;
                 long timeout_ms; size_t max_output; long mem_limit_mb; } proc_opts;
typedef struct { int exited, exit_code, signaled, signal, timed_out, truncated;
                 long elapsed_ms; strbuf out, err; } proc_result;
int  proc_run(const char *const argv[], const proc_opts *, proc_result *, int *exec_errno);
void proc_result_free(proc_result *);
const char *proc_signal_description(int sig);

/* compare.h */
void compare_normalize(const char *s, strbuf *out);
int  compare_outputs(const char *expected, const char *actual, compare_mode);
void compare_print_diff(FILE *, const char *expected, const char *actual, compare_mode);

/* runner.h */
enum { BUILD_OK = 0, BUILD_FAILED = 1, BUILD_ERROR = -1 };
typedef struct runner {
    const char *lang, *display_name, *default_file;
    int (*prepare_workspace)(const exercise *, const char *workdir);   /* optionnel */
    int (*build)(const exercise *, const char *workdir, strbuf *diag);
    int (*run)(const exercise *, const char *workdir, const test_case *, proc_result *);
} runner;
const runner *runner_for(const char *lang);

/* app.h */
typedef struct { char *exercises_dir, *workspace_dir, *self_path; } app_ctx;
int app_load_catalog(const app_ctx *, catalog *);
const exercise *app_find(const catalog *, const char *id);
void app_workdir(const app_ctx *, const exercise *, strbuf *out);
int cmd_list(const app_ctx *, int argc, char **argv);   /* idem show, start, check */
```

## Verdicts de `check` (ordre de priorité)

temps dépassé > sortie trop longue (> 1 Mo) > sanitizer (`==ERROR:` / `runtime error:`)
> plantage (signal) > sortie incorrecte > code de retour incorrect > réussi.
