/* lesson.c — voir lesson.h. */
#include "lesson.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "fs.h"
#include "strbuf.h"
#include "text.h"
#include "xalloc.h"

static int valid_id(const char *s)
{
    if (!*s || s[0] == '.' || strstr(s, ".."))
        return 0;
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        if (!islower(c) && !isdigit(c) && c != '.' && c != '_' && c != '-')
            return 0;
    }
    return 1;
}

int lesson_parse(const char *text, const char *origin, lesson *l)
{
    memset(l, 0, sizeof *l);
    const char *cursor = text, *line;
    size_t len;
    int lineno = 0, in_body = 0;
    long num;

    /* En-tête jusqu'à la ligne « --- », puis le corps tel quel. */
    while (!in_body && text_next_line(&cursor, &line, &len)) {
        lineno++;
        const char *t = line;
        size_t tn = len;
        span_trim(&t, &tn);
        if (tn == 0)
            continue;
        if (span_eq(t, tn, "---")) {
            in_body = 1;
            break;
        }
        const char *colon = memchr(t, ':', tn);
        if (!colon) {
            err_set("%s:%d: ligne d'en-tête invalide (attendu « clé: valeur » ou « --- »)",
                    origin, lineno);
            goto fail;
        }
        const char *k = t, *v = colon + 1;
        size_t kn = (size_t)(colon - t), vn = tn - kn - 1;
        span_trim(&k, &kn);
        span_trim(&v, &vn);
        char **field = NULL;
        if (span_eq(k, kn, "id"))
            field = &l->id;
        else if (span_eq(k, kn, "title"))
            field = &l->title;
        else if (span_eq(k, kn, "lang"))
            field = &l->lang;
        else if (span_eq(k, kn, "topic"))
            field = &l->topic;
        else if (span_eq(k, kn, "estimated_min")) {
            if (parse_long(v, vn, 1, 600, &num) != 0) {
                err_set("%s:%d: estimated_min doit être un entier entre 1 et 600",
                        origin, lineno);
                goto fail;
            }
            l->estimated_min = (int)num;
            continue;
        } else {
            err_set("%s:%d: clé inconnue « %.*s »", origin, lineno, (int)kn, k);
            goto fail;
        }
        if (*field) {
            err_set("%s:%d: clé « %.*s » en double", origin, lineno, (int)kn, k);
            goto fail;
        }
        if (vn == 0) {
            err_set("%s:%d: valeur vide pour « %.*s »", origin, lineno, (int)kn, k);
            goto fail;
        }
        *field = xstrndup(v, vn);
    }
    if (!in_body) {
        err_set("%s: ligne « --- » manquante entre l'en-tête et le cours", origin);
        goto fail;
    }

    /* Corps : sans lignes vides au début ni blancs à la fin. */
    l->body_line = lineno + 1;
    while (*cursor == '\n' || *cursor == '\r' || *cursor == ' ' || *cursor == '\t') {
        if (*cursor == '\n')
            l->body_line++;
        cursor++;
    }
    size_t blen = strlen(cursor);
    while (blen > 0 && isspace((unsigned char)cursor[blen - 1]))
        blen--;
    l->body = xstrndup(cursor, blen);

    if (!l->id || !l->title || !l->lang || !l->topic) {
        err_set("%s: clés obligatoires : id, title, lang, topic", origin);
        goto fail;
    }
    size_t lang_len = strlen(l->lang);
    if (!valid_id(l->id) || strncmp(l->id, l->lang, lang_len) != 0 || l->id[lang_len] != '.') {
        err_set("%s: id « %s » invalide (minuscules, commence par « %s. »)", origin, l->id,
                l->lang);
        goto fail;
    }
    if (l->body[0] == '\0') {
        err_set("%s: le cours est vide", origin);
        goto fail;
    }
    return 0;

fail:
    lesson_free(l);
    return -1;
}

int lesson_load(const char *dir, lesson *l)
{
    strbuf path = STRBUF_INIT, text = STRBUF_INIT;
    path_join(&path, dir, "lesson.md");
    int rc = fs_read_file(path.data, &text);
    if (rc == 0)
        rc = lesson_parse(sb_cstr(&text), path.data, l);
    if (rc == 0)
        l->dir = xstrdup(dir);
    sb_free(&path);
    sb_free(&text);
    return rc;
}

void lesson_free(lesson *l)
{
    free(l->id);
    free(l->title);
    free(l->lang);
    free(l->topic);
    free(l->body);
    free(l->dir);
    memset(l, 0, sizeof *l);
}

/* Lit un bloc de code jusqu'à la ligne « ``` » fermante. */
static char *read_fence(const char **cursor, int *lineno)
{
    strbuf sb = STRBUF_INIT;
    const char *line;
    size_t len;
    while (text_next_line(cursor, &line, &len)) {
        (*lineno)++;
        if (span_starts_with(line, len, "```"))
            break;
        sb_append_n(&sb, line, len);
        sb_append_char(&sb, '\n');
    }
    return sb_release(&sb);
}

void lesson_extract_examples(const lesson *l, lesson_examples *out)
{
    memset(out, 0, sizeof *out);
    strbuf marker = STRBUF_INIT;
    sb_appendf(&marker, "```%s example", l->lang);

    /* Numéros de ligne du fichier : le corps commence à body_line. */
    const char *cursor = l->body, *line;
    size_t len;
    int lineno = l->body_line > 0 ? l->body_line - 1 : 0;
    while (text_next_line(&cursor, &line, &len)) {
        lineno++;
        const char *t = line;
        size_t tn = len;
        span_trim(&t, &tn);
        if (!span_eq(t, tn, marker.data))
            continue;
        out->items = grow_array(out->items, &out->cap, out->len + 1, sizeof *out->items);
        lesson_example *ex = &out->items[out->len++];
        memset(ex, 0, sizeof *ex);
        ex->line = lineno;
        ex->code = read_fence(&cursor, &lineno);

        /* Un bloc ```output juste après (lignes vides permises) ? */
        const char *peek = cursor;
        int peek_line = lineno;
        while (text_next_line(&peek, &line, &len)) {
            peek_line++;
            t = line;
            tn = len;
            span_trim(&t, &tn);
            if (tn == 0)
                continue;
            if (span_eq(t, tn, "```output")) {
                cursor = peek;
                lineno = peek_line;
                ex->expected = read_fence(&cursor, &lineno);
            }
            break;
        }
    }
    sb_free(&marker);
}

void lesson_examples_free(lesson_examples *ex)
{
    for (size_t i = 0; i < ex->len; i++) {
        free(ex->items[i].code);
        free(ex->items[i].expected);
    }
    free(ex->items);
    memset(ex, 0, sizeof *ex);
}
