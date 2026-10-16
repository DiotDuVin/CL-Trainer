/*
 * compare.h — comparaison de la sortie du programme avec la sortie attendue.
 *
 * Mode normalized (par défaut) : on ignore les différences invisibles qui ne
 * relèvent pas de l'exercice : "\r\n" contre "\n", espaces en fin de ligne,
 * lignes vides finales. Mode exact : comparaison octet par octet.
 */
#ifndef TRAINER_COMPARE_H
#define TRAINER_COMPARE_H

#include <stdio.h>

#include "exercise.h"
#include "strbuf.h"

/* Ajoute à out la version normalisée de s : chaque ligne sans blancs
 * finaux et terminée par '\n', sans lignes vides à la fin. */
void compare_normalize(const char *s, strbuf *out);

/* 1 si actual correspond à expected selon le mode, 0 sinon. */
int compare_outputs(const char *expected, const char *actual, compare_mode mode);

/* Affiche la première ligne qui diffère, de façon lisible (caractères
 * invisibles rendus visibles). */
void compare_print_diff(FILE *f, const char *expected, const char *actual,
                        compare_mode mode);

#endif
