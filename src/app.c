/* app.c — fonctions partagées par les sous-commandes (voir app.h). */
#include "app.h"

#include <stdio.h>

#include "error.h"
#include "fs.h"
#include "term.h"

int app_load_catalog(const app_ctx *app, catalog *cat)
{
    if (catalog_load(app->exercises_dir, cat) != 0) {
        fprintf(stderr, "trainer: %s\n", err_msg());
        return TR_EXIT_ERROR;
    }
    for (size_t i = 0; i < cat->errors.len; i++)
        fprintf(stderr, "%sattention%s : %s\n", term(T_YELLOW), term(T_RESET),
                cat->errors.items[i]);
    return TR_EXIT_OK;
}

const exercise *app_find(const catalog *cat, const char *id)
{
    const exercise *ex = catalog_find(cat, id);
    if (!ex)
        fprintf(stderr, "trainer: exercice inconnu « %s ». "
                        "La commande « trainer list » affiche les exercices.\n", id);
    return ex;
}

void app_workdir(const app_ctx *app, const exercise *ex, strbuf *out)
{
    strbuf tmp = STRBUF_INIT;
    path_join(&tmp, app->workspace_dir, ex->lang);
    path_join(out, tmp.data, ex->id);
    sb_free(&tmp);
}
