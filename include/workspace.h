/*
 * workspace.h — dossier de travail de l'élève pour un exercice :
 * <workspace>/<lang>/<id>/, avec le code copié depuis starter/.
 */
#ifndef TRAINER_WORKSPACE_H
#define TRAINER_WORKSPACE_H

#include "config.h"
#include "exercise.h"
#include "strbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    START_CREATED,   /* premier démarrage : starter copié */
    START_EXISTED,   /* déjà commencé : fichiers de l'élève conservés */
    START_RESET,     /* --force : starter recopié par-dessus */
} start_result;

/* out = dossier de travail de ex (vidé avant). */
void workspace_dir(const trainer_config *cfg, const exercise *ex, strbuf *out);

/* 1 si l'exercice a déjà été commencé (dossier présent). */
int workspace_exists(const trainer_config *cfg, const exercise *ex);

/* Copie le starter (si besoin, ou toujours si force) puis (re)génère les
 * fichiers d'éditeur : .vscode/tasks.json, compile_flags.txt, .gitignore.
 * 0 / -1. */
int workspace_start(const trainer_config *cfg, const exercise *ex, int force,
                    start_result *result);

/* Ouvre dir dans VS Code (commande « code »). 0 / -1. */
int workspace_open_editor(const char *dir);

#ifdef __cplusplus
}
#endif

#endif
