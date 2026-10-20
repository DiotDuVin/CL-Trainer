/* test_parsers.c — tests des parsers exercise.txt / tests.txt et de compare. */
#include "compare.h"
#include "json.h"
#include "lesson.h"
#include "error.h"
#include "exercise.h"
#include "test.h"
#include "testspec.h"

static const char *const VALID_EXERCISE =
    "id: cpp.basics.hello\n"
    "title: Bonjour\n"
    "lang: cpp\n"
    "level: 1\n"
    "topic: basics\n"
    "requires: cpp.a, cpp.b\n"
    "timeout_ms: 500\n"
    "\n"
    "## statement\n"
    "\n"
    "Écris un programme.\n"
    "### sous-titre (reste du texte)\n"
    "\n"
    "## hints\n"
    "Premier indice.\n"
    "---\n"
    "Second\n"
    "indice.\n"
    "## explanation\n"
    "Parce que.\n";

/* Vérifie qu'un exercice est rejeté et que le message contient `needle`. */
static void expect_exercise_error(const char *text, const char *needle)
{
    exercise ex;
    int rc = exercise_parse(text, "t.txt", &ex);
    CHECK(rc != 0);
    if (rc == 0) {
        exercise_free(&ex);
        return;
    }
    if (!strstr(err_msg(), needle)) {
        test_failures++;
        fprintf(stderr, "message inattendu : «%s» (attendu : «%s»)\n", err_msg(), needle);
    }
}

void test_exercise_suite(void)
{
    exercise ex;
    CHECK(exercise_parse(VALID_EXERCISE, "t.txt", &ex) == 0);
    CHECK_STR(ex.id, "cpp.basics.hello");
    CHECK_INT(ex.level, 1);
    CHECK_INT(ex.timeout_ms, 500);
    CHECK_INT(ex.prereqs.len, 2);
    CHECK_STR(ex.prereqs.items[1], "cpp.b");
    CHECK_STR(ex.statement, "Écris un programme.\n### sous-titre (reste du texte)");
    CHECK_INT(ex.hints.len, 2);
    CHECK_STR(ex.hints.items[1], "Second\nindice.");
    CHECK_STR(ex.explanation, "Parce que.");
    CHECK(ex.common_mistakes == NULL);
    CHECK(ex.compare == COMPARE_NORMALIZED);
    exercise_free(&ex);

    expect_exercise_error("id: cpp.x\ntittle: x\n", "t.txt:2: clé inconnue « tittle »");
    expect_exercise_error("id: cpp.x\nid: cpp.y\n", "t.txt:2: clé « id » en double");
    expect_exercise_error("id: Cpp.X\n", "id invalide");
    expect_exercise_error("level: 9\n", "level doit être");
    expect_exercise_error("files: ../etc/passwd\n", "chemin relatif");
    expect_exercise_error("pas de deux-points\n", "ligne d'en-tête invalide");
    expect_exercise_error("id: cpp.x\n## enonce\n", "section inconnue « enonce »");
    expect_exercise_error("id: cpp.x\ntitle: t\nlang: cpp\nlevel: 1\n", "« topic »");
    expect_exercise_error("id: rust.x\ntitle: t\nlang: cpp\nlevel: 1\ntopic: t\n",
                          "doit commencer par « cpp. »");
    expect_exercise_error("id: cpp.x\ntitle: t\nlang: cpp\nlevel: 1\ntopic: t\n"
                          "## statement\nx\n",
                          "« explanation » manquante");
}

void test_testspec_suite(void)
{
    const char *text =
        "=== test: simple\n"
        "--- stdin\n"
        "3 7\n"
        "--- stdout\n"
        "7 3\n"
        "\n"
        "\n"
        "=== test: caché [hidden]\n"
        "--- args\n"
        "--verbose\n"
        "deux mots\n"
        "--- stdout\n"
        "--- exit\n"
        "3\n"
        "=== test:\n"
        "--- stdout\n"
        "  indentation conservée\n";
    test_suite ts;
    CHECK(testspec_parse(text, "t", &ts) == 0);
    CHECK_INT(ts.len, 3);
    if (ts.len == 3) {
        CHECK_STR(ts.items[0].name, "simple");
        CHECK_STR(ts.items[0].input, "3 7\n");
        CHECK_STR(ts.items[0].expected, "7 3\n"); /* lignes vides finales retirées */
        CHECK(!ts.items[0].hidden);
        CHECK_STR(ts.items[1].name, "caché");
        CHECK(ts.items[1].hidden);
        CHECK_STR(ts.items[1].input, "");
        CHECK_STR(ts.items[1].expected, "");
        CHECK_INT(ts.items[1].args.len, 2);
        CHECK_STR(ts.items[1].args.items[1], "deux mots");
        CHECK_INT(ts.items[1].expected_exit, 3);
        CHECK_STR(ts.items[2].name, "test 3");
        CHECK_STR(ts.items[2].expected, "  indentation conservée\n");
    }
    testspec_free(&ts);

    CHECK(testspec_parse("", "t", &ts) != 0);
    CHECK(strstr(err_msg(), "aucun test") != NULL);
    CHECK(testspec_parse("=== test: a\n--- stdin\nx\n", "t", &ts) != 0);
    CHECK(strstr(err_msg(), "pas de bloc « --- stdout »") != NULL);
    CHECK(testspec_parse("=== test: a\n--- stdout\n--- stdout\n", "t", &ts) != 0);
    CHECK(strstr(err_msg(), "t:3: bloc « stdout » en double") != NULL);
    CHECK(testspec_parse("=== test: a\n--- exit\nabc\n--- stdout\n", "t", &ts) != 0);
    CHECK(strstr(err_msg(), "t:2: le bloc exit") != NULL);
    CHECK(testspec_parse("bonjour\n", "t", &ts) != 0);
    CHECK(testspec_parse("=== mauvais\n", "t", &ts) != 0);
}

