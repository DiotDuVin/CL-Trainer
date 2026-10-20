/* cmd_learn.c — sous-commandes d'accompagnement : hint, solution, next, stats. */
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cli.h"
#include "fs.h"
#include "term.h"

/* Chargement commun : catalogue + progression + exercice demandé. */
typedef struct {
    catalog cat;
    progress prog;
    const exercise *ex;
} session;

static int open_session(const trainer_config *cfg, const char *id, session *s)
{
    memset(s, 0, sizeof *s);
    int rc = cli_load_catalog(cfg, &s->cat);
    if (rc != TR_EXIT_OK)
        return rc;
    cli_load_progress(cfg, &s->prog);
    if (id && !(s->ex = cli_find(&s->cat, id)))
        return TR_EXIT_ERROR;
    return TR_EXIT_OK;
}

static void close_session(session *s)
{
    progress_free(&s->prog);
    catalog_free(&s->cat);
}

/* ---------------------------------------------------------------- hint */

int cmd_hint(const trainer_config *cfg, int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage : trainer hint <id>   (révèle l'indice suivant)\n");
        return TR_EXIT_ERROR;
    }
    session s;
    int rc = open_session(cfg, argv[1], &s);
    if (rc != TR_EXIT_OK) {
        close_session(&s);
        return rc;
    }
    const exercise *ex = s.ex;
    size_t total = ex->hints.len;
    if (total == 0) {
        printf("Pas d'indice pour cet exercice. Relis l'énoncé : trainer show %s\n", ex->id);
        close_session(&s);
        return TR_EXIT_OK;
    }

    /* Les indices sont révélés un par un : on réaffiche les précédents. */
    const progress_entry *e = progress_find(&s.prog, ex->id);
    size_t seen = e ? (size_t)e->hints : 0;
    size_t shown = seen < total ? seen + 1 : total;
    for (size_t i = 0; i < shown; i++) {
        int is_new = i == seen;
        printf("%s💡 Indice %zu/%zu%s%s\n%s\n\n", term(T_BOLD), i + 1, total,
               is_new ? " (nouveau)" : "", term(T_RESET), ex->hints.items[i]);
    }
    if (shown > seen)
        cli_record(cfg, ex->id, EVENT_HINT, (int)shown);
    if (shown == total)
        printf("%sTous les indices sont affichés. En dernier recours : trainer solution %s%s\n",
               term(T_DIM), ex->id, term(T_RESET));
    else
        printf("%sIndice suivant : trainer hint %s%s\n", term(T_DIM), ex->id, term(T_RESET));
    close_session(&s);
    return TR_EXIT_OK;
}

/* ---------------------------------------------------------------- solution */

static int confirm(const char *question)
{
    if (!isatty(STDIN_FILENO))
        return 0; /* pas de clavier (script) : il faut --yes */
    printf("%s [o/N] ", question);
    fflush(stdout);
    char answer[16];
    if (!fgets(answer, sizeof answer, stdin))
        return 0;
    return answer[0] == 'o' || answer[0] == 'O' || answer[0] == 'y' || answer[0] == 'Y';
}

int cmd_solution(const trainer_config *cfg, int argc, char **argv)
{
    const char *id = NULL;
    int yes = 0, bad = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--yes") == 0)
            yes = 1;
        else if (!id && argv[i][0] != '-')
            id = argv[i];
        else
            bad = 1;
    }
    if (!id || bad) {
        fprintf(stderr, "usage : trainer solution <id> [--yes]\n");
        return TR_EXIT_ERROR;
    }
    session s;
    int rc = open_session(cfg, id, &s);
    if (rc != TR_EXIT_OK) {
        close_session(&s);
        return rc;
    }
    const exercise *ex = s.ex;
    int passed = progress_status(&s.prog, ex->id) == STATUS_PASSED;
    if (!passed && !yes &&
        !confirm("Tu n'as pas encore réussi cet exercice. Voir la solution quand même ?\n"
                 "(Ce sera noté dans ta progression, et l'exercice te sera reproposé.)")) {
        printf("D'accord. Un indice peut suffire : trainer hint %s\n"
               "%s(Sans clavier, confirmer avec --yes.)%s\n", ex->id, term(T_DIM), term(T_RESET));
        close_session(&s);
        return TR_EXIT_FAIL;
    }

    /* Fichiers de la solution : ceux de `files`, ou le fichier par défaut. */
    const runner *r = runner_for(ex->lang);
    strvec files = STRVEC_INIT;
    for (size_t i = 0; i < ex->files.len; i++)
        sv_push(&files, ex->files.items[i]);
    if (files.len == 0 && r)
        sv_push(&files, r->default_file);
    strbuf path = STRBUF_INIT, base = STRBUF_INIT, content = STRBUF_INIT;
    path_join(&base, ex->dir, "solution");
    for (size_t i = 0; i < files.len; i++) {
        path_join(&path, base.data, files.items[i]);
        sb_clear(&content);
        if (fs_read_file(path.data, &content) != 0)
            continue;
        printf("%s── %s ──%s\n%s\n", term(T_BOLD), files.items[i], term(T_RESET),
               sb_cstr(&content));
    }
    printf("%s── Explication ──%s\n\n%s\n", term(T_BOLD), term(T_RESET), ex->explanation);
    if (ex->common_mistakes)
        printf("\n%s── Erreurs fréquentes ──%s\n\n%s\n", term(T_BOLD), term(T_RESET),
               ex->common_mistakes);
    /* Consulter une solution après avoir réussi ne compte pas comme une aide. */
    if (!passed) {
        cli_record(cfg, ex->id, EVENT_SOLUTION, 0);
        printf("\n%sConseil : ferme la solution et réécris le code de mémoire, puis "
               "trainer check %s.%s\n", term(T_DIM), ex->id, term(T_RESET));
    }
    sv_free(&files);
    sb_free(&path);
    sb_free(&base);
    sb_free(&content);
    close_session(&s);
    return TR_EXIT_OK;
}

