/*
 * strbuf.h — chaîne de caractères dynamique.
 *
 * Invariant : si data != NULL, data[len] == '\0'. On peut donc toujours
 * passer sb_cstr(&sb) à une fonction qui attend une chaîne C.
 *
 * Usage typique :
 *     strbuf sb = STRBUF_INIT;
 *     sb_appendf(&sb, "%d tests", n);
 *     puts(sb_cstr(&sb));
 *     sb_free(&sb);
 *
 * Attention : ne jamais passer sb.data (ou un pointeur dedans) en argument
 * d'une fonction d'ajout sur le même strbuf : un realloc pourrait l'invalider.
 */
#ifndef TRAINER_STRBUF_H
#define TRAINER_STRBUF_H

#include <stddef.h>

#include "error.h"

typedef struct {
    char *data;  /* NULL tant que rien n'a été ajouté */
    size_t len;  /* longueur sans le '\0' final */
    size_t cap;  /* taille allouée, '\0' compris */
} strbuf;

#define STRBUF_INIT { NULL, 0, 0 }

void sb_append(strbuf *sb, const char *s);
void sb_append_n(strbuf *sb, const char *s, size_t n);
void sb_append_char(strbuf *sb, char c);
void sb_appendf(strbuf *sb, const char *fmt, ...) ATTR_PRINTF(2, 3);

/* Réduit la longueur à len (len <= sb->len). */
void sb_truncate(strbuf *sb, size_t len);
/* Vide la chaîne sans libérer la mémoire (réutilisation). */
void sb_clear(strbuf *sb);
/* Libère la mémoire ; le strbuf redevient STRBUF_INIT. */
void sb_free(strbuf *sb);

/* Contenu en chaîne C, "" si vide. Le pointeur reste la propriété du strbuf. */
const char *sb_cstr(const strbuf *sb);
/* Transfère la propriété : renvoie une chaîne à libérer avec free() et
 * remet le strbuf à zéro. Ne renvoie jamais NULL. */
char *sb_release(strbuf *sb);

#endif
