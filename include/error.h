/*
 * error.h — gestion d'erreurs centralisée.
 *
 * Convention du projet : une fonction qui peut échouer renvoie 0 en cas de
 * succès et -1 en cas d'erreur, après avoir enregistré un message lisible
 * avec err_set(). L'appelant peut alors afficher err_msg() ou l'enrichir :
 *
 *     if (fs_read_file(path, &buf) != 0) {
 *         err_set("chargement de l'exercice : %s", err_msg());
 *         return -1;
 *     }
 *
 * C'est le même principe que errno/strerror, mais avec un message complet
 * (chemin, numéro de ligne...) au lieu d'un simple code.
 */
#ifndef TRAINER_ERROR_H
#define TRAINER_ERROR_H

#if defined(__GNUC__)
#define ATTR_PRINTF(fmt, args) __attribute__((format(printf, fmt, args)))
#define ATTR_NORETURN __attribute__((noreturn))
#else
#define ATTR_PRINTF(fmt, args)
#define ATTR_NORETURN
#endif

/* Enregistre un message d'erreur (format printf). Les arguments peuvent
 * contenir err_msg() lui-même : le message est formaté dans un tampon
 * temporaire avant d'être copié. */
void err_set(const char *fmt, ...) ATTR_PRINTF(1, 2);

/* Comme err_set, mais ajoute " : <strerror(errno)>" à la fin. */
void err_set_errno(const char *fmt, ...) ATTR_PRINTF(1, 2);

/* Message de la dernière erreur (jamais NULL). */
const char *err_msg(void);

/* Erreur irrécupérable (bug interne, mémoire épuisée) : affiche et quitte. */
void die(const char *fmt, ...) ATTR_PRINTF(1, 2) ATTR_NORETURN;

#endif