/* ---------------------------------------------------------------- next */

int cmd_next(const trainer_config *cfg, int argc, char **argv)
{
    const char *lang = NULL;
    if (argc == 3 && strcmp(argv[1], "--lang") == 0)
        lang = argv[2];
    else if (argc != 1) {
        fprintf(stderr, "usage : trainer next [--lang LANG]\n");
        return TR_EXIT_ERROR;
    }
    session s;
    int rc = open_session(cfg, NULL, &s);
    if (rc != TR_EXIT_OK) {
        close_session(&s);
        return rc;
    }

    recommendation reco;
    recommend_next(&s.cat, &s.prog, lang, &reco);
    switch (reco.reason) {
    case RECO_ALL_DONE:
        printf("🎉 Tous les exercices%s%s sont réussis !\n", lang ? " " : "",
               lang ? lang_name(lang) : "");
        break;
    case RECO_LOCKED:
        printf("Les exercices restants ont des prérequis non réussis. "
               "Voir : trainer list\n");
        break;
    case RECO_RESUME:
        printf("Tu as commencé cet exercice sans le terminer : finis-le d'abord.\n\n");
        break;
    case RECO_WEAK_TOPIC:
        printf("Le thème « %s » te pose des difficultés : on le renforce.\n\n", reco.topic);
        break;
    case RECO_NEXT:
        printf("Prochain exercice de la progression :\n\n");
        break;
    }
    if (reco.ex) {
        const exercise *ex = reco.ex;
        printf("  %s%s%s  ", term(T_BOLD), ex->title, term(T_RESET));
        print_level(ex->level);
        printf("\n  %s%s · %s · thème %s%s\n\n", term(T_DIM), ex->id, lang_name(ex->lang),
               ex->topic, term(T_RESET));
        if (reco.reason == RECO_RESUME)
            printf("  Vérifier : %strainer check %s%s\n", term(T_CYAN), ex->id, term(T_RESET));
        else
            printf("  Énoncé : trainer show %s\n  Commencer : %strainer start %s%s\n",
                   ex->id, term(T_CYAN), ex->id, term(T_RESET));
    }
    close_session(&s);
    return TR_EXIT_OK;
}

/* ---------------------------------------------------------------- stats */

static const char *struggle_label(const topic_stats *t)
{
    if (t->attempted == 0)
        return "—";
    if (t->struggle < 0.5)
        return "faible";
    if (t->struggle < STATS_WEAK_THRESHOLD)
        return "moyenne";
    return "élevée";
}

int cmd_stats(const trainer_config *cfg, int argc, char **argv)
{
    (void)argv;
    if (argc != 1) {
        fprintf(stderr, "usage : trainer stats\n");
        return TR_EXIT_ERROR;
    }
    session s;
    int rc = open_session(cfg, NULL, &s);
    if (rc != TR_EXIT_OK) {
        close_session(&s);
        return rc;
    }
    stats st;
    stats_compute(&s.cat, &s.prog, &st);

    printf("%sProgression%s  ", term(T_BOLD), term(T_RESET));
    print_bar(st.passed, st.total, 24);
    printf("  %zu/%zu exercices réussis\n", st.passed, st.total);
    printf("%s%d vérification(s), %d exercice(s) réussi(s) du premier coup sans aide%s\n",
           term(T_DIM), st.checks, st.first_try, term(T_RESET));

    const char *current = "";
    for (size_t i = 0; i < st.len; i++) {
        const topic_stats *t = &st.topics[i];
        if (strcmp(t->lang, current) != 0) {
            printf("\n%s%-22s%s %-18s %-8s %s\n", term(T_BOLD), lang_name(t->lang),
                   term(T_RESET), "", "réussis", "difficulté");
            current = t->lang;
        }
        printf("  %-20s ", t->topic);
        print_bar(t->passed, t->total, 16);
        int weak = stats_topic_is_weak(t);
        printf("   %3zu/%-4zu %s%s%s\n", t->passed, t->total, weak ? term(T_YELLOW) : "",
               struggle_label(t), weak ? term(T_RESET) : "");
    }

    printf("\n%sPoints faibles :%s ", term(T_BOLD), term(T_RESET));
    int any = 0;
    for (size_t i = 0; i < st.len; i++) {
        const topic_stats *t = &st.topics[i];
        if (stats_topic_is_weak(t)) {
            printf("%s%s/%s", any ? ", " : "", t->lang, t->topic);
            any = 1;
        }
    }
    printf("%s\n", any ? "" : "aucun pour l'instant");
    printf("%sDifficulté = échecs + 0,5 × indices + 2 × solutions vues, en moyenne "
           "par exercice tenté.\nUn thème devient point faible à partir de %d exercices "
           "tentés.%s\n", term(T_DIM), STATS_MIN_ATTEMPTED, term(T_RESET));

    stats_free(&st);
    close_session(&s);
    return TR_EXIT_OK;
}
