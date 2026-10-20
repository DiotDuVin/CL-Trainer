/*
 * test.h — mini-framework de tests unitaires.
 *
 * Volontairement minimal (quelques macros) : pas de dépendance externe, et
 * on voit exactement ce qui se passe. Les frameworks comme Unity, Check ou
 * Criterion (C), GoogleTest (C++), xUnit (C#) suivent le même principe :
 * compter les vérifications, signaler les échecs avec fichier et ligne.
 */
#ifndef TRAINER_TEST_H
#define TRAINER_TEST_H

#include <stdio.h>
#include <string.h>

extern int test_checks;
extern int test_failures;

#define CHECK(cond)                                                            \
    do {                                                                       \
        test_checks++;                                                         \
        if (!(cond)) {                                                         \
            test_failures++;                                                   \
            fprintf(stderr, "%s:%d: échec : %s\n", __FILE__, __LINE__, #cond); \
        }                                                                      \
    } while (0)

#define CHECK_STR(actual, expected)                                            \
    do {                                                                       \
        const char *a_ = (actual), *e_ = (expected);                           \
        test_checks++;                                                         \
        if (!a_ || !e_ || strcmp(a_, e_) != 0) {                               \
            test_failures++;                                                   \
            fprintf(stderr, "%s:%d: %s\n    obtenu : «%s»\n    attendu: «%s»\n", \
                    __FILE__, __LINE__, #actual, a_ ? a_ : "(null)",           \
                    e_ ? e_ : "(null)");                                       \
        }                                                                      \
    } while (0)

#define CHECK_INT(actual, expected)                                            \
    do {                                                                       \
        long a_ = (long)(actual), e_ = (long)(expected);                       \
        test_checks++;                                                         \
        if (a_ != e_) {                                                        \
            test_failures++;                                                   \
            fprintf(stderr, "%s:%d: %s = %ld, attendu %ld\n", __FILE__,        \
                    __LINE__, #actual, a_, e_);                                \
        }                                                                      \
    } while (0)

/* Une suite par module testé (définies dans test_<module>.c). */
void test_strbuf_suite(void);
void test_text_suite(void);
void test_fs_suite(void);
void test_exercise_suite(void);
void test_testspec_suite(void);
void test_compare_suite(void);
void test_process_suite(void);
void test_progress_suite(void);
void test_stats_suite(void);
void test_catalog_suite(void);
void test_json_suite(void);
void test_lesson_suite(void);

#endif
