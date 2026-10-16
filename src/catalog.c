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

static int compare_by_dir(const void *a, const void *b)
{
    const exercise *ea = a, *eb = b;
    return strcmp(ea->dir, eb->dir);
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

    /* Prérequis inconnus : signalés, mais l'exercice reste utilisable. */
    for (size_t i = 0; i < cat->len; i++) {
        const strvec *req = &cat->items[i].requires;
        for (size_t k = 0; k < req->len; k++) {
            if (!catalog_find(cat, req->items[k])) {
                sb_clear(&msg);
                sb_appendf(&msg, "%s : prérequis inconnu « %s »", cat->items[i].id,
                           req->items[k]);
                add_error(cat, msg.data);
            }
        }
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

void catalog_free(catalog *cat)
{
    for (size_t i = 0; i < cat->len; i++)
        exercise_free(&cat->items[i]);
    free(cat->items);
    sv_free(&cat->errors);
    memset(cat, 0, sizeof *cat);
}
