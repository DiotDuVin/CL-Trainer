/* xalloc.c — voir xalloc.h. */
#include "xalloc.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"

void *xmalloc(size_t size)
{
    /* malloc(0) peut légalement renvoyer NULL : on demande au moins 1 octet
     * pour que NULL signifie toujours « plus de mémoire ». */
    void *p = malloc(size ? size : 1);
    if (!p)
        die("mémoire épuisée (%zu octets demandés)", size);
    return p;
}

void *xcalloc(size_t count, size_t size)
{
    void *p = calloc(count ? count : 1, size ? size : 1);
    if (!p)
        die("mémoire épuisée");
    return p;
}

void *xrealloc(void *ptr, size_t size)
{
    void *p = realloc(ptr, size ? size : 1);
    if (!p)
        die("mémoire épuisée (%zu octets demandés)", size);
    return p;
}

char *xstrdup(const char *s)
{
    return xstrndup(s, strlen(s));
}

char *xstrndup(const char *s, size_t n)
{
    char *p = xmalloc(n + 1);
    memcpy(p, s, n);
    p[n] = '\0';
    return p;
}

void *grow_array(void *ptr, size_t *cap, size_t needed, size_t elem_size)
{
    if (needed <= *cap)
        return ptr;
    size_t new_cap = *cap ? *cap : 8;
    while (new_cap < needed) {
        if (new_cap > SIZE_MAX / 2)
            die("tableau trop grand");
        new_cap *= 2;
    }
    /* Vérifie que new_cap * elem_size ne déborde pas avant de multiplier. */
    if (elem_size && new_cap > SIZE_MAX / elem_size)
        die("tableau trop grand");
    ptr = xrealloc(ptr, new_cap * elem_size);
    *cap = new_cap;
    return ptr;
}
