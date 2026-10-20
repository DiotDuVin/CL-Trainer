/* catalog.c — voir catalog.h. */
#include "catalog.h"

#include <dirent.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "fs.h"
#include "strbuf.h"
#include "xalloc.h"

#define MAX_DEPTH 6 /* garde-fou contre un lien symbolique qui boucle */

static void add_error(catalog *cat, const char *msg)
{
    sv_push(&cat->errors, msg);
}

static int scan(catalog *cat, const char *dir, int depth)
{
    strbuf path = STRBUF_INIT;
    path_join(&path, dir, "exercise.txt");
    if (fs_is_file(path.data)) {
        exercise ex;
        if (exercise_load(dir, &ex) == 0) {
            cat->items = grow_array(cat->items, &cat->cap, cat->len + 1,
                                    sizeof *cat->items);
            cat->items[cat->len++] = ex; /* copie de la structure : le catalogue
                                            devient propriétaire de ses champs */
        } else {
            add_error(cat, err_msg());
        }
        sb_free(&path);
        return 0; /* un exercice ne contient pas d'autres exercices */
    }

    /* Un chapitre : son cours, puis on continue dans ses sous-dossiers. */
    path_join(&path, dir, "lesson.md");
    if (fs_is_file(path.data)) {
        lesson l;
        if (lesson_load(dir, &l) == 0) {
            cat->lessons = grow_array(cat->lessons, &cat->lessons_cap, cat->nlessons + 1,
                                      sizeof *cat->lessons);
            cat->lessons[cat->nlessons++] = l;
        } else {
            add_error(cat, err_msg());
        }
    }

    if (depth >= MAX_DEPTH) {
        sb_free(&path);
        return 0;
    }
    DIR *d = opendir(dir);
    if (!d) {
        err_set_errno("impossible d'ouvrir %s", dir);
        sb_free(&path);
        return -1;
    }
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') /* ., .. et dossiers cachés */
            continue;
        path_join(&path, dir, ent->d_name);
        if (fs_is_dir(path.data) && scan(cat, path.data, depth + 1) != 0)
            add_error(cat, err_msg());
    }
    closedir(d);
    sb_free(&path);
    return 0;
}

/* Détection de cycle dans le graphe des prérequis : parcours en profondeur
 * avec trois couleurs (algorithme classique). BLANC = pas encore vu,
 * GRIS = en cours d'exploration (sur la pile), NOIR = terminé. Atteindre un
 * sommet GRIS signifie qu'on est revenu sur ses pas : il y a un cycle. */
enum { WHITE, GRAY, BLACK };

static int find_cycle(const catalog *cat, size_t i, unsigned char *color, size_t *stack,
                      size_t depth, strbuf *msg)
{
    color[i] = GRAY;
    stack[depth] = i;
    const strvec *req = &cat->items[i].prereqs;
    for (size_t k = 0; k < req->len; k++) {
        const exercise *dep = catalog_find(cat, req->items[k]);
        if (!dep)
            continue;
        size_t j = (size_t)(dep - cat->items);
        if (color[j] == GRAY) { /* cycle : on l'écrit depuis j */
            sb_append(msg, "cycle de prérequis : ");
            size_t from = depth;
            while (stack[from] != j)
                from--;
            for (size_t d = from; d <= depth; d++)
                sb_appendf(msg, "%s -> ", cat->items[stack[d]].id);
            sb_append(msg, cat->items[j].id);
            return 1;
        }
        if (color[j] == WHITE && find_cycle(cat, j, color, stack, depth + 1, msg))
            return 1;
    }
    color[i] = BLACK;
    return 0;
}

static int compare_by_dir(const void *a, const void *b)
{
    const exercise *ea = a, *eb = b;
    return strcmp(ea->dir, eb->dir);
}

static int compare_lessons(const void *a, const void *b)
{
    const lesson *la = a, *lb = b;
    return strcmp(la->dir, lb->dir);
}

/* Rattache chaque exercice au cours du dossier parent le plus proche. */
static void link_lessons(catalog *cat)
{
    for (size_t i = 0; i < cat->len; i++) {
        exercise *ex = &cat->items[i];
        const lesson *best = NULL;
        size_t best_len = 0;
        for (size_t k = 0; k < cat->nlessons; k++) {
            const lesson *l = &cat->lessons[k];
            size_t n = strlen(l->dir);
            /* "a/b" est parent de "a/b/c" mais pas de "a/bc" : on exige le '/'. */
            if (strncmp(ex->dir, l->dir, n) == 0 && ex->dir[n] == '/' && n > best_len) {
                best = l;
                best_len = n;
            }
        }
        if (best)
            ex->lesson = xstrdup(best->id);
    }
}

