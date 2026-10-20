/* test_progress.c — tests de la progression, des statistiques, de la
 * recommandation et du catalogue (prérequis, cycles). */
#include <stdlib.h>
#include <unistd.h>

#include "catalog.h"
#include "error.h"
#include "fs.h"
#include "progress.h"
#include "stats.h"
#include "test.h"
#include "xalloc.h"

#define T1 "2026-01-01T10:00:00Z"
#define T2 "2026-01-02T10:00:00Z"

void test_progress_suite(void)
{
    progress p = PROGRESS_INIT;

    /* Transitions d'état */
    progress_apply(&p, "a", EVENT_START, 0, T1);
    CHECK_INT(progress_status(&p, "a"), STATUS_STARTED);
    progress_apply(&p, "a", EVENT_CHECK_BUILD_FAILED, 0, T1);
    progress_apply(&p, "a", EVENT_CHECK_FAILED, 0, T1);
    CHECK_INT(progress_status(&p, "a"), STATUS_FAILED);
    progress_apply(&p, "a", EVENT_HINT, 2, T1);
    progress_apply(&p, "a", EVENT_HINT, 1, T1); /* revoir l'indice 1 ne baisse pas */
    progress_apply(&p, "a", EVENT_CHECK_PASSED, 0, T2);
    progress_apply(&p, "a", EVENT_CHECK_FAILED, 0, T2); /* réussi le reste */
    const progress_entry *e = progress_find(&p, "a");
    CHECK(e != NULL);
    if (e) {
        CHECK_INT(e->status, STATUS_PASSED);
        CHECK_INT(e->attempts, 4);
        CHECK_INT(e->build_failures, 1);
        CHECK_INT(e->hints, 2);
        CHECK_STR(e->first, T1);
        CHECK_STR(e->passed_at, T2);
    }
    CHECK_INT(progress_status(&p, "inconnu"), STATUS_NEW);

    /* Aller-retour sur disque, champ inconnu conservé */
    char tmpl[] = "/tmp/trainer-prog-XXXXXX";
    char *dir = mkdtemp(tmpl);
    CHECK(dir != NULL);
    if (!dir) {
        progress_free(&p);
        return;
    }
    strbuf path = STRBUF_INIT, text = STRBUF_INIT;
    path_join(&path, dir, "sub/progress.txt"); /* le dossier sera créé */
    progress_apply(&p, "b", EVENT_SOLUTION, 0, T1);
    CHECK(progress_save(path.data, &p) == 0);
    progress_free(&p);

    CHECK(fs_read_file(path.data, &text) == 0);
    sb_append(&text, "c status=failed attempts=1 futur=42 box=3\n");
    CHECK(fs_write_file(path.data, text.data, text.len) == 0);
    CHECK(progress_load(path.data, &p) == 0);
    CHECK_INT(p.len, 3);
    e = progress_find(&p, "b");
    CHECK(e && e->solution_seen == 1);
    e = progress_find(&p, "c");
    CHECK(e && e->attempts == 1);
    if (e)
        CHECK_STR(e->extra, "futur=42 box=3");
    CHECK(progress_save(path.data, &p) == 0);
    progress_free(&p);
    sb_clear(&text);
    CHECK(fs_read_file(path.data, &text) == 0);
    CHECK(strstr(sb_cstr(&text), "futur=42 box=3") != NULL);
    CHECK(strstr(sb_cstr(&text), "# trainer-progress v1") == sb_cstr(&text));

    /* Transaction complète */
    CHECK(progress_record(path.data, "d", EVENT_CHECK_PASSED, 0) == 0);
    CHECK(progress_load(path.data, &p) == 0);
    CHECK_INT(progress_status(&p, "d"), STATUS_PASSED);
    CHECK_INT(p.len, 4);
    progress_free(&p);

    /* Fichiers invalides */
    CHECK(fs_write_file(path.data, "x status=bizarre\n", 17) == 0);
    CHECK(progress_load(path.data, &p) != 0);
    CHECK(strstr(err_msg(), ":1: valeur invalide") != NULL);
    CHECK(fs_write_file(path.data, "# trainer-progress v9\n", 22) == 0);
    CHECK(progress_load(path.data, &p) != 0);
    CHECK(strstr(err_msg(), "version plus récente") != NULL);
    CHECK(fs_write_file(path.data, "x\nx\n", 4) == 0);
    CHECK(progress_load(path.data, &p) != 0);

    /* Fichier absent = progression vide */
    path_join(&path, dir, "absent.txt");
    CHECK(progress_load(path.data, &p) == 0 && p.len == 0);

    CHECK(fs_remove_tree(dir) == 0);
    CHECK(!fs_exists(dir));
    sb_free(&path);
    sb_free(&text);
}

