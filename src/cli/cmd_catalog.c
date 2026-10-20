/* cmd_catalog.c — sous-commandes list et show. */
#include <stdio.h>
#include <string.h>

#include "cli.h"
#include "json.h"
#include "term.h"

static void list_json(const catalog *cat, const progress *p, const char *lang,
                      const char *topic)
{
    strbuf out = STRBUF_INIT;
    json_writer w;
    json_init(&w, &out);
    json_begin_array(&w);
    for (size_t i = 0; i < cat->len; i++) {
        const exercise *ex = &cat->items[i];
        if ((lang && strcmp(ex->lang, lang) != 0) || (topic && strcmp(ex->topic, topic) != 0))
            continue;
        json_begin_object(&w);
        json_key(&w, "id");       json_string(&w, ex->id);
        json_key(&w, "title");    json_string(&w, ex->title);
        json_key(&w, "lang");     json_string(&w, ex->lang);
        json_key(&w, "topic");    json_string(&w, ex->topic);
        json_key(&w, "level");    json_int(&w, ex->level);
        json_key(&w, "status");   json_string(&w, progress_status_name(progress_status(p, ex->id)));
        json_key(&w, "unlocked"); json_bool(&w, exercise_unlocked(cat, p, ex));
        json_key(&w, "kind");     json_string(&w, "exercise");
        json_key(&w, "lesson");   json_string(&w, ex->lesson);
        json_end(&w);
    }
    for (size_t i = 0; i < cat->nlessons; i++) {
        const lesson *l = &cat->lessons[i];
        if ((lang && strcmp(l->lang, lang) != 0) || (topic && strcmp(l->topic, topic) != 0))
            continue;
        json_begin_object(&w);
        json_key(&w, "id");     json_string(&w, l->id);
        json_key(&w, "title");  json_string(&w, l->title);
        json_key(&w, "lang");   json_string(&w, l->lang);
        json_key(&w, "topic");  json_string(&w, l->topic);
        json_key(&w, "status"); json_string(&w, progress_status(p, l->id) == STATUS_PASSED
                                                    ? "read" : "unread");
        json_key(&w, "kind");   json_string(&w, "lesson");
        json_end(&w);
    }
    json_end(&w);
    puts(sb_cstr(&out));
    sb_free(&out);
}

int cmd_list(const trainer_config *cfg, int argc, char **argv)
{
    const char *lang = NULL, *topic = NULL;
    int json = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--lang") == 0 && i + 1 < argc)
            lang = argv[++i];
        else if (strcmp(argv[i], "--topic") == 0 && i + 1 < argc)
            topic = argv[++i];
        else if (strcmp(argv[i], "--json") == 0)
            json = 1;
        else {
            fprintf(stderr, "usage : trainer list [--lang LANG] [--topic THÈME] [--json]\n");
            return TR_EXIT_ERROR;
        }
    }

    catalog cat;
    int rc = cli_load_catalog(cfg, &cat);
    if (rc != TR_EXIT_OK)
        return rc;
    progress p;
    cli_load_progress(cfg, &p);

    if (json) {
        list_json(&cat, &p, lang, topic);
        goto out;
    }

    const char *current_lang = "";
    size_t shown = 0, j = 0;
    for (size_t i = 0; i <= cat.len; i++) {
        const exercise *ex = i < cat.len ? &cat.items[i] : NULL;
        /* Les cours s'intercalent avant les exercices de leur chapitre : leur
         * dossier est un préfixe de celui des exercices, donc trié avant. */
        for (; j < cat.nlessons && (!ex || strcmp(cat.lessons[j].dir, ex->dir) < 0); j++) {
            const lesson *l = &cat.lessons[j];
            if ((lang && strcmp(l->lang, lang) != 0) || (topic && strcmp(l->topic, topic) != 0))
                continue;
            if (strcmp(l->lang, current_lang) != 0) {
                printf("%s%s%s%s\n", shown ? "\n" : "", term(T_BOLD), lang_name(l->lang),
                       term(T_RESET));
                current_lang = l->lang;
            }
            int read = progress_status(&p, l->id) == STATUS_PASSED;
            printf("  %s📖%s %-28s %sCours : %s%s%s\n", read ? "" : term(T_CYAN),
                   term(T_RESET), l->id, term(T_BOLD), l->title, term(T_RESET),
                   read ? " ✓" : "");
            shown++;
        }
        if (!ex)
            break;
        if ((lang && strcmp(ex->lang, lang) != 0) || (topic && strcmp(ex->topic, topic) != 0))
            continue;
        if (strcmp(ex->lang, current_lang) != 0) { /* nouveau groupe */
            printf("%s%s%s%s\n", shown ? "\n" : "", term(T_BOLD), lang_name(ex->lang),
                   term(T_RESET));
            current_lang = ex->lang;
        }
        int unlocked = exercise_unlocked(&cat, &p, ex);
        printf("  ");
        print_status_symbol(progress_status(&p, ex->id), unlocked);
        printf(" %-28s ", ex->id);
        print_level(ex->level);
        printf("  %s%s %s(%s)%s\n", unlocked ? "" : term(T_DIM), ex->title, term(T_DIM),
               ex->topic, term(T_RESET));
        shown++;
    }
    if (shown == 0)
        printf("Aucun exercice trouvé dans %s.\n", cfg->exercises_dir);
    else
        printf("\n%s📖 cours  ✓ réussi  ✗ à reprendre  ● commencé  ○ nouveau  – prérequis "
               "manquants%s\n"
               "Détails : trainer show <id>   Cours : trainer learn <id>   Suggestion : "
               "trainer next\n",
               term(T_DIM), term(T_RESET));
