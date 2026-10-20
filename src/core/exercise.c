/* exercise.c — parser du fichier exercise.txt (voir exercise.h). */
#include "exercise.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "fs.h"
#include "strbuf.h"
#include "text.h"
#include "xalloc.h"

/* Les clés et sections connues sont dans des tables plutôt que dans une
 * cascade de if/strcmp : ajouter une clé = ajouter une ligne ici + un case. */
enum {
    K_ID, K_TITLE, K_LANG, K_LEVEL, K_TOPIC, K_TAGS, K_REQUIRES, K_KIND,
    K_FILES, K_TIMEOUT, K_ESTIMATED, K_COMPARE, K_SANITIZE, K_COUNT
};
static const char *const key_names[K_COUNT] = {
    "id", "title", "lang", "level", "topic", "tags", "requires", "kind",
    "files", "timeout_ms", "estimated_min", "compare", "sanitize",
};

enum { S_STATEMENT, S_HINTS, S_EXPLANATION, S_MISTAKES, S_COUNT };
static const char *const section_names[S_COUNT] = {
    "statement", "hints", "explanation", "common_mistakes",
};

static int find_name(const char *const names[], int count, const char *s, size_t n)
{
    for (int i = 0; i < count; i++)
        if (span_eq(s, n, names[i]))
            return i;
    return -1;
}

/* id : lettres minuscules, chiffres, '.', '_' et '-', sans ".." ni '.' aux
 * extrémités. Un id sert de nom de dossier : on refuse tout ce qui pourrait
 * former un chemin bizarre. */
static int valid_id(const char *s)
{
    size_t n = strlen(s);
    if (n == 0 || s[0] == '.' || s[n - 1] == '.' || strstr(s, ".."))
        return 0;
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];
        if (!islower(c) && !isdigit(c) && c != '.' && c != '_' && c != '-')
            return 0;
    }
    return 1;
}

static int valid_lang(const char *s)
{
    if (!*s)
        return 0;
    for (; *s; s++)
        if (!islower((unsigned char)*s))
            return 0;
    return 1;
}

/* Un fichier de l'élève doit rester dans son dossier : pas de chemin
 * absolu ni de composant "..". */
static int safe_relative_path(const char *s)
{
    if (!*s || s[0] == '/')
        return 0;
    const char *p = s;
    while (*p) {
        const char *slash = strchr(p, '/');
        size_t n = slash ? (size_t)(slash - p) : strlen(p);
        if (span_eq(p, n, "..") || n == 0)
            return 0;
        p += n;
        if (*p == '/')
            p++;
    }
    return 1;
}

/* Texte d'une section : sans lignes vides au début ni blancs à la fin. */
static char *block_text(const char *s, size_t n)
{
    for (;;) {
        const char *nl = memchr(s, '\n', n);
        size_t line_len = nl ? (size_t)(nl - s) : n;
        if (!nl || !span_is_blank(s, line_len))
            break;
        s = nl + 1;
        n -= line_len + 1;
    }
    while (n > 0 && isspace((unsigned char)s[n - 1]))
        n--;
    return xstrndup(s, n);
}

/* Les indices sont séparés par une ligne "---". */
static void split_hints(const strbuf *body, strvec *hints)
{
    const char *cursor = sb_cstr(body), *line = "";
    size_t len = 0;
    strbuf part = STRBUF_INIT;
    for (int more = 1; more; ) {
        more = text_next_line(&cursor, &line, &len);
        const char *t = line;
        size_t tn = len;
        if (more)
            span_trim(&t, &tn);
        if (!more || span_eq(t, tn, "---")) {
            char *hint = block_text(sb_cstr(&part), part.len);
            if (hint[0])
                sv_push_owned(hints, hint);
            else
                free(hint);
            sb_clear(&part);
        } else {
            sb_append_n(&part, line, len);
            sb_append_char(&part, '\n');
        }
    }
    sb_free(&part);
}

static void store_section(exercise *ex, int section, const strbuf *body)
{
    switch (section) {
    case S_STATEMENT:   ex->statement = block_text(sb_cstr(body), body->len); break;
    case S_EXPLANATION: ex->explanation = block_text(sb_cstr(body), body->len); break;
    case S_MISTAKES:    ex->common_mistakes = block_text(sb_cstr(body), body->len); break;
    case S_HINTS:       split_hints(body, &ex->hints); break;
    default: break;
    }
}

