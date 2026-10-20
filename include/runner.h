/*
 * runner.h — interface commune à tous les langages.
 *
 * Un runner est une table de pointeurs de fonctions : c'est la manière
 * idiomatique d'obtenir du polymorphisme en C. C'est exactement ce que fait
 * le compilateur C++ avec une classe à méthodes virtuelles (la « vtable »),
 * ou Rust avec un `dyn Trait`. Le moteur ne connaît que cette interface :
 * ajouter un langage = écrire un nouveau runner_xxx.c et l'enregistrer dans
 * runner.c, sans toucher au reste.
 */
#ifndef TRAINER_RUNNER_H
#define TRAINER_RUNNER_H

#include "exercise.h"
#include "process.h"
#include "strbuf.h"
#include "testspec.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Résultat de build() */
enum { BUILD_OK = 0, BUILD_FAILED = 1, BUILD_ERROR = -1 };

typedef struct runner {
    const char *lang;          /* "cpp" */
    const char *display_name;  /* "C++" */
    const char *default_file;  /* fichier élève si `files` est vide : "main.cpp" */

    /* Prépare le dossier de l'élève après copie du starter (fichiers de
     * configuration d'éditeur...). Optionnel (NULL). 0 / -1. */
    int (*prepare_workspace)(const exercise *ex, const char *workdir);

    /* Compile le code de workdir. Les messages du compilateur vont dans diag.
     * BUILD_OK, BUILD_FAILED (erreur dans le code de l'élève) ou BUILD_ERROR
     * (problème d'outil : compilateur absent... ; message dans err_msg()). */
    int (*build)(const exercise *ex, const char *workdir, strbuf *diag);

    /* Exécute un cas de test. 0 / -1 (lancement impossible). */
    int (*run)(const exercise *ex, const char *workdir, const test_case *tc,
               proc_result *res);
} runner;

/* Runner du langage, ou NULL s'il n'est pas (encore) pris en charge. */
const runner *runner_for(const char *lang);

#ifdef __cplusplus
}
#endif

#endif
