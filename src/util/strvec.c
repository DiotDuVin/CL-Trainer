/* strvec.c — voir strvec.h. */
#include "strvec.h"

#include <stdlib.h>
#include <string.h>

#include "xalloc.h"

void sv_push_owned(strvec *sv, char *s)
{
    /* +2 : le nouvel élément et la sentinelle NULL. */
    sv->items = grow_array(sv->items, &sv->cap, sv->len + 2, sizeof *sv->items);
    sv->items[sv->len++] = s;
    sv->items[sv->len] = NULL;
}

void sv_push(strvec *sv, const char *s)
{
    sv_push_owned(sv, xstrdup(s));
}

void sv_push_n(strvec *sv, const char *s, size_t n)
{
    sv_push_owned(sv, xstrndup(s, n));
}

int sv_contains(const strvec *sv, const char *s)
{
    for (size_t i = 0; i < sv->len; i++)
        if (strcmp(sv->items[i], s) == 0)
            return 1;
    return 0;
}

void sv_free(strvec *sv)
{
    for (size_t i = 0; i < sv->len; i++)
        free(sv->items[i]);
    free(sv->items);
    sv->items = NULL;
    sv->len = sv->cap = 0;
}
