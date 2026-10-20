/*
 * check.h — vérification d'un exercice : compilation puis exécution de tous
 * les tests. Le résultat est un rapport (structure), sans aucun affichage :
 * le terminal (src/cli/cmd_check.c) et l'application graphique le
 * présentent chacun à leur manière.
 */
#ifndef TRAINER_CHECK_H
#define TRAINER_CHECK_H

#include <stddef.h>

#include "exercise.h"
#include "process.h"
#include "testspec.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Verdict d'un test, du plus « grave » au réussi. L'ordre de détection
 * compte : un programme tué par un délai a forcément une sortie incomplète,
 * on signale donc la cause (le délai) et pas le symptôme. */
typedef enum {
    VERDICT_PASS,
    VERDICT_TIMEOUT,          /* délai dépassé */
    VERDICT_TOO_MUCH_OUTPUT,  /* sortie > 1 Mo */
    VERDICT_SANITIZER,        /* rapport AddressSanitizer / UBSan */
    VERDICT_CRASH,            /* tué par un signal (SIGSEGV...) */
    VERDICT_WRONG_OUTPUT,
    VERDICT_WRONG_EXIT,
} verdict;

typedef struct {
    verdict result;           /* (pas « verdict verdict » : invalide en C++) */
    proc_result res;          /* sorties capturées, durée, code de retour */
} test_outcome;

/* État global de la vérification */
typedef enum {
    CHECK_PASSED,             /* compilation OK et tous les tests passent */
    CHECK_TESTS_FAILED,       /* compilation OK, au moins un test échoue */
    CHECK_BUILD_FAILED,       /* erreur de compilation dans le code de l'élève */
} check_status;

typedef struct {
    check_status status;
    char *build_log;          /* messages du compilateur ("" si aucun) */
    test_suite tests;         /* les cas de test (tests.txt) */
    test_outcome *outcomes;   /* un résultat par test, si la compilation a réussi */
    size_t passed;            /* nombre de tests réussis */
} check_report;

/* Compile le code de workdir et exécute les tests de l'exercice.
 * 0 si la vérification a pu avoir lieu (quel que soit le résultat de
 * l'élève) ; -1 en cas de problème d'environnement (compilateur absent,
 * tests.txt invalide...), avec le message dans err_msg().
 * rep doit être libéré avec check_report_free dans tous les cas. */
int check_run(const exercise *ex, const char *workdir, check_report *rep);
void check_report_free(check_report *rep);

/* Libellé court : "réussi", "temps dépassé"... */
const char *verdict_label(verdict v);

#ifdef __cplusplus
}
#endif

#endif
