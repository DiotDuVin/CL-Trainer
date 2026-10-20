/* stats.c — voir stats.h.
 *
 * Mesure de la « difficulté rencontrée » (struggle) sur un exercice :
 *     échecs de vérification + 0,5 × indices révélés + 2 × solution consultée
 * puis moyenne sur les exercices tentés du thème. Un thème dont la moyenne
 * dépasse STATS_WEAK_THRESHOLD est un point faible. Ces poids sont un choix
 * arbitraire mais simple à expliquer : une solution consultée coûte « deux
 * échecs », un indice un demi.
 */
#include "stats.h"

#include <stdlib.h>
#include <string.h>

#include "xalloc.h"

static double entry_struggle(const progress_entry *e)
{
    int failed_checks = e->attempts - (e->status == STATUS_PASSED ? 1 : 0);
    if (failed_checks < 0)
        failed_checks = 0;
    return failed_checks + 0.5 * e->hints + 2.0 * e->solution_seen;
}

static int attempted(const progress_entry *e)
{
    return e && (e->attempts > 0 || e->hints > 0 || e->solution_seen);
}

static topic_stats *topic_for(stats *s, const exercise *ex)
{
    for (size_t i = 0; i < s->len; i++)
        if (strcmp(s->topics[i].lang, ex->lang) == 0 &&
            strcmp(s->topics[i].topic, ex->topic) == 0)
            return &s->topics[i];
    s->topics = grow_array(s->topics, &s->cap, s->len + 1, sizeof *s->topics);
    topic_stats *t = &s->topics[s->len++];
    memset(t, 0, sizeof *t);
    t->lang = ex->lang;
    t->topic = ex->topic;
    return t;
}

void stats_compute(const catalog *cat, const progress *p, stats *s)
{
    memset(s, 0, sizeof *s);
    for (size_t i = 0; i < cat->len; i++) {
        const exercise *ex = &cat->items[i];
        const progress_entry *e = progress_find(p, ex->id);
        topic_stats *t = topic_for(s, ex);
        t->total++;
        s->total++;
        if (e && e->status == STATUS_PASSED) {
            t->passed++;
            s->passed++;
            if (e->attempts == 1 && e->hints == 0 && !e->solution_seen)
                s->first_try++;
        }
        if (attempted(e)) {
            /* moyenne incrémentale : m_n = m_{n-1} + (x - m_{n-1}) / n */
            t->attempted++;
            t->struggle += (entry_struggle(e) - t->struggle) / (double)t->attempted;
            s->checks += e->attempts;
        }
    }
}

void stats_free(stats *s)
{
    free(s->topics);
    memset(s, 0, sizeof *s);
}

int exercise_unlocked(const catalog *cat, const progress *p, const exercise *ex)
{
    for (size_t i = 0; i < ex->prereqs.len; i++) {
        const char *req = ex->prereqs.items[i];
        if (catalog_find(cat, req) && progress_status(p, req) != STATUS_PASSED)
            return 0;
    }
    return 1;
}

int stats_topic_is_weak(const topic_stats *t)
{
    return t->attempted >= STATS_MIN_ATTEMPTED && t->struggle >= STATS_WEAK_THRESHOLD;
}

static int topic_is_weak(const stats *s, const exercise *ex)
{
    for (size_t i = 0; i < s->len; i++)
        if (strcmp(s->topics[i].lang, ex->lang) == 0 &&
            strcmp(s->topics[i].topic, ex->topic) == 0)
            return stats_topic_is_weak(&s->topics[i]);
    return 0;
}

void recommend_next(const catalog *cat, const progress *p, const char *lang,
                    recommendation *out)
{
    memset(out, 0, sizeof *out);

    /* 1. Finir ce qui est commencé : le plus récemment travaillé d'abord
     *    (les dates ISO 8601 se comparent comme des chaînes). */
    const char *latest = "";
    for (size_t i = 0; i < cat->len; i++) {
        const exercise *ex = &cat->items[i];
        if (lang && strcmp(ex->lang, lang) != 0)
            continue;
        const progress_entry *e = progress_find(p, ex->id);
        if (e && (e->status == STATUS_STARTED || e->status == STATUS_FAILED) &&
            strcmp(e->last, latest) >= 0) {
            out->ex = ex;
            latest = e->last;
        }
    }
    if (out->ex) {
        out->reason = RECO_RESUME;
        return;
    }

    /* 2. Sinon, un nouvel exercice déverrouillé du plus bas niveau ; à
     *    niveau égal, un thème faible passe en premier, puis l'ordre du
     *    catalogue (la progression prévue par les dossiers 01-, 02-...). */
    stats s;
    stats_compute(cat, p, &s);
    int min_level = 99, remaining = 0;
    for (size_t i = 0; i < cat->len; i++) {
        const exercise *ex = &cat->items[i];
        if (lang && strcmp(ex->lang, lang) != 0)
            continue;
        if (progress_status(p, ex->id) == STATUS_PASSED)
            continue;
        remaining = 1;
        if (exercise_unlocked(cat, p, ex) && ex->level < min_level)
            min_level = ex->level;
    }
    const exercise *first = NULL, *weak = NULL;
    for (size_t i = 0; i < cat->len; i++) {
        const exercise *ex = &cat->items[i];
        if ((lang && strcmp(ex->lang, lang) != 0) || ex->level != min_level ||
            progress_status(p, ex->id) != STATUS_NEW || !exercise_unlocked(cat, p, ex))
            continue;
        if (!first)
            first = ex;
        if (!weak && topic_is_weak(&s, ex))
            weak = ex;
    }
    stats_free(&s);

    if (weak) {
        out->ex = weak;
        out->reason = RECO_WEAK_TOPIC;
        out->topic = weak->topic;
    } else if (first) {
        out->ex = first;
        out->reason = RECO_NEXT;
    } else {
        out->reason = remaining ? RECO_LOCKED : RECO_ALL_DONE;
    }

    /* 3. Le cours avant les exercices : si celui de l'exercice choisi n'est
     *    pas encore lu, on propose de le lire d'abord. */
    if (out->ex && out->ex->lesson && progress_status(p, out->ex->lesson) != STATUS_PASSED) {
        out->lesson_to_read = catalog_find_lesson(cat, out->ex->lesson);
        if (out->lesson_to_read)
            out->reason = RECO_LESSON;
    }
}
