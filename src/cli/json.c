/* json.c — voir json.h. */
#include "json.h"

#include <string.h>

#include "error.h"
#include "text.h"

void json_init(json_writer *w, strbuf *out)
{
    memset(w, 0, sizeof *w);
    w->out = out;
}

/* Avant chaque valeur : une virgule si le conteneur a déjà un élément. */
static void before_value(json_writer *w)
{
    if (w->after_key) {
        w->after_key = 0;
        return;
    }
    if (w->depth > 0) {
        if (w->has_items[w->depth - 1])
            sb_append_char(w->out, ',');
        w->has_items[w->depth - 1] = 1;
    }
}

static void begin(json_writer *w, char open)
{
    before_value(w);
    if (w->depth >= JSON_MAX_DEPTH)
        die("JSON trop imbriqué");
    sb_append_char(w->out, open);
    w->kind[w->depth] = open;
    w->has_items[w->depth++] = 0;
}

void json_begin_object(json_writer *w) { begin(w, '{'); }
void json_begin_array(json_writer *w) { begin(w, '['); }

void json_end(json_writer *w)
{
    if (w->depth == 0)
        die("json_end sans conteneur ouvert");
    w->depth--;
    sb_append_char(w->out, w->kind[w->depth] == '{' ? '}' : ']');
}

void json_key(json_writer *w, const char *key)
{
    before_value(w);
    sb_append_char(w->out, '"');
    json_escape(w->out, key);
    sb_append(w->out, "\":");
    w->after_key = 1;
}

void json_string(json_writer *w, const char *s)
{
    before_value(w);
    if (!s) {
        sb_append(w->out, "null");
        return;
    }
    sb_append_char(w->out, '"');
    json_escape(w->out, s);
    sb_append_char(w->out, '"');
}

void json_int(json_writer *w, long v)
{
    before_value(w);
    sb_appendf(w->out, "%ld", v);
}

void json_bool(json_writer *w, int v)
{
    before_value(w);
    sb_append(w->out, v ? "true" : "false");
}
