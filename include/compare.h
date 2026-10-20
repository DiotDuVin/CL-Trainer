/*
 * compare.h — comparaison de la sortie du programme avec la sortie attendue.
 *
 * Mode normalized (par défaut) : on ignore les différences invisibles qui ne
 * relèvent pas de l'exercice : "\r\n" contre "\n", espaces en fin de ligne,
 * lignes vides finales. Mode exact : comparaison octet par octet.
 *
 * Ce module ne fait aucun affichage : il décrit la différence dans une
 * structure, que le terminal ou l'application graphique présentent à leur
 * façon.
 */
#ifndef TRAINER_COMPARE_H
#define TRAINER_COMPARE_H

#include "exercise.h"
#include "strbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Première différence entre deux sorties. */
typedef struct {
    int empty_output;     /* 1 : la sortie obtenue est vide */
    int same_lines;       /* 1 : lignes identiques, seules les fins de ligne
                             diffèrent (possible en mode exact) */
    int line;             /* numéro (1..n) de la première ligne différente */
    char *expected_line;  /* NULL si la sortie attendue s'arrête avant */
    char *actual_line;    /* NULL si la sortie obtenue s'arrête avant */
} output_diff;

/* Ajoute à out la version normalisée de s : chaque ligne sans blancs
 * finaux et terminée par '\n', sans lignes vides à la fin. */
void compare_normalize(const char *s, strbuf *out);

/* 1 si actual correspond à expected selon le mode, 0 sinon. */
int compare_outputs(const char *expected, const char *actual, compare_mode mode);

/* Décrit la première différence (à n'appeler que si les sorties diffèrent).
 * Libérer d avec output_diff_free. */
void compare_first_diff(const char *expected, const char *actual, compare_mode mode,
                        output_diff *d);
void output_diff_free(output_diff *d);

#ifdef __cplusplus
}
#endif

#endif
