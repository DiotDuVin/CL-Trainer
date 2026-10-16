/* testspec.c — parser du fichier tests.txt (voir testspec.h). */
#include "testspec.h"

#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "fs.h"
#include "strbuf.h"
#include "text.h"
#include "xalloc.h"

enum { B_NONE = -1, B_STDIN, B_STDOUT, B_ARGS, B_EXIT, B_COUNT };
static const char *const block_names[B_COUNT] = { "stdin", "stdout", "args", "exit" };

/* État du parser regroupé dans une structure : les fonctions auxiliaires
 * reçoivent un seul pointeur au lieu de six paramètres. */
typedef struct {
    test_suite *ts;
    const char *origin;
    test_case *cur;     /* test en cours, NULL avant le premier "=== test:" */
    int block;          /* bloc en cours (B_NONE si aucun) */
    int block_line;
    unsigned seen;      /* blocs déjà vus dans le test courant (1 bit chacun) */
    strbuf body;        /* contenu du bloc en cours */
} parser;

/* Retire les lignes vides finales : "a\n\n\n" -> "a\n", "\n" -> "". */
static char *finish_text(const strbuf *b)
{
    size_t n = b->len;
    while (n >= 1 && b->data[n - 1] == '\n' && (n == 1 || b->data[n - 2] == '\n'))
        n--;
    return xstrndup(sb_cstr(b), n);
}

static int close_block(parser *p)
{
    test_case *tc = p->cur;
    switch (p->block) {
    case B_STDIN:
        tc->input = finish_text(&p->body);
        break;
    case B_STDOUT:
        tc->expected = finish_text(&p->body);
        break;
    case B_ARGS: {
        /* Un argument par ligne : pas besoin de règles de guillemets comme
         * dans le shell, et un argument peut contenir des espaces. */
        const char *cursor = sb_cstr(&p->body), *line;
        size_t len;
        while (text_next_line(&cursor, &line, &len))
            if (len > 0)
                sv_push_n(&tc->args, line, len);
        break;
    }
    case B_EXIT: {
        long code;
        if (parse_long(sb_cstr(&p->body), p->body.len, 0, 255, &code) != 0) {
            err_set("%s:%d: le bloc exit doit contenir un entier entre 0 et 255",
                    p->origin, p->block_line);
            return -1;
        }
        tc->expected_exit = (int)code;
        break;
    }
    default:
        break;
    }
    p->block = B_NONE;
    sb_clear(&p->body);
    return 0;
}

static int close_test(parser *p)
{
    if (!p->cur)
        return 0;
    if (close_block(p) != 0)
        return -1;
    if (!(p->seen & (1u << B_STDOUT))) {
        err_set("%s:%d: le test « %s » n'a pas de bloc « --- stdout »",
                p->origin, p->cur->line, p->cur->name);
        return -1;
    }
    if (!p->cur->input)
        p->cur->input = xstrdup("");
    p->cur = NULL;
    return 0;
}

/* "=== test: nom [hidden]" */
static void open_test(parser *p, const char *rest, size_t n, int lineno)
{
    test_suite *ts = p->ts;
    ts->items = grow_array(ts->items, &ts->cap, ts->len + 1, sizeof *ts->items);
    test_case *tc = &ts->items[ts->len++];
    memset(tc, 0, sizeof *tc);
    tc->line = lineno;

    span_trim(&rest, &n);
    const char *tag = "[hidden]";
    size_t tag_len = strlen(tag);
    if (n >= tag_len && memcmp(rest + n - tag_len, tag, tag_len) == 0) {
        tc->hidden = 1;
        n -= tag_len;
        span_trim(&rest, &n);
    }
    if (n > 0)
        tc->name = xstrndup(rest, n);
    else {
        strbuf name = STRBUF_INIT;
        sb_appendf(&name, "test %zu", ts->len);
        tc->name = sb_release(&name);
    }
    p->cur = tc;
    p->seen = 0;
}

int testspec_parse(const char *text, const char *origin, test_suite *ts)
{
    memset(ts, 0, sizeof *ts);
    parser p = { ts, origin, NULL, B_NONE, 0, 0, STRBUF_INIT };
    const char *cursor = text, *line;
    size_t len;
    int lineno = 0;

    while (text_next_line(&cursor, &line, &len)) {
        lineno++;

        if (span_starts_with(line, len, "===")) {
            if (!span_starts_with(line, len, "=== test:")) {
                err_set("%s:%d: « === » réservé aux débuts de test (« === test: nom »)",
                        origin, lineno);
                goto fail;
            }
            if (close_test(&p) != 0)
                goto fail;
            open_test(&p, line + 9, len - 9, lineno);
            continue;
        }

        if (span_starts_with(line, len, "--- ") || span_eq(line, len, "---")) {
            const char *name = line + 3;
            size_t nn = len - 3;
            span_trim(&name, &nn);
            int b = -1;
            for (int i = 0; i < B_COUNT; i++)
                if (span_eq(name, nn, block_names[i]))
                    b = i;
            if (b < 0) {
                err_set("%s:%d: bloc inconnu « %.*s » (attendu : stdin, stdout, args, exit)",
                        origin, lineno, (int)nn, name);
                goto fail;
            }
            if (!p.cur) {
                err_set("%s:%d: bloc en dehors d'un test", origin, lineno);
                goto fail;
            }
            if (p.seen & (1u << b)) {
                err_set("%s:%d: bloc « %s » en double dans le test « %s »",
                        origin, lineno, block_names[b], p.cur->name);
                goto fail;
            }
            if (close_block(&p) != 0)
                goto fail;
            p.seen |= 1u << b;
            p.block = b;
            p.block_line = lineno;
            continue;
        }

        if (p.block == B_NONE) {
            if (span_is_blank(line, len))
                continue;
            err_set("%s:%d: texte en dehors d'un bloc (ajoutez « --- stdin », "
                    "« --- stdout »...)", origin, lineno);
            goto fail;
        }
        sb_append_n(&p.body, line, len);
        sb_append_char(&p.body, '\n');
    }

    if (close_test(&p) != 0)
        goto fail;
    if (ts->len == 0) {
        err_set("%s: aucun test (« === test: nom » attendu)", origin);
        goto fail;
    }
    sb_free(&p.body);
    return 0;

fail:
    sb_free(&p.body);
    testspec_free(ts);
    return -1;
}

int testspec_load(const char *path, test_suite *ts)
{
    strbuf text = STRBUF_INIT;
    int rc = fs_read_file(path, &text);
    if (rc == 0)
        rc = testspec_parse(sb_cstr(&text), path, ts);
    sb_free(&text);
    return rc;
}

void testspec_free(test_suite *ts)
{
    for (size_t i = 0; i < ts->len; i++) {
        test_case *tc = &ts->items[i];
        free(tc->name);
        free(tc->input);
        free(tc->expected);
        sv_free(&tc->args);
    }
    free(ts->items);
    memset(ts, 0, sizeof *ts);
}
