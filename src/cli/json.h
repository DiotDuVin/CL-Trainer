/*
 * json.h — écriture de JSON (sortie --json, lisible par d'autres programmes).
 *
 * Le rédacteur retient s'il faut une virgule avant le prochain élément :
 *     json_writer w; json_init(&w, &out);
 *     json_begin_object(&w);
 *       json_key(&w, "id"); json_string(&w, "cpp.basics.hello");
 *       json_key(&w, "passed"); json_bool(&w, 1);
 *     json_end(&w);
 * produit {"id":"cpp.basics.hello","passed":true}
 */
#ifndef TRAINER_JSON_H
#define TRAINER_JSON_H

#include "strbuf.h"

#define JSON_MAX_DEPTH 32

typedef struct {
    strbuf *out;
    int depth;
    char kind[JSON_MAX_DEPTH];     /* pile des conteneurs ouverts : '{' ou '[' */
    int has_items[JSON_MAX_DEPTH]; /* 1 si le conteneur a déjà un élément */
    int after_key;                 /* la valeur suit une clé : pas de virgule */
} json_writer;

void json_init(json_writer *w, strbuf *out);
void json_begin_object(json_writer *w);
void json_begin_array(json_writer *w);
void json_end(json_writer *w);   /* ferme l'objet ou le tableau courant */
void json_key(json_writer *w, const char *key);
void json_string(json_writer *w, const char *s); /* NULL -> null */
void json_int(json_writer *w, long v);
void json_bool(json_writer *w, int v);

#endif
