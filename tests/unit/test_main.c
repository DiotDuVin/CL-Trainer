/* test_main.c — lance toutes les suites de tests unitaires. */
#include <stdio.h>

#include "test.h"

int test_checks = 0;
int test_failures = 0;

int main(void)
{
    static const struct {
        const char *name;
        void (*fn)(void);
    } suites[] = {
        { "strbuf", test_strbuf_suite },     { "text", test_text_suite },
        { "fs", test_fs_suite },             { "exercise", test_exercise_suite },
        { "testspec", test_testspec_suite }, { "compare", test_compare_suite },
        { "process", test_process_suite },   { "progress", test_progress_suite },
        { "stats", test_stats_suite },       { "catalog", test_catalog_suite },
        { "json", test_json_suite },
    };
    for (size_t i = 0; i < sizeof suites / sizeof suites[0]; i++) {
        int before = test_failures;
        suites[i].fn();
        printf("  %-10s %s\n", suites[i].name, test_failures == before ? "ok" : "ÉCHEC");
    }
    printf("%d vérifications, %d échec(s)\n", test_checks, test_failures);
    return test_failures == 0 ? 0 : 1;
}
