/*
 * config.h — emplacements utilisés par trainer, résolus une fois au
 * démarrage et partagés par le terminal et l'application graphique.
 *
 *   exercices   : --exercises > $TRAINER_EXERCISES > <dossier du programme>/exercises
 *                 > $(PREFIX)/share/trainer/exercises > ./exercises
 *   workspace   : --workspace > $TRAINER_WORKSPACE > ~/trainer-workspace
 *   données     : $XDG_DATA_HOME/trainer > ~/.local/share/trainer
 *                 (convention XDG, suivie par la plupart des logiciels Linux)
 */
#ifndef TRAINER_CONFIG_H
#define TRAINER_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *exercises_dir;  /* chemins absolus */
    char *workspace_dir;
    char *data_dir;
    char *progress_path;  /* <data_dir>/progress.txt */
    char *cli_path;       /* commande « trainer » (pour .vscode/tasks.json) */
} trainer_config;

/* opt_exercises / opt_workspace : valeurs des options, ou NULL.
 * argv0 : argv[0] du programme (sert à trouver ses dossiers voisins).
 * 0 / -1 (err_msg). Libérer avec config_free dans tous les cas. */
int config_resolve(const char *opt_exercises, const char *opt_workspace,
                   const char *argv0, trainer_config *cfg);
void config_free(trainer_config *cfg);

#ifdef __cplusplus
}
#endif

#endif
