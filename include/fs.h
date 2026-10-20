/*
 * fs.h — accès au système de fichiers (POSIX).
 *
 * Tout le code dépendant du système de fichiers est regroupé ici : un futur
 * portage Windows natif ne toucherait que ce fichier et process.c.
 */
#ifndef TRAINER_FS_H
#define TRAINER_FS_H

#include <stddef.h>

#include "strbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Lit tout le fichier et l'ajoute à out. 0 / -1 (err_msg). */
int fs_read_file(const char *path, strbuf *out);
/* Crée ou écrase path avec len octets de data. 0 / -1. */
int fs_write_file(const char *path, const char *data, size_t len);

int fs_exists(const char *path);
int fs_is_dir(const char *path);
int fs_is_file(const char *path);

/* Équivalent de « mkdir -p ». 0 / -1. */
int fs_mkdir_p(const char *path);

/* Copie un fichier en conservant ses permissions. 0 / -1. */
int fs_copy_file(const char *src, const char *dst);
/* Copie récursivement le dossier src dans dst (créé si besoin).
 * Si overwrite == 0, les fichiers déjà présents dans dst sont conservés.
 * 0 / -1. */
int fs_copy_tree(const char *src, const char *dst, int overwrite);

/* Supprime path et tout son contenu (« rm -r »). Les liens symboliques
 * sont supprimés, jamais suivis. 0 / -1. */
int fs_remove_tree(const char *path);
/* Crée un dossier temporaire unique (dans $TMPDIR ou /tmp) dont le nom
 * commence par prefix. out reçoit son chemin (vidé avant). 0 / -1. */
int fs_make_temp_dir(const char *prefix, strbuf *out);

/* out = a + "/" + b (sans doubler le '/'). out est vidé avant. */
void path_join(strbuf *out, const char *a, const char *b);
/* Chemin absolu, sans exiger que le chemin existe (contrairement à
 * realpath). out est vidé avant. 0 / -1. */
int fs_absolute(const char *path, strbuf *out);

#ifdef __cplusplus
}
#endif

#endif
