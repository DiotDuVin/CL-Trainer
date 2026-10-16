/* test_util.c — tests de strbuf, text et fs. */
#include <stdlib.h>
#include <unistd.h>

#include "fs.h"
#include "strbuf.h"
#include "strvec.h"
#include "test.h"
#include "text.h"

void test_strbuf_suite(void)
{
    strbuf sb = STRBUF_INIT;
    CHECK_STR(sb_cstr(&sb), ""); /* jamais NULL, même vide */
    sb_append(&sb, "abc");
    sb_append_char(&sb, '-');
    sb_appendf(&sb, "%d/%s", 42, "x");
    CHECK_STR(sb_cstr(&sb), "abc-42/x");
    CHECK_INT(sb.len, 8);
    sb_truncate(&sb, 3);
    CHECK_STR(sb_cstr(&sb), "abc");

    /* Croissance : beaucoup d'ajouts successifs. */
    sb_clear(&sb);
    for (int i = 0; i < 10000; i++)
        sb_append_char(&sb, 'a');
    CHECK_INT(sb.len, 10000);
    CHECK(sb.cap > sb.len);

    char *owned = sb_release(&sb);
    CHECK(sb.data == NULL && sb.len == 0);
    CHECK_INT(owned[9999], 'a');
    free(owned);
    sb_free(&sb); /* sans effet, mais doit être sûr */
}

void test_text_suite(void)
{
    const char *text = "a\r\nbb\n\nlast", *cursor = text, *line;
    size_t len;
    CHECK(text_next_line(&cursor, &line, &len) && span_eq(line, len, "a"));
    CHECK(text_next_line(&cursor, &line, &len) && span_eq(line, len, "bb"));
    CHECK(text_next_line(&cursor, &line, &len) && len == 0);
    CHECK(text_next_line(&cursor, &line, &len) && span_eq(line, len, "last"));
    CHECK(!text_next_line(&cursor, &line, &len));

    const char *s = "  x y \t";
    size_t n = strlen(s);
    span_trim(&s, &n);
    CHECK(span_eq(s, n, "x y"));

    long v;
    CHECK(parse_long("42", 2, 0, 100, &v) == 0 && v == 42);
    CHECK(parse_long(" -7 ", 4, -10, 10, &v) == 0 && v == -7);
    CHECK(parse_long("101", 3, 0, 100, &v) != 0); /* hors bornes */
    CHECK(parse_long("4x", 2, 0, 100, &v) != 0);  /* caractère parasite */
    CHECK(parse_long("", 0, 0, 100, &v) != 0);

    strvec sv = STRVEC_INIT;
    const char *list = " a, b ,,c ";
    split_list(list, strlen(list), ',', &sv);
    CHECK_INT(sv.len, 3);
    CHECK_STR(sv.items[1], "b");
    CHECK(sv.items[3] == NULL); /* sentinelle pour execvp */
    sv_free(&sv);

    strbuf sb = STRBUF_INIT;
    json_escape(&sb, "a\"b\\c\n");
    CHECK_STR(sb_cstr(&sb), "a\\\"b\\\\c\\n");
    sb_clear(&sb);
    escape_visible(&sb, "x\ty\r", 4);
    CHECK_STR(sb_cstr(&sb), "x\\ty\\r");
    sb_free(&sb);
}

void test_fs_suite(void)
{
    char tmpl[] = "/tmp/trainer-test-XXXXXX";
    char *root = mkdtemp(tmpl);
    CHECK(root != NULL);
    if (!root)
        return;

    strbuf a = STRBUF_INIT, b = STRBUF_INIT, content = STRBUF_INIT;
    path_join(&a, root, "src/sub");
    CHECK(fs_mkdir_p(a.data) == 0);
    CHECK(fs_mkdir_p(a.data) == 0); /* idempotent */
    CHECK(fs_is_dir(a.data));

    path_join(&b, a.data, "f.txt");
    CHECK(fs_write_file(b.data, "hello\n", 6) == 0);
    CHECK(fs_read_file(b.data, &content) == 0);
    CHECK_STR(sb_cstr(&content), "hello\n");

    /* copy_tree sans écrasement : un fichier existant est conservé. */
    path_join(&a, root, "src");
    path_join(&b, root, "dst");
    CHECK(fs_copy_tree(a.data, b.data, 0) == 0);
    path_join(&b, root, "dst/sub/f.txt");
    CHECK(fs_is_file(b.data));
    CHECK(fs_write_file(b.data, "modifié", 8) == 0);
    path_join(&b, root, "dst");
    CHECK(fs_copy_tree(a.data, b.data, 0) == 0);
    path_join(&b, root, "dst/sub/f.txt");
    sb_clear(&content);
    CHECK(fs_read_file(b.data, &content) == 0);
    CHECK_STR(sb_cstr(&content), "modifié");

    CHECK(fs_read_file("/inexistant/zzz", &content) != 0);

    path_join(&a, "dir/", "x");
    CHECK_STR(sb_cstr(&a), "dir/x");

    /* Nettoyage (rm -r) via le shell : acceptable dans un test. */
    sb_clear(&a);
    sb_appendf(&a, "rm -rf '%s'", root);
    CHECK(system(a.data) == 0);
    sb_free(&a);
    sb_free(&b);
    sb_free(&content);
}
