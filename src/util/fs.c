/* fs.c — voir fs.h. */
#include "fs.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "error.h"

int fs_read_file(const char *path, strbuf *out)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        err_set_errno("impossible d'ouvrir %s", path);
        return -1;
    }
    char chunk[8192];
    size_t n;
    while ((n = fread(chunk, 1, sizeof chunk, f)) > 0)
        sb_append_n(out, chunk, n);
    /* fread renvoie 0 en fin de fichier ET en cas d'erreur : ferror tranche. */
    int failed = ferror(f);
    fclose(f);
    if (failed) {
        err_set("erreur de lecture de %s", path);
        return -1;
    }
    return 0;
}

int fs_write_file(const char *path, const char *data, size_t len)
{
    FILE *f = fopen(path, "wb");
    if (!f) {
        err_set_errno("impossible de créer %s", path);
        return -1;
    }
    size_t written = fwrite(data, 1, len, f);
    /* fclose peut échouer (disque plein au moment du vidage du tampon) :
     * il faut vérifier son retour, c'est un oubli classique. */
    if (fclose(f) != 0 || written != len) {
        err_set_errno("erreur d'écriture dans %s", path);
        return -1;
    }
    return 0;
}

int fs_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

int fs_is_dir(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

int fs_is_file(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

int fs_mkdir_p(const char *path)
{
    if (path[0] == '\0') {
        err_set("chemin de dossier vide");
        return -1;
    }
    strbuf tmp = STRBUF_INIT;
    sb_append(&tmp, path);
    char *p = tmp.data;
    int rc = 0;

    /* On crée chaque préfixe "a", "a/b", "a/b/c" en coupant temporairement
     * la chaîne à chaque '/'. EEXIST n'est pas une erreur. */
    for (char *s = p + 1; ; s++) {
        if (*s == '/' || *s == '\0') {
            char saved = *s;
            *s = '\0';
            if (mkdir(p, 0755) != 0 && errno != EEXIST) {
                err_set_errno("impossible de créer le dossier %s", p);
                rc = -1;
            } else if (!fs_is_dir(p)) {
                err_set("%s existe mais n'est pas un dossier", p);
                rc = -1;
            }
            *s = saved;
            if (rc != 0 || saved == '\0')
                break;
        }
    }
    sb_free(&tmp);
    return rc;
}

int fs_copy_file(const char *src, const char *dst)
{
    int in = open(src, O_RDONLY);
    if (in < 0) {
        err_set_errno("impossible d'ouvrir %s", src);
        return -1;
    }
    struct stat st;
    if (fstat(in, &st) != 0) {
        err_set_errno("stat %s", src);
        close(in);
        return -1;
    }
    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode & 0777);
    if (out < 0) {
        err_set_errno("impossible de créer %s", dst);
        close(in);
        return -1;
    }

    char buf[65536];
    int rc = 0;
    for (;;) {
        ssize_t n = read(in, buf, sizeof buf);
        if (n == 0)
            break;
        if (n < 0) {
            if (errno == EINTR)
                continue;
            err_set_errno("lecture de %s", src);
            rc = -1;
            break;
        }
        /* write peut écrire moins que demandé : on boucle jusqu'au bout. */
        for (ssize_t off = 0; off < n; ) {
            ssize_t w = write(out, buf + off, (size_t)(n - off));
            if (w < 0) {
                if (errno == EINTR)
                    continue;
                err_set_errno("écriture de %s", dst);
                rc = -1;
                break;
            }
            off += w;
        }
        if (rc != 0)
            break;
    }
    close(in);
    if (close(out) != 0 && rc == 0) {
        err_set_errno("écriture de %s", dst);
        rc = -1;
    }
    return rc;
}

int fs_copy_tree(const char *src, const char *dst, int overwrite)
{
    if (fs_mkdir_p(dst) != 0)
        return -1;
    DIR *dir = opendir(src);
    if (!dir) {
        err_set_errno("impossible d'ouvrir le dossier %s", src);
        return -1;
    }

    strbuf from = STRBUF_INIT, to = STRBUF_INIT;
    int rc = 0;
    struct dirent *ent;
    while (rc == 0 && (ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;
        path_join(&from, src, ent->d_name);
        path_join(&to, dst, ent->d_name);
        if (fs_is_dir(from.data))
            rc = fs_copy_tree(from.data, to.data, overwrite);
        else if (overwrite || !fs_exists(to.data))
            rc = fs_copy_file(from.data, to.data);
    }
    closedir(dir);
    sb_free(&from);
    sb_free(&to);
    return rc;
}

int fs_remove_tree(const char *path)
{
    struct stat st;
    /* lstat et pas stat : sur un lien symbolique, lstat décrit le lien
     * lui-même. Suivre un lien vers /home serait catastrophique ici. */
    if (lstat(path, &st) != 0) {
        if (errno == ENOENT)
            return 0;
        err_set_errno("stat %s", path);
        return -1;
    }
    if (!S_ISDIR(st.st_mode)) {
        if (unlink(path) != 0) {
            err_set_errno("impossible de supprimer %s", path);
            return -1;
        }
        return 0;
    }
    DIR *dir = opendir(path);
    if (!dir) {
        err_set_errno("impossible d'ouvrir le dossier %s", path);
        return -1;
    }
    strbuf child = STRBUF_INIT;
    int rc = 0;
    struct dirent *ent;
    while (rc == 0 && (ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;
        path_join(&child, path, ent->d_name);
        rc = fs_remove_tree(child.data);
    }
    closedir(dir);
    sb_free(&child);
    if (rc == 0 && rmdir(path) != 0) {
        err_set_errno("impossible de supprimer %s", path);
        rc = -1;
    }
    return rc;
}

int fs_make_temp_dir(const char *prefix, strbuf *out)
{
    const char *tmp = getenv("TMPDIR");
    sb_clear(out);
    sb_appendf(out, "%s/%s-XXXXXX", (tmp && *tmp) ? tmp : "/tmp", prefix);
    /* mkdtemp remplace les X par des caractères aléatoires et crée le
     * dossier de façon atomique (pas de course avec un autre programme). */
    if (!mkdtemp(out->data)) {
        err_set_errno("impossible de créer un dossier temporaire %s", out->data);
        return -1;
    }
    return 0;
}

void path_join(strbuf *out, const char *a, const char *b)
{
    sb_clear(out);
    sb_append(out, a);
    if (out->len > 0 && out->data[out->len - 1] != '/')
        sb_append_char(out, '/');
    sb_append(out, b);
}

int fs_absolute(const char *path, strbuf *out)
{
    sb_clear(out);
    if (path[0] == '/') {
        sb_append(out, path);
        return 0;
    }
    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof cwd)) {
        err_set_errno("getcwd");
        return -1;
    }
    path_join(out, cwd, path);
    return 0;
}