/* Interprète une ligne « clé: valeur ». Le message d'erreur ne contient pas
 * la position : l'appelant l'ajoute. */
static int parse_key(exercise *ex, int key, const char *v, size_t vn)
{
    long num;
    int is_list = (key == K_TAGS || key == K_REQUIRES || key == K_FILES);
    if (vn == 0 && !is_list) {
        err_set("valeur vide pour « %s »", key_names[key]);
        return -1;
    }

    switch (key) {
    case K_ID:
        ex->id = xstrndup(v, vn);
        if (!valid_id(ex->id)) {
            err_set("id invalide « %s » (minuscules, chiffres, '.', '_', '-')", ex->id);
            return -1;
        }
        break;
    case K_TITLE:
        ex->title = xstrndup(v, vn);
        break;
    case K_LANG:
        ex->lang = xstrndup(v, vn);
        if (!valid_lang(ex->lang)) {
            err_set("langage invalide « %s » (minuscules uniquement)", ex->lang);
            return -1;
        }
        break;
    case K_TOPIC:
        ex->topic = xstrndup(v, vn);
        break;
    case K_LEVEL:
        if (parse_long(v, vn, 1, 5, &num) != 0) {
            err_set("level doit être un entier entre 1 et 5");
            return -1;
        }
        ex->level = (int)num;
        break;
    case K_TAGS:
        split_list(v, vn, ',', &ex->tags);
        break;
    case K_REQUIRES:
        split_list(v, vn, ',', &ex->prereqs);
        break;
    case K_FILES:
        split_list(v, vn, ',', &ex->files);
        for (size_t i = 0; i < ex->files.len; i++) {
            if (!safe_relative_path(ex->files.items[i])) {
                err_set("fichier « %s » : chemin relatif sans « .. » attendu",
                        ex->files.items[i]);
                return -1;
            }
        }
        break;
    case K_KIND:
        if (span_eq(v, vn, "exercise"))
            ex->kind = KIND_EXERCISE;
        else if (span_eq(v, vn, "project"))
            ex->kind = KIND_PROJECT;
        else {
            err_set("kind doit valoir « exercise » ou « project »");
            return -1;
        }
        break;
    case K_TIMEOUT:
        if (parse_long(v, vn, 1, 120000, &num) != 0) {
            err_set("timeout_ms doit être un entier entre 1 et 120000");
            return -1;
        }
        ex->timeout_ms = num;
        break;
    case K_ESTIMATED:
        if (parse_long(v, vn, 1, 600, &num) != 0) {
            err_set("estimated_min doit être un entier entre 1 et 600");
            return -1;
        }
        ex->estimated_min = (int)num;
        break;
    case K_COMPARE:
        if (span_eq(v, vn, "normalized"))
            ex->compare = COMPARE_NORMALIZED;
        else if (span_eq(v, vn, "exact"))
            ex->compare = COMPARE_EXACT;
        else {
            err_set("compare doit valoir « normalized » ou « exact »");
            return -1;
        }
        break;
    case K_SANITIZE:
        if (span_eq(v, vn, "yes"))
            ex->sanitize = 1;
        else if (span_eq(v, vn, "no"))
            ex->sanitize = 0;
        else {
            err_set("sanitize doit valoir « yes » ou « no »");
            return -1;
        }
        break;
    default:
        break;
    }
    return 0;
}

