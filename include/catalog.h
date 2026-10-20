/*
 * catalog.h — ensemble des exercices disponibles.
 *
 * Tout dossier contenant un fichier exercise.txt est un exercice, quelle que
 * soit sa profondeur sous la racine (exercises/cpp/01-basics/hello/...).
 * Les exercices sont triés par chemin : les préfixes numériques des dossiers
 * (01-basics, 02-pointers...) donnent l'ordre de la progression.
 */
#ifndef TRAINER_CATALOG_H
#define TRAINER_CATALOG_H

#include <stddef.h>

#include "exercise.h"
#include "strvec.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    exercise *items;
    size_t len;
    size_t cap;
    /* Problèmes rencontrés (exercice invalide ignoré, id en double,
     * prérequis inconnu...) : non bloquants pour l'utilisateur, mais
     * bloquants pour la validation. */
    strvec errors;
} catalog;

#define CATALOG_INIT { NULL, 0, 0, STRVEC_INIT }

/* Charge tous les exercices sous root. -1 seulement si root est illisible ;
 * les exercices invalides sont ignorés et décrits dans cat->errors. */
int catalog_load(const char *root, catalog *cat);

/* Exercice d'id donné, ou NULL. */
const exercise *catalog_find(const catalog *cat, const char *id);

void catalog_free(catalog *cat);

#ifdef __cplusplus
}
#endif

#endif