int catalog_load(const char *root, catalog *cat)
{
    memset(cat, 0, sizeof *cat);
    if (!fs_is_dir(root)) {
        err_set("dossier d'exercices introuvable : %s", root);
        return -1;
    }
    if (scan(cat, root, 0) != 0) {
        catalog_free(cat);
        return -1;
    }
    /* readdir ne garantit aucun ordre : on trie pour un affichage stable. */
    if (cat->len > 1)
        qsort(cat->items, cat->len, sizeof *cat->items, compare_by_dir);
    if (cat->nlessons > 1)
        qsort(cat->lessons, cat->nlessons, sizeof *cat->lessons, compare_lessons);

    strbuf msg = STRBUF_INIT;

    /* Ids en double : on garde le premier (dans l'ordre trié). Recherche
     * quadratique : négligeable pour quelques centaines d'exercices. */
    for (size_t i = 0; i < cat->len; i++) {
        for (size_t j = 0; j < i; j++) {
            if (strcmp(cat->items[i].id, cat->items[j].id) == 0) {
                sb_clear(&msg);
                sb_appendf(&msg, "%s : id « %s » déjà utilisé par %s (ignoré)",
                           cat->items[i].dir, cat->items[i].id, cat->items[j].dir);
                add_error(cat, msg.data);
                exercise_free(&cat->items[i]);
                memmove(&cat->items[i], &cat->items[i + 1],
                        (cat->len - i - 1) * sizeof *cat->items);
                cat->len--;
                i--;
                break;
            }
        }
    }

    /* Cours : id en double (entre eux ou avec un exercice) -> ignoré. */
    for (size_t i = 0; i < cat->nlessons; i++) {
        const lesson *l = &cat->lessons[i];
        int dup = catalog_find(cat, l->id) != NULL;
        for (size_t j = 0; j < i && !dup; j++)
            dup = strcmp(cat->lessons[j].id, l->id) == 0;
        if (dup) {
            sb_clear(&msg);
            sb_appendf(&msg, "%s : id « %s » déjà utilisé (cours ignoré)", l->dir, l->id);
            add_error(cat, msg.data);
            lesson_free(&cat->lessons[i]);
            memmove(&cat->lessons[i], &cat->lessons[i + 1],
                    (cat->nlessons - i - 1) * sizeof *cat->lessons);
            cat->nlessons--;
            i--;
        }
    }
    link_lessons(cat);

    /* Prérequis inconnus : signalés, mais l'exercice reste utilisable. */
    for (size_t i = 0; i < cat->len; i++) {
        const strvec *req = &cat->items[i].prereqs;
        for (size_t k = 0; k < req->len; k++) {
            if (!catalog_find(cat, req->items[k])) {
                sb_clear(&msg);
                sb_appendf(&msg, "%s : prérequis inconnu « %s »", cat->items[i].id,
                           req->items[k]);
                add_error(cat, msg.data);
            }
        }
    }

    /* Cycles : un seul est signalé, cela suffit pour corriger. */
    if (cat->len > 0) {
        unsigned char *color = xcalloc(cat->len, 1);
        size_t *stack = xcalloc(cat->len, sizeof *stack);
        for (size_t i = 0; i < cat->len; i++) {
            sb_clear(&msg);
            if (color[i] == WHITE && find_cycle(cat, i, color, stack, 0, &msg)) {
                add_error(cat, msg.data);
                break;
            }
        }
        free(color);
        free(stack);
    }
    sb_free(&msg);
    return 0;
}

const exercise *catalog_find(const catalog *cat, const char *id)
{
    for (size_t i = 0; i < cat->len; i++)
        if (strcmp(cat->items[i].id, id) == 0)
            return &cat->items[i];
    return NULL;
}

const lesson *catalog_find_lesson(const catalog *cat, const char *id)
{
    for (size_t i = 0; i < cat->nlessons; i++)
        if (strcmp(cat->lessons[i].id, id) == 0)
            return &cat->lessons[i];
    return NULL;
}

void catalog_free(catalog *cat)
{
    for (size_t i = 0; i < cat->nlessons; i++)
        lesson_free(&cat->lessons[i]);
    free(cat->lessons);
    for (size_t i = 0; i < cat->len; i++)
        exercise_free(&cat->items[i]);
    free(cat->items);
    sv_free(&cat->errors);
    memset(cat, 0, sizeof *cat);
}
