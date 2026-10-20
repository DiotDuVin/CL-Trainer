/*
 * catalog.h — ensemble des exercices disponibles.
 *
 * Tout dossier contenant un fichier exercise.txt est un exercice, quelle que
 * soit sa profondeur sous la racine (exercises/cpp/01-basics/hello/...).
 * Un dossier contenant lesson.md est un chapitre : son cours est rattaché à
 * tous les exercices qu'il contient (champ exercise.lesson).
 * Les exercices sont triés par chemin : les préfixes numériques des dossiers
 * (01-basics, 02-pointers...) donnent l'ordre de la progression.
 */
#ifndef TRAINER_CATALOG_H
#define TRAINER_CATALOG_H

#include <stddef.h>

#include "exercise.h"
#include "lesson.h"
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
    lesson *lessons;      /* cours des chapitres, triés par dossier */
    size_t nlessons;
    size_t lessons_cap;
} catalog;

#define CATALOG_INIT { NULL, 0, 0, STRVEC_INIT, NULL, 0, 0 }

/* Charge tous les exercices sous root. -1 seulement si root est illisible ;
 * les exercices invalides sont ignorés et décrits dans cat->errors. */
int catalog_load(const char *root, catalog *cat);

/* Exercice d'id donné, ou NULL. */
const exercise *catalog_find(const catalog *cat, const char *id);
/* Cours d'id donné, ou NULL. */
const lesson *catalog_find_lesson(const catalog *cat, const char *id);

void catalog_free(catalog *cat);

#ifdef __cplusplus
}
#endif

#endif
