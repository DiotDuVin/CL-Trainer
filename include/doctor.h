/*
 * doctor.h — diagnostic de l'environnement : quels outils sont installés,
 * en quelle version, et comment installer ceux qui manquent.
 */
#ifndef TRAINER_DOCTOR_H
#define TRAINER_DOCTOR_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *command;     /* "g++" */
    const char *purpose;     /* "exercices C++" */
    int essential;           /* 1 : nécessaire au cœur de l'outil */
    int found;
    char *version;           /* première ligne de « --version », ou NULL */
    char *install_hint;      /* commande d'installation pour cette distribution */
} tool_status;

typedef struct {
    tool_status *tools;
    size_t len;
    char *distro;            /* "arch", "ubuntu"... (ID de /etc/os-release) ou "" */
} doctor_report;

/* Interroge chaque outil (« <outil> --version », avec un délai). */
void doctor_run(doctor_report *rep);
void doctor_free(doctor_report *rep);

#ifdef __cplusplus
}
#endif

#endif
