/*
 * lesson.h — cours (fichier lesson.md placé dans un dossier de chapitre).
 *
 * Un chapitre est un dossier contenant un cours et ses exercices :
 *
 *   exercises/cpp/02-references/
 *   ├── lesson.md           <- le cours du chapitre
 *   ├── swap/exercise.txt   <- exercices du chapitre (rattachés au cours)
 *   └── minmax/exercise.txt
 *
 * Format de lesson.md : un en-tête « clé: valeur », une ligne « --- », puis
 * le cours en Markdown.
 *
 *   id: cpp.lesson.references
 *   title: Références et const
 *   lang: cpp
 *   topic: references
 *   estimated_min: 15
 *   ---
 *   ## Ce que tu connais en C
 *   ...
 *
 * Les blocs de code ```cpp example sont des programmes complets : la
 * validation les compile et les exécute. Un bloc ```output qui suit
 * immédiatement un exemple donne sa sortie attendue.
 */
#ifndef TRAINER_LESSON_H
#define TRAINER_LESSON_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *id;               /* "cpp.lesson.references" : commence par "<lang>." */
    char *title;
    char *lang;
    char *topic;
    int estimated_min;      /* 0 = non renseigné */
    char *body;             /* le cours, en Markdown */
    int body_line;          /* ligne de lesson.md où commence body */
    char *dir;              /* dossier du chapitre (NULL si parsé depuis un texte) */
} lesson;

/* Un exemple de code extrait du cours. */
typedef struct {
    char *code;             /* programme complet */
    char *expected;         /* sortie attendue (bloc ```output), ou NULL */
    int line;               /* ligne de la balise ```<lang> example dans lesson.md */
} lesson_example;

typedef struct {
    lesson_example *items;
    size_t len;
    size_t cap;
} lesson_examples;

int lesson_parse(const char *text, const char *origin, lesson *l);
/* Charge <dir>/lesson.md. 0 / -1. */
int lesson_load(const char *dir, lesson *l);
void lesson_free(lesson *l);

/* Extrait les blocs ```<lang> example (et leurs ```output). */
void lesson_extract_examples(const lesson *l, lesson_examples *out);
void lesson_examples_free(lesson_examples *ex);

#ifdef __cplusplus
}
#endif

#endif