/* Ajoute un exercice minimal au catalogue (sans passer par le disque). */
static void add_exercise(catalog *cat, const char *id, const char *topic, int level,
                         const char *requires)
{
    strbuf text = STRBUF_INIT;
    sb_appendf(&text, "id: %s\ntitle: %s\nlang: cpp\nlevel: %d\ntopic: %s\nrequires: %s\n"
                      "## statement\nx\n## explanation\ny\n",
               id, id, level, topic, requires);
    cat->items = xrealloc(cat->items, (cat->len + 1) * sizeof *cat->items);
    CHECK(exercise_parse(text.data, id, &cat->items[cat->len]) == 0);
    cat->len++;
    sb_free(&text);
}

void test_stats_suite(void)
{
    catalog cat = CATALOG_INIT;
    add_exercise(&cat, "cpp.a", "basics", 1, "");
    add_exercise(&cat, "cpp.b", "basics", 1, "cpp.a");
    add_exercise(&cat, "cpp.c", "pointers", 1, "");
    add_exercise(&cat, "cpp.c2", "pointers", 1, "");
    add_exercise(&cat, "cpp.d", "pointers", 2, "cpp.c, cpp.inconnu");
    progress p = PROGRESS_INIT;
    recommendation r;

    /* Au départ : le premier exercice du catalogue */
    recommend_next(&cat, &p, NULL, &r);
    CHECK_INT(r.reason, RECO_NEXT);
    CHECK(r.ex && strcmp(r.ex->id, "cpp.a") == 0);
    CHECK(!exercise_unlocked(&cat, &p, &cat.items[1]));

    /* Exercice commencé : on le reprend */
    progress_apply(&p, "cpp.c", EVENT_CHECK_FAILED, 0, T1);
    recommend_next(&cat, &p, NULL, &r);
    CHECK_INT(r.reason, RECO_RESUME);
    CHECK(r.ex && strcmp(r.ex->id, "cpp.c") == 0);

    /* Réussi après beaucoup d'aide... */
    progress_apply(&p, "cpp.c", EVENT_HINT, 2, T1);
    progress_apply(&p, "cpp.c", EVENT_SOLUTION, 0, T1);
    progress_apply(&p, "cpp.c", EVENT_CHECK_PASSED, 0, T1);
    progress_apply(&p, "cpp.a", EVENT_CHECK_PASSED, 0, T1);
    stats s;
    stats_compute(&cat, &p, &s);
    CHECK_INT(s.total, 5);
    CHECK_INT(s.passed, 2);
    CHECK_INT(s.first_try, 1); /* cpp.a */
    CHECK_INT(s.len, 2);
    if (s.len == 2) {
        CHECK_STR(s.topics[1].topic, "pointers");
        /* 1 échec + 0,5 × 2 indices + 2 × solution = 4 */
        CHECK(s.topics[1].struggle > 3.99 && s.topics[1].struggle < 4.01);
        /* ... mais un seul exercice tenté : pas encore assez pour conclure */
        CHECK(!stats_topic_is_weak(&s.topics[1]));
        CHECK(s.topics[0].struggle < 0.01);
    }
    stats_free(&s);
    recommend_next(&cat, &p, NULL, &r);
    CHECK_INT(r.reason, RECO_NEXT);
    CHECK(r.ex && strcmp(r.ex->id, "cpp.b") == 0);

    /* Second exercice difficile du thème : « pointers » devient faible */
    progress_apply(&p, "cpp.c2", EVENT_CHECK_FAILED, 0, T1);
    progress_apply(&p, "cpp.c2", EVENT_CHECK_FAILED, 0, T1);
    progress_apply(&p, "cpp.c2", EVENT_CHECK_PASSED, 0, T2);
    stats_compute(&cat, &p, &s);
    if (s.len == 2) {
        CHECK(s.topics[1].struggle > 2.99 && s.topics[1].struggle < 3.01); /* (4 + 2) / 2 */
        CHECK(stats_topic_is_weak(&s.topics[1]));
    }
    stats_free(&s);

    /* cpp.b (niveau 1) passe avant cpp.d (niveau 2) malgré le thème faible ;
     * le prérequis inconnu de cpp.d est ignoré. */
    CHECK(exercise_unlocked(&cat, &p, &cat.items[4]));
    recommend_next(&cat, &p, NULL, &r);
    CHECK(r.ex && strcmp(r.ex->id, "cpp.b") == 0);
    progress_apply(&p, "cpp.b", EVENT_CHECK_PASSED, 0, T2);
    recommend_next(&cat, &p, NULL, &r);
    CHECK_INT(r.reason, RECO_WEAK_TOPIC);
    CHECK(r.ex && strcmp(r.ex->id, "cpp.d") == 0);
    progress_apply(&p, "cpp.d", EVENT_CHECK_PASSED, 0, T2);
    recommend_next(&cat, &p, NULL, &r);
    CHECK_INT(r.reason, RECO_ALL_DONE);
    recommend_next(&cat, &p, "rust", &r);
    CHECK_INT(r.reason, RECO_ALL_DONE);

    progress_free(&p);
    catalog_free(&cat);
}

