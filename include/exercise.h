/*
 * exercise.h — description d'un exercice (fichier exercise.txt).
 *
 * Format complet : docs/exercise-format.md. En résumé :
 *
 *     id: cpp.basics.hello          <- en-tête « clé: valeur »
 *     title: Bonjour
 *     ...
 *     ## statement                  <- sections « ## nom »
 *     Texte de l'énoncé...
 */
#ifndef TRAINER_EXERCISE_H
#define TRAINER_EXERCISE_H

#include "strvec.h"

typedef enum { COMPARE_NORMALIZED, COMPARE_EXACT } compare_mode;
typedef enum { KIND_EXERCISE, KIND_PROJECT } exercise_kind;

typedef struct {
    /* En-tête */
    char *id;               /* "cpp.basics.hello", commence par "<lang>." */
    char *title;
    char *lang;             /* "cpp", "git", "csharp", "rust" */
    char *topic;            /* thème, pour les statistiques */
    int level;              /* 1 (débutant) à 5 (professionnel) */
    exercise_kind kind;
    strvec tags;
    strvec requires;        /* ids des prérequis */
    strvec files;           /* fichiers édités par l'élève ; vide = défaut du runner */
    long timeout_ms;        /* délai par test */
    int estimated_min;      /* 0 = non renseigné */
    compare_mode compare;
    int sanitize;           /* 1 = compiler avec AddressSanitizer/UBSan */

    /* Sections */
    char *statement;
    char *explanation;
    char *common_mistakes;  /* NULL si absente */
    strvec hints;           /* indices progressifs */

    char *dir;              /* dossier de l'exercice (NULL si parsé depuis un texte) */
} exercise;

#define EXERCISE_DEFAULT_TIMEOUT_MS 2000L

/* Analyse le contenu d'un exercise.txt. `origin` sert aux messages d'erreur
 * (ex. "exercises/cpp/hello/exercise.txt:12: clé inconnue « tittle »").
 * 0 / -1. En cas d'échec, *ex est entièrement libéré. */
int exercise_parse(const char *text, const char *origin, exercise *ex);

/* Charge <dir>/exercise.txt et renseigne ex->dir. 0 / -1. */
int exercise_load(const char *dir, exercise *ex);

/* Libère tout ce que possède ex (pas ex lui-même). */
void exercise_free(exercise *ex);

#endif
