/*
 * strvec.h — tableau dynamique de chaînes, possédées par le tableau.
 *
 * Invariant : si items != NULL, items[len] == NULL. Le tableau peut donc
 * servir directement d'argv pour execvp().
 */
#ifndef TRAINER_STRVEC_H
#define TRAINER_STRVEC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char **items;
    size_t len;
    size_t cap;
} strvec;

#define STRVEC_INIT { NULL, 0, 0 }

/* Ajoute une copie de s. */
void sv_push(strvec *sv, const char *s);
/* Ajoute une copie des n premiers octets de s. */
void sv_push_n(strvec *sv, const char *s, size_t n);
/* Ajoute s sans copie : le strvec en devient propriétaire (le libérera). */
void sv_push_owned(strvec *sv, char *s);
/* 1 si une chaîne égale à s est présente. */
int sv_contains(const strvec *sv, const char *s);
/* Libère toutes les chaînes et le tableau. */
void sv_free(strvec *sv);

#ifdef __cplusplus
}
#endif

#endif
