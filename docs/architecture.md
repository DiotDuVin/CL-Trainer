# Architecture (à coller dans un nouveau chat pour reprendre le projet)

## Contexte

**CL-Trainer** (https://github.com/DiotDuVin/CL-Trainer) est un outil d'entraînement
(C++, puis Git, C#, Rust) avec correction automatique. Il est écrit en C11 + POSIX
(`-D_XOPEN_SOURCE=700`) pour Linux, macOS et WSL.

- **Environnement de développement :** Arch Linux, KDE Plasma, VS Code (build Microsoft),
  clangd, `gh` pour GitHub.
- **CI :** Ubuntu, avec gcc et clang.
- **Méthode de travail :** une branche par étape, puis une pull request relue et fusionnée
  par Rémi.

Feuille de route :
1. Conception ✅
2. Moteur C++ minimal ✅ (v0.2.0)
3. libtrainer + progression ✅ (v0.3.0) : `hint`, `solution`, `next`, `stats`,
   `validate`, `doctor`, `--json`
4. Application graphique ✅ (v0.4.0) : **Qt 6 Widgets / C++17**, construite avec CMake,
   liée à libtrainer. VS Code reste l'éditeur ; l'application sert de tableau de bord.
5a. Cours ✅ : chapitres (`lesson.md` + exercices), `learn`, exemples vérifiés par
   `validate` ; 3 chapitres C++ et 9 exercices.
5b. Cours C++ 4 à 14 et exercices restants, par lots (plan ci-dessous).
5c. Embellissement de l'application (thème clair/sombre, mise en page), puis `review`
   (répétition espacée Leitner, avec les champs `box=` et `due=` dans `progress.txt`).

   Plan C++ : 1 Premier programme ✅, 2 Références et const ✅, 3 Fonctions modernes ✅,
   4 vector/array ✅, 5 Classes ✅, 6 RAII et destructeurs ✅, 7 new/delete et pointeurs
   intelligents ✅, 8 Sémantique de déplacement ✅, 9 Conteneurs associatifs ✅,
   10 Algorithmes et lambdas, 11 Templates, 12 Exceptions, 13 Héritage et
   polymorphisme, 14 Organiser un projet (en-têtes, CMake).
6. Git, puis C#, puis Rust

## Organisation

```
include/            API publique de libtrainer (trainer.h = tout) ; extern "C", compilable en C++20
src/util/           error (thread-local), xalloc, strbuf, strvec, text, fs
src/core/           le moteur, SANS affichage :
  config.c            dossiers (exercices, workspace, données XDG, chemin de la CLI)
  exercise.c          parser exercise.txt         testspec.c  parser tests.txt
  lesson.c            parser lesson.md (en-tête, ---, Markdown) ; extraction des
                      blocs « ```cpp example » et de leur « ```output »
  catalog.c           scan, tri, doublons, prérequis inconnus, cycles (DFS 3 couleurs) ;
                      chapitres : lesson.md rattaché aux exercices des sous-dossiers
  process.c           fork/execv/poll, timeout, kill du groupe ; PATH résolu dans le père
  compare.c           normalisation + première différence (output_diff)
  runner.c            registre ; runners/runner_cpp.c (g++ -std=c++17 ..., sanitize)
  check.c             check_run -> check_report (verdicts)
  workspace.c         start (copie du starter, .vscode/tasks.json, compile_flags.txt), code
  progress.c          progress.txt : lecture, écriture atomique, verrou fcntl, événements
  stats.c             statistiques par thème, points faibles, recommandation (next)
  validate.c          contrôle qualité d'un exercice (dans des dossiers temporaires)
  doctor.c            outils installés (--version) + conseils d'installation par distribution
src/cli/            interface terminal : main.c, cli.c (aides d'affichage), term.c (couleurs),
                    json.c, cmd_catalog.c (list, show), cmd_start.c, cmd_check.c,
                    cmd_learn.c (hint, solution, next, stats), cmd_admin.c (validate, doctor)
tests/unit/         mini-framework CHECK* ; une suite par module
tests/e2e.sh        bout en bout (progression isolée via XDG_DATA_HOME)
tests/cxx/          headers.cpp : l'API compile en C++20
tests/fixtures/     exercices pièges (exercises/) et catalogue cassé (broken/)
gui/                application Qt 6 (C++17, CMake ; compile aussi libtrainer depuis src/)
  src/engine.*        enveloppe RAII de libtrainer ; erreurs C -> exceptions ;
                      runCheck() thread-safe (copie de l'exercice) -> shared_ptr<CheckResult>
  src/catalogview.*   QTreeWidget langage > thème > exercice (id dans Qt::UserRole)
  src/lessonview.*    cours (Markdown), « Marquer comme lu », « Passer aux exercices »
  src/exerciseview.*  énoncé (Markdown), boutons, résultats, correction ; vérification
                      dans un thread (QtConcurrent::run + QFutureWatcher)
  src/mainwindow.*    assemblage, barre d'outils (Suivant, Actualiser), QSettings,
                      QFileSystemWatcher sur le dossier de progression (mises à jour
                      venant du terminal / de VS Code)
  src/format.*        mise en forme Markdown / HTML (fonctions libres)
  tests/test_gui.cpp  QTest : parcours complet d'un élève en mode « offscreen »
```

Construction : `src/core` + `src/util` produisent `build/<variante>/libtrainer.a` ;
`src/cli` produit `trainer` (Makefile) ; `make gui` produit `trainer-gui` (CMake). Les en-têtes de `src/cli` sont invisibles du moteur
(`-Isrc/cli` est réservé à la CLI et aux tests).

## Conventions

- Retour `0` / `-1`, avec le message dans `err_msg()`. Les commandes renvoient
  `TR_EXIT_OK` (0), `TR_EXIT_FAIL` (1) ou `TR_EXIT_ERROR` (2).
- `xmalloc` et compagnie appellent `die()` si la mémoire est épuisée. Propriété explicite :
  chaque `x_load` / `x_run` / `x_compute` a son `x_free`.
- Nettoyage par `goto out` / `goto fail`.
- Le moteur n'affiche jamais rien : il remplit des structures.
- **Threads** (pour l'application Qt) : `err_msg` est par thread ; le fils créé par `fork`
  n'appelle que des fonctions async-signal-safe (`execv`) ; `SIGPIPE` est ignoré
  globalement. Les verrous `fcntl` de la progression étant par processus, dans un même
  programme, `progress_record` doit être appelée d'un seul thread à la fois.
- Zéro avertissement (gcc, clang, `-Werror`) ; zéro erreur valgrind et ASan.

## Signatures principales (include/)

```c
/* config.h */
typedef struct { char *exercises_dir, *workspace_dir, *data_dir, *progress_path, *cli_path; } trainer_config;
int  config_resolve(const char *opt_exercises, const char *opt_workspace, const char *argv0, trainer_config *);
void config_free(trainer_config *);

/* exercise.h — champs : id, title, lang, topic, level, kind, tags, prereqs (clé « requires »),
   files, timeout_ms, estimated_min, compare, sanitize, statement, explanation,
   common_mistakes, hints, dir */
int  exercise_parse(const char *text, const char *origin, exercise *);
int  exercise_load(const char *dir, exercise *);   void exercise_free(exercise *);

/* catalog.h */
typedef struct { exercise *items; size_t len, cap; strvec errors; } catalog;  /* CATALOG_INIT */
int  catalog_load(const char *root, catalog *);
const exercise *catalog_find(const catalog *, const char *id);   void catalog_free(catalog *);

/* workspace.h */
typedef enum { START_CREATED, START_EXISTED, START_RESET } start_result;
void workspace_dir(const trainer_config *, const exercise *, strbuf *out);
int  workspace_exists(const trainer_config *, const exercise *);
int  workspace_start(const trainer_config *, const exercise *, int force, start_result *);
int  workspace_open_editor(const char *dir);

/* check.h */
typedef enum { VERDICT_PASS, VERDICT_TIMEOUT, VERDICT_TOO_MUCH_OUTPUT, VERDICT_SANITIZER,
               VERDICT_CRASH, VERDICT_WRONG_OUTPUT, VERDICT_WRONG_EXIT } verdict;
typedef struct { verdict result; proc_result res; } test_outcome;
typedef enum { CHECK_PASSED, CHECK_TESTS_FAILED, CHECK_BUILD_FAILED } check_status;
typedef struct { check_status status; char *build_log; test_suite tests;
                 test_outcome *outcomes; size_t passed; } check_report;
int  check_run(const exercise *, const char *workdir, check_report *);
void check_report_free(check_report *);   const char *verdict_label(verdict);

/* compare.h */
typedef struct { int empty_output, same_lines, line; char *expected_line, *actual_line; } output_diff;
int  compare_outputs(const char *expected, const char *actual, compare_mode);
void compare_first_diff(const char *expected, const char *actual, compare_mode, output_diff *);
void output_diff_free(output_diff *);

/* progress.h — fichier « # trainer-progress v1 » puis « id clé=valeur ... » */
typedef enum { STATUS_NEW, STATUS_STARTED, STATUS_FAILED, STATUS_PASSED } exercise_status;
typedef struct { char *id; exercise_status status; int attempts, build_failures, hints,
                 solution_seen; char first[21], last[21], passed_at[21]; char *extra; } progress_entry;
typedef struct { progress_entry *items; size_t len, cap; } progress;   /* PROGRESS_INIT */
typedef enum { EVENT_START, EVENT_CHECK_BUILD_FAILED, EVENT_CHECK_FAILED, EVENT_CHECK_PASSED,
               EVENT_HINT /* arg = n° indice */, EVENT_SOLUTION } progress_event;
int  progress_load(const char *path, progress *);       /* absent = vide */
int  progress_save(const char *path, const progress *); /* tmp + fsync + rename + fsync(dir) */
int  progress_record(const char *path, const char *id, progress_event, int arg); /* verrou + charger + appliquer + sauver */
void progress_apply(progress *, const char *id, progress_event, int arg, const char *now);
const progress_entry *progress_find(const progress *, const char *id);
exercise_status progress_status(const progress *, const char *id);
void progress_free(progress *);   const char *progress_status_name(exercise_status);

/* stats.h — struggle = échecs + 0,5×indices + 2×solution ; faible si ≥ 1,5 et ≥ 2 exercices tentés */
typedef struct { const char *lang, *topic; size_t total, passed, attempted; double struggle; } topic_stats;
typedef struct { topic_stats *topics; size_t len, cap, total, passed; int checks, first_try; } stats;
void stats_compute(const catalog *, const progress *, stats *);   void stats_free(stats *);
int  stats_topic_is_weak(const topic_stats *);
int  exercise_unlocked(const catalog *, const progress *, const exercise *);
typedef enum { RECO_RESUME, RECO_NEXT, RECO_WEAK_TOPIC, RECO_ALL_DONE, RECO_LOCKED } reco_reason;
typedef struct { const exercise *ex; reco_reason reason; const char *topic; } recommendation;
void recommend_next(const catalog *, const progress *, const char *lang, recommendation *);
   /* 1. reprendre le plus récent commencé/échoué ; 2. sinon nouveau déverrouillé de plus
      bas niveau, thème faible d'abord, puis ordre du catalogue */

/* validate.h — tests.txt valide + ≥1 test caché ; solution réussit tout ; starter compile et échoue */
int  validate_exercise(const exercise *, strvec *problems);
int  validate_lesson(const lesson *, strvec *problems);   /* exemples : compilés (ASan/UBSan), exécutés */

/* lesson.h */
typedef struct { char *id, *title, *lang, *topic; int estimated_min; char *body; int body_line;
                 char *dir; } lesson;
int  lesson_parse(const char *text, const char *origin, lesson *);  int lesson_load(const char *dir, lesson *);
void lesson_free(lesson *);
void lesson_extract_examples(const lesson *, lesson_examples *);  /* { code, expected, line } */
/* catalog : + lessons/nlessons, catalog_find_lesson(cat, id) ; exercise.lesson = id du cours */
/* progress : + EVENT_LESSON_READ (cours lu = STATUS_PASSED) */
/* stats : + RECO_LESSON, recommendation.lesson_to_read */

/* doctor.h */
typedef struct { const char *command, *purpose; int essential, found; char *version, *install_hint; } tool_status;
typedef struct { tool_status *tools; size_t len; char *distro; } doctor_report;
void doctor_run(doctor_report *);   void doctor_free(doctor_report *);

/* process.h */
typedef struct { const char *cwd, *input; size_t input_len; long timeout_ms; size_t max_output;
                 long mem_limit_mb; } proc_opts;
typedef struct { int exited, exit_code, signaled, signal, timed_out, truncated; long elapsed_ms;
                 strbuf out, err; } proc_result;
int  proc_run(const char *const argv[], const proc_opts *, proc_result *, int *exec_errno);
void proc_result_free(proc_result *);   const char *proc_signal_description(int sig);

/* runner.h */
typedef struct runner { const char *lang, *display_name, *default_file;
    int (*prepare_workspace)(const exercise *, const char *workdir);
    int (*build)(const exercise *, const char *workdir, strbuf *diag);   /* BUILD_OK/FAILED/ERROR */
    int (*run)(const exercise *, const char *workdir, const test_case *, proc_result *); } runner;
const runner *runner_for(const char *lang);

/* util : error.h (err_set, err_set_errno, err_msg, die), xalloc.h, strbuf.h, strvec.h,
   text.h (text_next_line, span_*, parse_long, split_list, json_escape, escape_visible),
   fs.h (fs_read_file, fs_write_file, fs_mkdir_p, fs_copy_tree, fs_remove_tree,
   fs_make_temp_dir, path_join, fs_absolute...) */
```

## Piège C/C++ rencontré deux fois

Un champ de structure ne doit pas porter le nom d'un type utilisé dans la même structure
(`verdict verdict;`, `const lesson *lesson;`) : légal en C, erreur en C++
(« changes meaning »). `make cxx-headers` le détecte.

## Application Qt : règles

- Les vues mémorisent un **id d'exercice**, jamais un pointeur : `Engine::reload()`
  invalide les pointeurs du catalogue.
- La vérification tourne dans un thread avec sa propre copie de l'exercice
  (`exercise_load`) ; la progression est enregistrée dans le thread principal.
- `ExerciseView::confirm` est injectable : les tests évitent ainsi les boîtes de dialogue.
- Les widgets ont un `objectName` (`checkButton`, `tests`...) pour que les tests les
  retrouvent avec `findChild`.

## Commandes

`learn [<id>]`, `list [--lang L] [--topic T] [--json]`, `show <id>` (exercice ou cours), `start <id> [--force] [--open]`,
`check <id> [--json]`, `hint <id>`, `solution <id> [--yes]`, `next [--lang L]`, `stats`,
`validate [<id>...]`, `doctor`.

Options globales : `--exercises DIR`, `--workspace DIR`, `--no-color`.
Variables d'environnement : `TRAINER_EXERCISES`, `TRAINER_WORKSPACE`, `XDG_DATA_HOME`,
`NO_COLOR`, `TRAINER_CXX`.
