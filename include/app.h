/*
 * app.h — contexte de l'application et sous-commandes.
 *
 * Le contexte (dossiers résolus au démarrage) est passé explicitement à
 * chaque commande plutôt que stocké dans des variables globales : chaque
 * fonction déclare ainsi clairement ce dont elle dépend, et les tests
 * peuvent construire leur propre contexte.
 */
#ifndef TRAINER_APP_H
#define TRAINER_APP_H

#include "catalog.h"
#include "exercise.h"
#include "strbuf.h"

/* Codes de sortie du programme (utilisables dans des scripts / la CI). */
enum {
    TR_EXIT_OK = 0,    /* succès (tous les tests passent...) */
    TR_EXIT_FAIL = 1,  /* l'élève a échoué (compilation, tests) */
    TR_EXIT_ERROR = 2, /* erreur d'utilisation ou d'environnement */
};

typedef struct {
    char *exercises_dir; /* chemin absolu du dossier d'exercices */
    char *workspace_dir; /* chemin absolu du dossier de travail de l'élève */
    char *self_path;     /* chemin absolu de l'exécutable (pour tasks.json) */
} app_ctx;

/* Charge le catalogue ; affiche les avertissements sur stderr.
 * TR_EXIT_OK ou TR_EXIT_ERROR. */
int app_load_catalog(const app_ctx *app, catalog *cat);

/* Cherche un exercice ; affiche un message d'erreur s'il n'existe pas. */
const exercise *app_find(const catalog *cat, const char *id);

/* Dossier de travail de l'élève pour ex : <workspace>/<lang>/<id>. */
void app_workdir(const app_ctx *app, const exercise *ex, strbuf *out);

/* Sous-commandes : argv[0] est le nom de la commande. Renvoient un TR_EXIT_*. */
int cmd_list(const app_ctx *app, int argc, char **argv);
int cmd_show(const app_ctx *app, int argc, char **argv);
int cmd_start(const app_ctx *app, int argc, char **argv);
int cmd_check(const app_ctx *app, int argc, char **argv);

#endif
