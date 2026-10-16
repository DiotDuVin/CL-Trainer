/* strbuf.c — voir strbuf.h. */
#include "strbuf.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xalloc.h"

/* Garantit la place pour `extra` octets de plus + le '\0'. */
static void sb_grow(strbuf *sb, size_t extra)
{
    if (extra > SIZE_MAX - sb->len - 1)
        die("chaîne trop longue");
    size_t needed = sb->len + extra + 1;
    if (needed <= sb->cap)
        return;
    /* Croissance géométrique (x2) : n ajouts coûtent O(n) au total,
     * contre O(n²) si on agrandissait d'un octet à la fois. */
    sb->data = grow_array(sb->data, &sb->cap, needed, 1);
}

void sb_append_n(strbuf *sb, const char *s, size_t n)
{
    sb_grow(sb, n);
    memcpy(sb->data + sb->len, s, n);
    sb->len += n;
    sb->data[sb->len] = '\0';
}

void sb_append(strbuf *sb, const char *s)
{
    sb_append_n(sb, s, strlen(s));
}

void sb_append_char(strbuf *sb, char c)
{
    sb_append_n(sb, &c, 1);
}

void sb_appendf(strbuf *sb, const char *fmt, ...)
{
    va_list ap, ap2;
    va_start(ap, fmt);
    /* Un va_list ne peut être parcouru qu'une fois : on en garde une copie
     * pour le second passage. */
    va_copy(ap2, ap);
    int n = vsnprintf(NULL, 0, fmt, ap); /* 1er passage : mesure la taille */
    va_end(ap);
    if (n < 0)
        die("format invalide dans sb_appendf");
    sb_grow(sb, (size_t)n);
    vsnprintf(sb->data + sb->len, (size_t)n + 1, fmt, ap2); /* 2e : écrit */
    va_end(ap2);
    sb->len += (size_t)n;
}

void sb_truncate(strbuf *sb, size_t len)
{
    if (len < sb->len) {
        sb->len = len;
        sb->data[len] = '\0';
    }
}

void sb_clear(strbuf *sb)
{
    sb_truncate(sb, 0);
}

void sb_free(strbuf *sb)
{
    free(sb->data);
    sb->data = NULL;
    sb->len = sb->cap = 0;
}

const char *sb_cstr(const strbuf *sb)
{
    return sb->data ? sb->data : "";
}

char *sb_release(strbuf *sb)
{
    char *s = sb->data ? sb->data : xstrdup("");
    sb->data = NULL;
    sb->len = sb->cap = 0;
    return s;
}