out:
    progress_free(&p);
    catalog_free(&cat);
    return TR_EXIT_OK;
}

int cmd_show(const trainer_config *cfg, int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage : trainer show <id>\n");
        return TR_EXIT_ERROR;
    }
    catalog cat;
    int rc = cli_load_catalog(cfg, &cat);
    if (rc != TR_EXIT_OK)
        return rc;
    progress p;
    cli_load_progress(cfg, &p);
    const lesson *lsn = catalog_find_lesson(&cat, argv[1]);
    if (lsn) { /* « show » sur un cours : l'afficher sans le marquer comme lu */
        print_lesson(lsn, &cat, &p);
        printf("\n%sMarquer comme lu : trainer learn %s%s\n", term(T_DIM), lsn->id,
               term(T_RESET));
        progress_free(&p);
        catalog_free(&cat);
        return TR_EXIT_OK;
    }
    const exercise *ex = cli_find(&cat, argv[1]);
    if (!ex) {
        progress_free(&p);
        catalog_free(&cat);
        return TR_EXIT_ERROR;
    }

    printf("%s%s%s\n", term(T_BOLD), ex->title, term(T_RESET));
    printf("%s%s · %s · niveau %d/5 · thème %s", term(T_DIM), ex->id, lang_name(ex->lang),
           ex->level, ex->topic);
    if (ex->estimated_min > 0)
        printf(" · ~%d min", ex->estimated_min);
    printf("%s\n", term(T_RESET));
    if (ex->prereqs.len > 0) {
        printf("%sPrérequis :", term(T_DIM));
        for (size_t i = 0; i < ex->prereqs.len; i++) {
            const char *req = ex->prereqs.items[i];
            printf("%s %s%s", i ? "," : "", req,
                   progress_status(&p, req) == STATUS_PASSED ? " ✓" : "");
        }
        printf("%s\n", term(T_RESET));
    }
    if (ex->lesson && progress_status(&p, ex->lesson) != STATUS_PASSED)
        printf("%s📖 Cours associé, pas encore lu : trainer learn %s%s\n", term(T_YELLOW),
               ex->lesson, term(T_RESET));
    printf("\n%s\n\n", ex->statement);

    const progress_entry *e = progress_find(&p, ex->id);
    if (ex->hints.len > 0)
        printf("%sIndices : %d/%zu révélé(s) — trainer hint %s%s\n", term(T_DIM),
               e ? e->hints : 0, ex->hints.len, ex->id, term(T_RESET));
    if (workspace_exists(cfg, ex))
        printf("Vérifier : %strainer check %s%s\n", term(T_CYAN), ex->id, term(T_RESET));
    else
        printf("Pour commencer : %strainer start %s%s\n", term(T_CYAN), ex->id, term(T_RESET));

    progress_free(&p);
    catalog_free(&cat);
    return TR_EXIT_OK;
}
