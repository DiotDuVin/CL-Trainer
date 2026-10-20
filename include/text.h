/*
 * text.h — petits outils de traitement de texte utilisés par les parsers.
 *
 * Beaucoup de fonctions travaillent sur des « spans » (pointeur + longueur)
 * plutôt que sur des chaînes terminées par '\0' : on peut ainsi désigner une
 * ligne au milieu d'un fichier sans la recopier.
 */
#ifndef TRAINER_TEXT_H
#define TRAINER_TEXT_H

#include <stddef.h>

#include "strbuf.h"
#include "strvec.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Itère sur les lignes d'un texte. À chaque appel, place dans *line / *len
 * la ligne suivante (sans '\n' ni '\r' final) et avance *cursor.
 * Renvoie 0 quand le texte est épuisé. */
int text_next_line(const char **cursor, const char **line, size_t *len);

/* Le span (s, n) est-il exactement égal à lit ? */
int span_eq(const char *s, size_t n, const char *lit);
/* Le span commence-t-il par prefix ? */
int span_starts_with(const char *s, size_t n, const char *prefix);
/* Le span ne contient-il que des espaces / tabulations ? */
int span_is_blank(const char *s, size_t n);
/* Retire les espaces et tabulations au début et à la fin du span. */
void span_trim(const char **s, size_t *n);

/* Convertit le span entier en long dans [min, max]. 0 si OK, -1 sinon. */
int parse_long(const char *s, size_t n, long min, long max, long *out);

/* Découpe "a, b ,c" en ["a","b","c"] (éléments vides ignorés). */
void split_list(const char *s, size_t n, char sep, strvec *out);

/* Ajoute s échappée pour une chaîne JSON (sans les guillemets). */
void json_escape(strbuf *out, const char *s);

/* Ajoute s avec les caractères invisibles rendus visibles (\t, \r, \xNN).
 * Sert à afficher une différence de sortie due à un espace ou un \r. */
void escape_visible(strbuf *out, const char *s, size_t n);

#ifdef __cplusplus
}
#endif

#endif