static void write_exercise(const char *root, const char *name, const char *id,
                           const char *requires)
{
    strbuf dir = STRBUF_INIT, path = STRBUF_INIT, text = STRBUF_INIT;
    path_join(&dir, root, name);
    CHECK(fs_mkdir_p(dir.data) == 0);
    path_join(&path, dir.data, "exercise.txt");
    sb_appendf(&text, "id: %s\ntitle: t\nlang: cpp\nlevel: 1\ntopic: t\nrequires: %s\n"
                      "## statement\nx\n## explanation\ny\n", id, requires);
    CHECK(fs_write_file(path.data, text.data, text.len) == 0);
    sb_free(&dir);
    sb_free(&path);
    sb_free(&text);
}

void test_catalog_suite(void)
{
    strbuf root = STRBUF_INIT;
    CHECK(fs_make_temp_dir("trainer-cat", &root) == 0);
    write_exercise(root.data, "02-b", "cpp.b", "cpp.c");
    write_exercise(root.data, "01-a", "cpp.a", "");
    write_exercise(root.data, "03-c", "cpp.c", "cpp.b");     /* cycle b <-> c */
    write_exercise(root.data, "04-dup", "cpp.a", "");        /* id en double */
    write_exercise(root.data, "05-d", "cpp.d", "cpp.absent");

    catalog cat;
    CHECK(catalog_load(root.data, &cat) == 0);
    CHECK_INT(cat.len, 4);
    if (cat.len == 4)
        CHECK_STR(cat.items[0].id, "cpp.a"); /* trié par dossier */
    int dup = 0, unknown = 0, cycle = 0;
    for (size_t i = 0; i < cat.errors.len; i++) {
        dup |= strstr(cat.errors.items[i], "déjà utilisé") != NULL;
        unknown |= strstr(cat.errors.items[i], "prérequis inconnu « cpp.absent »") != NULL;
        cycle |= strstr(cat.errors.items[i], "cycle de prérequis : cpp.b -> cpp.c -> cpp.b") != NULL;
    }
    CHECK(dup);
    CHECK(unknown);
    CHECK(cycle);
    catalog_free(&cat);

    /* fs_remove_tree ne suit pas les liens symboliques */
    strbuf outside = STRBUF_INIT, link = STRBUF_INIT, keep = STRBUF_INIT;
    CHECK(fs_make_temp_dir("trainer-outside", &outside) == 0);
    path_join(&keep, outside.data, "garde-moi.txt");
    CHECK(fs_write_file(keep.data, "x", 1) == 0);
    path_join(&link, root.data, "lien");
    CHECK(symlink(outside.data, link.data) == 0);
    CHECK(fs_remove_tree(root.data) == 0);
    CHECK(!fs_exists(root.data));
    CHECK(fs_is_file(keep.data)); /* toujours là */
    CHECK(fs_remove_tree(outside.data) == 0);
    sb_free(&root);
    sb_free(&outside);
    sb_free(&link);
    sb_free(&keep);
}
