/*
 * xalloc.h — allocation mémoire « qui ne peut pas échouer ».
 *
 * Choix de conception : si malloc renvoie NULL, on arrête le programme avec
 * un message clair au lieu de propager l'erreur. Pour un outil en ligne de
 * commande, la mémoire épuisée est irrécupérable en pratique, et vérifier
 * chaque allocation alourdirait tout le code. Git fait exactement ça
 * (xmalloc dans wrapper.c). Une bibliothèque, elle, devrait propager l'erreur.
 */
#ifndef TRAINER_XALLOC_H
#define TRAINER_XALLOC_H

#include <stddef.h>

void *xmalloc(size_t size);
void *xcalloc(size_t count, size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *s);
char *xstrndup(const char *s, size_t n); /* copie n octets + '\0' */

/* Agrandit un tableau dynamique : garantit *cap >= needed (croissance x2).
 * Renvoie le nouveau pointeur. Usage : arr = grow_array(arr, &cap, n + 1, sizeof *arr); */
void *grow_array(void *ptr, size_t *cap, size_t needed, size_t elem_size);

#endif