void test_compare_suite(void)
{
    strbuf sb = STRBUF_INIT;
    compare_normalize("a  \r\nb\t\n\n\n", &sb);
    CHECK_STR(sb_cstr(&sb), "a\nb\n");
    sb_clear(&sb);
    compare_normalize("\n\n", &sb);
    CHECK_STR(sb_cstr(&sb), "");
    sb_clear(&sb);
    compare_normalize("\nx", &sb); /* les lignes vides initiales comptent */
    CHECK_STR(sb_cstr(&sb), "\nx\n");
    sb_free(&sb);

    CHECK(compare_outputs("7 3\n", "7 3", COMPARE_NORMALIZED));
    CHECK(compare_outputs("7 3\n", "7 3  \r\n\n", COMPARE_NORMALIZED));
    CHECK(!compare_outputs("7 3\n", "7  3\n", COMPARE_NORMALIZED)); /* espace interne */
    CHECK(!compare_outputs("7 3\n", "7 3", COMPARE_EXACT));
    CHECK(compare_outputs("7 3\n", "7 3\n", COMPARE_EXACT));
    CHECK(compare_outputs("", "", COMPARE_NORMALIZED));
    CHECK(!compare_outputs("x\n", "", COMPARE_NORMALIZED));
}

void test_json_suite(void)
{
    strbuf out = STRBUF_INIT;
    json_writer w;
    json_init(&w, &out);
    json_begin_object(&w);
    json_key(&w, "id");    json_string(&w, "a\"b");
    json_key(&w, "n");     json_int(&w, -3);
    json_key(&w, "liste");
    json_begin_array(&w);
    json_bool(&w, 1);
    json_string(&w, NULL);
    json_begin_object(&w);
    json_end(&w);
    json_end(&w);
    json_end(&w);
    CHECK_STR(sb_cstr(&out), "{\"id\":\"a\\\"b\",\"n\":-3,\"liste\":[true,null,{}]}");
    sb_free(&out);

    /* Première différence entre deux sorties */
    output_diff d;
    compare_first_diff("a\nb\nc\n", "a\nB\n", COMPARE_NORMALIZED, &d);
    CHECK_INT(d.line, 2);
    CHECK_STR(d.expected_line, "b");
    CHECK_STR(d.actual_line, "B");
    output_diff_free(&d);
    compare_first_diff("a\nb\n", "a\n", COMPARE_NORMALIZED, &d);
    CHECK_INT(d.line, 2);
    CHECK(d.actual_line == NULL); /* la sortie s'arrête avant */
    output_diff_free(&d);
    compare_first_diff("a\n", "", COMPARE_NORMALIZED, &d);
    CHECK(d.empty_output);
    output_diff_free(&d);
    compare_first_diff("a\n", "a", COMPARE_EXACT, &d);
    CHECK(d.same_lines);
    output_diff_free(&d);
}

void test_lesson_suite(void)
{
    const char *text =
        "id: cpp.lesson.x\n"
        "title: Cours X\n"
        "lang: cpp\n"
        "topic: t\n"
        "estimated_min: 5\n"
        "---\n"
        "\n"
        "## Titre\n"
        "```cpp example\n"
        "int main() {}\n"
        "```\n"
        "\n"
        "```output\n"
        "rien\n"
        "```\n"
        "```cpp\n"
        "fragment(); // pas un exemple : non exécuté\n"
        "```\n"
        "```cpp example\n"
        "int main() { return 0; }\n"
        "```\n";
    lesson l;
    CHECK(lesson_parse(text, "l.md", &l) == 0);
    CHECK_STR(l.id, "cpp.lesson.x");
    CHECK_INT(l.estimated_min, 5);
    CHECK_INT(l.body_line, 8);
    CHECK(strncmp(l.body, "## Titre", 8) == 0);
    lesson_examples ex;
    lesson_extract_examples(&l, &ex);
    CHECK_INT(ex.len, 2);
    if (ex.len == 2) {
        CHECK_STR(ex.items[0].code, "int main() {}\n");
        CHECK_STR(ex.items[0].expected, "rien\n");
        CHECK_INT(ex.items[0].line, 9); /* ligne du fichier, pas du corps */
        CHECK(ex.items[1].expected == NULL);
    }
    lesson_examples_free(&ex);
    lesson_free(&l);

    CHECK(lesson_parse("id: cpp.lesson.x\ntitle: t\nlang: cpp\ntopic: t\n## pas de séparateur\n",
                       "l.md", &l) != 0);
    CHECK(lesson_parse("id: cpp.x\ntitle: t\nlang: cpp\ntopic: t\n---\n", "l.md", &l) != 0);
    CHECK(strstr(err_msg(), "vide") != NULL);
    CHECK(lesson_parse("id: rust.x\ntitle: t\nlang: cpp\ntopic: t\n---\nx\n", "l.md", &l) != 0);
    CHECK(lesson_parse("idd: cpp.x\n---\nx\n", "l.md", &l) != 0);
    CHECK(strstr(err_msg(), "clé inconnue") != NULL);
}
