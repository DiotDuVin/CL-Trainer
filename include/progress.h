/*
 * progress.h — progression de l'élève, dans un fichier texte lisible :
 *
 *   # trainer-progress v1
 *   cpp.basics.hello status=passed attempts=3 build_failures=1 hints=1 solution_seen=0 first=2026-10-16T20:34:00Z last=... passed_at=...
 *
 * Garanties :
 * - écriture ATOMIQUE : on écrit un fichier temporaire puis on le renomme.
 *   rename() remplace le fichier d'un coup : un lecteur voit l'ancienne ou
 *   la nouvelle version, jamais un fichier à moitié écrit, même en cas de
 *   coupure de courant au mauvais moment ;
 * - VERROU : progress_record() prend un verrou exclusif pendant
 *   « lire -> modifier -> écrire ». Sans lui, si le terminal et
 *   l'application graphique enregistrent en même temps, l'une des deux
 *   modifications serait perdue (« lost update ») ;
 * - COMPATIBILITÉ : les champs inconnus (ajoutés par une version future)
 *   sont conservés tels quels à la réécriture.
 */
#ifndef TRAINER_PROGRESS_H
#define TRAINER_PROGRESS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STATUS_NEW,      /* jamais commencé */
    STATUS_STARTED,  /* commencé, jamais vérifié */
    STATUS_FAILED,   /* vérifié, pas encore réussi */
    STATUS_PASSED,   /* réussi au moins une fois */
} exercise_status;

#define PROGRESS_DATE_LEN 21 /* "2026-10-16T20:34:00Z" + '\0' */

typedef struct {
    char *id;
    exercise_status status;
    int attempts;          /* nombre de vérifications (check) */
    int build_failures;    /* dont échecs de compilation */
    int hints;             /* nombre d'indices révélés */
    int solution_seen;     /* 1 si la solution a été consultée */
    char first[PROGRESS_DATE_LEN];      /* première activité ("" si aucune) */
    char last[PROGRESS_DATE_LEN];       /* dernière activité */
    char passed_at[PROGRESS_DATE_LEN];  /* première réussite */
    char *extra;           /* champs inconnus conservés ("" si aucun) */
} progress_entry;

typedef struct {
    progress_entry *items;
    size_t len;
    size_t cap;
} progress;

#define PROGRESS_INIT { NULL, 0, 0 }

/* Événements enregistrés */
typedef enum {
    EVENT_START,          /* trainer start */
    EVENT_CHECK_BUILD_FAILED,
    EVENT_CHECK_FAILED,   /* compilé, tests échoués */
    EVENT_CHECK_PASSED,
    EVENT_HINT,           /* arg = numéro de l'indice révélé (1..n) */
    EVENT_SOLUTION,
} progress_event;

/* Charge le fichier (inexistant = progression vide). 0 / -1. */
int progress_load(const char *path, progress *p);
/* Écrit le fichier de façon atomique (crée le dossier si besoin). 0 / -1. */
int progress_save(const char *path, const progress *p);
void progress_free(progress *p);

/* Entrée de l'exercice id, ou NULL. */
const progress_entry *progress_find(const progress *p, const char *id);
/* Statut (STATUS_NEW si absent). */
exercise_status progress_status(const progress *p, const char *id);

/* Applique un événement en mémoire (crée l'entrée si besoin).
 * now : date ISO 8601 UTC, ou NULL pour l'heure actuelle. */
void progress_apply(progress *p, const char *id, progress_event ev, int arg,
                    const char *now);

/* Transaction complète : verrouille, charge, applique, sauvegarde,
 * déverrouille. C'est la fonction à utiliser depuis les commandes. 0 / -1. */
int progress_record(const char *path, const char *id, progress_event ev, int arg);

const char *progress_status_name(exercise_status s); /* "passed"... */

#ifdef __cplusplus
}
#endif

#endif
