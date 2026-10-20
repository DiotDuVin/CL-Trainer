/* compare.c — voir compare.h. */
#include "compare.h"

#include <stdlib.h>
#include <string.h>

#include "text.h"
#include "xalloc.h"

void compare_normalize(const char *s, strbuf *out)
{
    size_t keep = out->len; /* longueur juste après la dernière ligne non vide */
    const char *cursor = s, *line;
    size_t len;
    while (text_next_line(&cursor, &line, &len)) {
        while (len > 0 && (line[len - 1] == ' ' || line[len - 1] == '\t' ||
                           line[len - 1] == '\r'))
            len--;
        sb_append_n(out, line, len);
        sb_append_char(out, '\n');
        if (len > 0)
            keep = out->len;
    }
    sb_truncate(out, keep);
}

int compare_outputs(const char *expected, const char *actual, compare_mode mode)
{
    if (mode == COMPARE_EXACT)
        return strcmp(expected, actual) == 0;

    strbuf a = STRBUF_INIT, b = STRBUF_INIT;
    compare_normalize(expected, &a);
    compare_normalize(actual, &b);
    int equal = strcmp(sb_cstr(&a), sb_cstr(&b)) == 0;
    sb_free(&a);
    sb_free(&b);
    return equal;
}

void compare_first_diff(const char *expected, const char *actual, compare_mode mode,
                        output_diff *d)
{
    memset(d, 0, sizeof *d);
    strbuf ne = STRBUF_INIT, na = STRBUF_INIT;
    if (mode == COMPARE_NORMALIZED) {
        compare_normalize(expected, &ne);
        compare_normalize(actual, &na);
    } else {
        sb_append(&ne, expected);
        sb_append(&na, actual);
    }

    if (na.len == 0) {
        d->empty_output = 1;
        goto out;
    }

    const char *ce = sb_cstr(&ne), *ca = sb_cstr(&na);
    const char *le = "", *la = "";
    size_t ne_len = 0, na_len = 0;
    for (int lineno = 1; ; lineno++) {
        int has_e = text_next_line(&ce, &le, &ne_len);
        int has_a = text_next_line(&ca, &la, &na_len);
        if (!has_e && !has_a) {
            d->same_lines = 1;
            break;
        }
        if (has_e && has_a && ne_len == na_len && memcmp(le, la, ne_len) == 0)
            continue;
        d->line = lineno;
        d->expected_line = has_e ? xstrndup(le, ne_len) : NULL;
        d->actual_line = has_a ? xstrndup(la, na_len) : NULL;
        break;
    }

out:
    sb_free(&ne);
    sb_free(&na);
}

void output_diff_free(output_diff *d)
{
    free(d->expected_line);
    free(d->actual_line);
    memset(d, 0, sizeof *d);
}
