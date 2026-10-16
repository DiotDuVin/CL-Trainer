/* compare.c — voir compare.h. */
#include "compare.h"

#include <string.h>

#include "term.h"
#include "text.h"

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

static void print_line(FILE *f, const char *label, const char *line, size_t len, int present)
{
    fprintf(f, "      %s : ", label);
    if (!present) {
        fprintf(f, "%s(fin de la sortie)%s\n", term(T_DIM), term(T_RESET));
        return;
    }
    strbuf vis = STRBUF_INIT;
    escape_visible(&vis, line, len);
    /* Les guillemets « » montrent où la ligne commence et finit : un
     * espace en trop devient ainsi visible. */
    fprintf(f, "«%s»\n", sb_cstr(&vis));
    sb_free(&vis);
}

void compare_print_diff(FILE *f, const char *expected, const char *actual,
                        compare_mode mode)
{
    strbuf ne = STRBUF_INIT, na = STRBUF_INIT;
    if (mode == COMPARE_NORMALIZED) {
        compare_normalize(expected, &ne);
        compare_normalize(actual, &na);
    } else {
        sb_append(&ne, expected);
        sb_append(&na, actual);
    }

    if (na.len == 0) {
        fprintf(f, "    Le programme n'a rien affiché.\n");
        goto out;
    }

    const char *ce = sb_cstr(&ne), *ca = sb_cstr(&na);
    const char *le = "", *la = "";
    size_t ne_len = 0, na_len = 0;
    for (int lineno = 1; ; lineno++) {
        int has_e = text_next_line(&ce, &le, &ne_len);
        int has_a = text_next_line(&ca, &la, &na_len);
        if (!has_e && !has_a) {
            /* Toutes les lignes sont identiques : seule la fin diffère
             * (retour à la ligne final, \r...). Possible en mode exact. */
            fprintf(f, "    Les lignes sont identiques mais les fins de ligne diffèrent "
                       "(retour à la ligne final manquant ou en trop, \\r\\n ?).\n");
            break;
        }
        if (has_e && has_a && ne_len == na_len && memcmp(le, la, ne_len) == 0)
            continue;
        fprintf(f, "    Première différence à la ligne %d :\n", lineno);
        print_line(f, "attendu", le, ne_len, has_e);
        print_line(f, "obtenu ", la, na_len, has_a);
        break;
    }

out:
    sb_free(&ne);
    sb_free(&na);
}
