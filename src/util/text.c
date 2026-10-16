/* text.c — voir text.h. */
#include "text.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int text_next_line(const char **cursor, const char **line, size_t *len)
{
    const char *p = *cursor;
    if (*p == '\0')
        return 0;
    const char *nl = strchr(p, '\n');
    size_t n = nl ? (size_t)(nl - p) : strlen(p);
    *line = p;
    /* Fichiers écrits sous Windows : on ignore le '\r' de "\r\n". */
    *len = (n > 0 && p[n - 1] == '\r') ? n - 1 : n;
    *cursor = nl ? nl + 1 : p + n;
    return 1;
}

int span_eq(const char *s, size_t n, const char *lit)
{
    size_t m = strlen(lit);
    return n == m && memcmp(s, lit, m) == 0;
}

int span_starts_with(const char *s, size_t n, const char *prefix)
{
    size_t m = strlen(prefix);
    return n >= m && memcmp(s, prefix, m) == 0;
}

int span_is_blank(const char *s, size_t n)
{
    for (size_t i = 0; i < n; i++)
        if (s[i] != ' ' && s[i] != '\t')
            return 0;
    return 1;
}

void span_trim(const char **s, size_t *n)
{
    /* Le cast en unsigned char est obligatoire pour les fonctions de
     * <ctype.h> : passer un char négatif (octet UTF-8 > 127) est indéfini. */
    while (*n > 0 && isspace((unsigned char)**s)) {
        (*s)++;
        (*n)--;
    }
    while (*n > 0 && isspace((unsigned char)(*s)[*n - 1]))
        (*n)--;
}

int parse_long(const char *s, size_t n, long min, long max, long *out)
{
    char buf[32];
    span_trim(&s, &n);
    if (n == 0 || n >= sizeof buf)
        return -1;
    memcpy(buf, s, n);
    buf[n] = '\0';

    /* strtol est la seule fonction standard qui détecte proprement les
     * erreurs (atoi renvoie 0 sur "abc", indistinguable de "0"). */
    char *end;
    errno = 0;
    long v = strtol(buf, &end, 10);
    if (errno != 0 || *end != '\0' || v < min || v > max)
        return -1;
    *out = v;
    return 0;
}

void split_list(const char *s, size_t n, char sep, strvec *out)
{
    size_t start = 0;
    for (size_t i = 0; i <= n; i++) {
        if (i == n || s[i] == sep) {
            const char *item = s + start;
            size_t len = i - start;
            span_trim(&item, &len);
            if (len > 0)
                sv_push_n(out, item, len);
            start = i + 1;
        }
    }
}

void json_escape(strbuf *out, const char *s)
{
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        switch (c) {
        case '"':  sb_append(out, "\\\""); break;
        case '\\': sb_append(out, "\\\\"); break;
        case '\n': sb_append(out, "\\n"); break;
        case '\t': sb_append(out, "\\t"); break;
        case '\r': sb_append(out, "\\r"); break;
        default:
            if (c < 0x20)
                sb_appendf(out, "\\u%04x", c);
            else
                sb_append_char(out, (char)c);
        }
    }
}

void escape_visible(strbuf *out, const char *s, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == '\t')
            sb_append(out, "\\t");
        else if (c == '\r')
            sb_append(out, "\\r");
        else if (c < 0x20 || c == 0x7f)
            sb_appendf(out, "\\x%02x", c);
        else
            sb_append_char(out, (char)c); /* UTF-8 laissé tel quel */
    }
}