int exercise_parse(const char *text, const char *origin, exercise *ex)
{
    memset(ex, 0, sizeof *ex);
    ex->timeout_ms = EXERCISE_DEFAULT_TIMEOUT_MS;
    ex->compare = COMPARE_NORMALIZED;
    ex->kind = KIND_EXERCISE;

    unsigned seen_keys = 0, seen_sections = 0; /* un bit par clé / section */
    int section = -1;                           /* -1 : encore dans l'en-tête */
    strbuf body = STRBUF_INIT;
    const char *cursor = text, *line;
    size_t len;
    int lineno = 0;

    while (text_next_line(&cursor, &line, &len)) {
        lineno++;

        /* Début de section : "## nom" (mais "### titre" reste du texte). */
        if (span_starts_with(line, len, "##") && (len == 2 || line[2] == ' ')) {
            if (section >= 0)
                store_section(ex, section, &body);
            const char *name = line + 2;
            size_t nn = len - 2;
            span_trim(&name, &nn);
            int idx = find_name(section_names, S_COUNT, name, nn);
            if (idx < 0) {
                err_set("%s:%d: section inconnue « %.*s » (attendu : statement, "
                        "hints, explanation, common_mistakes)",
                        origin, lineno, (int)nn, name);
                goto fail;
            }
            if (seen_sections & (1u << idx)) {
                err_set("%s:%d: section « %s » en double", origin, lineno,
                        section_names[idx]);
                goto fail;
            }
            seen_sections |= 1u << idx;
            section = idx;
            sb_clear(&body);
            continue;
        }

        if (section >= 0) { /* contenu de section : recopié tel quel */
            sb_append_n(&body, line, len);
            sb_append_char(&body, '\n');
            continue;
        }

        /* En-tête */
        if (span_is_blank(line, len))
            continue;
        const char *colon = memchr(line, ':', len);
        if (!colon) {
            err_set("%s:%d: ligne d'en-tête invalide (attendu « clé: valeur »)",
                    origin, lineno);
            goto fail;
        }
        const char *k = line, *v = colon + 1;
        size_t kn = (size_t)(colon - line), vn = len - kn - 1;
        span_trim(&k, &kn);
        span_trim(&v, &vn);
        int key = find_name(key_names, K_COUNT, k, kn);
        if (key < 0) {
            err_set("%s:%d: clé inconnue « %.*s »", origin, lineno, (int)kn, k);
            goto fail;
        }
        if (seen_keys & (1u << key)) {
            err_set("%s:%d: clé « %s » en double", origin, lineno, key_names[key]);
            goto fail;
        }
        seen_keys |= 1u << key;
        if (parse_key(ex, key, v, vn) != 0) {
            err_set("%s:%d: %s", origin, lineno, err_msg());
            goto fail;
        }
    }
    if (section >= 0)
        store_section(ex, section, &body);

    /* Vérifications globales */
    static const int required[] = { K_ID, K_TITLE, K_LANG, K_LEVEL, K_TOPIC };
    for (size_t i = 0; i < sizeof required / sizeof required[0]; i++) {
        if (!(seen_keys & (1u << required[i]))) {
            err_set("%s: clé obligatoire manquante : « %s »", origin,
                    key_names[required[i]]);
            goto fail;
        }
    }
    size_t lang_len = strlen(ex->lang);
    if (strncmp(ex->id, ex->lang, lang_len) != 0 || ex->id[lang_len] != '.') {
        err_set("%s: l'id « %s » doit commencer par « %s. »", origin, ex->id, ex->lang);
        goto fail;
    }
    if (!ex->statement || !ex->statement[0]) {
        err_set("%s: section « statement » manquante ou vide", origin);
        goto fail;
    }
    if (!ex->explanation || !ex->explanation[0]) {
        err_set("%s: section « explanation » manquante ou vide", origin);
        goto fail;
    }

    sb_free(&body);
    return 0;

fail:
    /* goto pour le nettoyage : c'est l'usage idiomatique en C (noyau Linux,
     * Git...) pour avoir un seul chemin de libération. */
    sb_free(&body);
    exercise_free(ex);
    return -1;
}

int exercise_load(const char *dir, exercise *ex)
{
    strbuf path = STRBUF_INIT, text = STRBUF_INIT;
    path_join(&path, dir, "exercise.txt");
    int rc = fs_read_file(path.data, &text);
    if (rc == 0)
        rc = exercise_parse(sb_cstr(&text), path.data, ex);
    if (rc == 0)
        ex->dir = xstrdup(dir);
    sb_free(&path);
    sb_free(&text);
    return rc;
}

void exercise_free(exercise *ex)
{
    free(ex->id);
    free(ex->title);
    free(ex->lang);
    free(ex->topic);
    free(ex->statement);
    free(ex->explanation);
    free(ex->common_mistakes);
    free(ex->dir);
    sv_free(&ex->tags);
    sv_free(&ex->prereqs);
    sv_free(&ex->files);
    sv_free(&ex->hints);
    memset(ex, 0, sizeof *ex);
}
