/*
 * stats.h — statistiques de progression et recommandation de l'exercice
 * suivant, calculées à partir du catalogue et de la progression.
 */
#ifndef TRAINER_STATS_H
#define TRAINER_STATS_H

#include <stddef.h>

#include "catalog.h"
#include "progress.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Statistiques d'un thème (topic) d'un langage. Les chaînes pointent dans
 * le catalogue : elles restent valides tant que le catalogue existe. */
typedef struct {
    const char *lang;
    const char *topic;
    size_t total;         /* exercices du thème */
    size_t passed;
    size_t attempted;     /* vérifiés au moins une fois */
    double struggle;      /* difficulté rencontrée, voir stats.c (0 = aucune) */
} topic_stats;

typedef struct {
    topic_stats *topics;  /* dans l'ordre du catalogue */
    size_t len;
    size_t cap;
    size_t total, passed;
    int checks;           /* vérifications au total */
    int first_try;        /* exercices réussis du premier coup */
} stats;

/* Un thème est un point faible si au moins STATS_MIN_ATTEMPTED exercices y
 * ont été tentés (sinon, pas assez de données pour conclure) et si la
 * difficulté moyenne atteint STATS_WEAK_THRESHOLD. */
#define STATS_WEAK_THRESHOLD 1.5
#define STATS_MIN_ATTEMPTED 2
int stats_topic_is_weak(const topic_stats *t);

void stats_compute(const catalog *cat, const progress *p, stats *s);
void stats_free(stats *s);

/* 1 si tous les prérequis de ex sont réussis (prérequis inconnus ignorés). */
int exercise_unlocked(const catalog *cat, const progress *p, const exercise *ex);

typedef enum {
    RECO_RESUME,        /* reprendre un exercice commencé et pas réussi */
    RECO_NEXT,          /* nouvel exercice : niveau le plus bas d'abord */
    RECO_WEAK_TOPIC,    /* nouvel exercice choisi dans un thème faible */
    RECO_ALL_DONE,      /* tout est réussi */
    RECO_LOCKED,        /* il reste des exercices, mais tous verrouillés */
} reco_reason;

typedef struct {
    const exercise *ex;     /* NULL si RECO_ALL_DONE / RECO_LOCKED */
    reco_reason reason;
    const char *topic;      /* thème faible concerné (RECO_WEAK_TOPIC) */
} recommendation;

/* lang : limiter à un langage, ou NULL. */
void recommend_next(const catalog *cat, const progress *p, const char *lang,
                    recommendation *out);

#ifdef __cplusplus
}
#endif

#endif
