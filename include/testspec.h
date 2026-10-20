/*
 * testspec.h — cas de test d'un exercice (fichier tests.txt).
 *
 *     === test: cas simple            <- début d'un test ([hidden] optionnel)
 *     --- stdin                       <- blocs : stdin, stdout, args, exit
 *     3 7
 *     --- stdout
 *     7 3
 *
 * Contenu d'un bloc stdin/stdout : chaque ligne est terminée par '\n', les
 * lignes vides finales sont retirées (elles servent de séparation visuelle).
 */
#ifndef TRAINER_TESTSPEC_H
#define TRAINER_TESTSPEC_H

#include <stddef.h>

#include "strvec.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *name;
    int hidden;          /* 1 : entrée et sortie attendue non affichées */
    int line;            /* ligne du "=== test:" dans tests.txt */
    char *input;         /* contenu de stdin ("" si pas de bloc) */
    char *expected;      /* sortie standard attendue */
    strvec args;         /* arguments de la ligne de commande, un par ligne */
    int expected_exit;   /* code de retour attendu (0 par défaut) */
} test_case;

typedef struct {
    test_case *items;
    size_t len;
    size_t cap;
} test_suite;

#define TEST_SUITE_INIT { NULL, 0, 0 }

/* Analyse le contenu d'un tests.txt. 0 / -1 ; en cas d'échec *ts est libéré. */
int testspec_parse(const char *text, const char *origin, test_suite *ts);
/* Charge un fichier tests.txt. 0 / -1. */
int testspec_load(const char *path, test_suite *ts);
void testspec_free(test_suite *ts);

#ifdef __cplusplus
}
#endif

#endif
