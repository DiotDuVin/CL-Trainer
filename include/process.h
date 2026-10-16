/*
 * process.h — lancer un programme, lui fournir stdin, capturer stdout et
 * stderr, et l'arrêter s'il dépasse un délai.
 *
 * Pourquoi pas popen()/system() ? Ils passent par le shell (problèmes de
 * guillemets et d'injection), ne capturent qu'un seul flux, n'ont pas de
 * timeout et ne savent pas tuer un programme qui boucle. On utilise donc
 * directement fork() + execvp() + pipe() + poll().
 */
#ifndef TRAINER_PROCESS_H
#define TRAINER_PROCESS_H

#include <stddef.h>

#include "strbuf.h"

typedef struct {
    const char *cwd;          /* dossier de travail du fils, NULL = inchangé */
    const char *input;        /* données envoyées sur stdin, NULL = rien */
    size_t input_len;
    long timeout_ms;          /* <= 0 : pas de limite */
    size_t max_output;        /* par flux ; 0 = illimité. Au-delà : arrêt */
    long mem_limit_mb;        /* limite d'espace d'adressage, 0 = aucune */
} proc_opts;

typedef struct {
    int exited;               /* 1 : terminé normalement (exit / return) */
    int exit_code;            /* valable si exited */
    int signaled;             /* 1 : tué par un signal */
    int signal;               /* valable si signaled (ex. SIGSEGV) */
    int timed_out;            /* 1 : tué car délai dépassé */
    int truncated;            /* 1 : tué car sortie trop longue */
    long elapsed_ms;
    strbuf out;               /* stdout capturé */
    strbuf err;               /* stderr capturé */
} proc_result;

/* Lance argv[0] (cherché dans le PATH) avec les arguments argv (terminé par
 * NULL). Renvoie 0 si le programme a pu être lancé, quelle que soit sa façon
 * de se terminer ; -1 si le lancement est impossible (commande introuvable,
 * dossier inexistant...), avec le message dans err_msg() et l'errno dans
 * *exec_errno si non NULL. res doit être libéré avec proc_result_free()
 * dans tous les cas. */
int proc_run(const char *const argv[], const proc_opts *opts, proc_result *res,
             int *exec_errno);

void proc_result_free(proc_result *res);

/* Description pédagogique d'un signal, ex. SIGSEGV -> "erreur de
 * segmentation (accès mémoire invalide : pointeur nul, hors tableau...)". */
const char *proc_signal_description(int sig);

#endif
