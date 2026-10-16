/* error.c — voir error.h. */
#include "error.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERR_SIZE 2048

/* Variable globale « privée » (static) : invisible hors de ce fichier.
 * Un programme multithreadé utiliserait _Thread_local ; ce n'est pas notre cas. */
static char err_buf[ERR_SIZE];

/* Formate dans un tampon temporaire puis copie : ainsi err_set("x: %s",
 * err_msg()) fonctionne. Sans cela, vsnprintf lirait et écrirait le même
 * tableau en même temps, ce qui est un comportement indéfini. */
ATTR_PRINTF(1, 0)
static void err_vset(const char *fmt, va_list ap, int with_errno, int saved)
{
    char tmp[ERR_SIZE];
    int n = vsnprintf(tmp, sizeof tmp, fmt, ap);
    if (n < 0)
        n = 0;
    if (with_errno && (size_t)n < sizeof tmp)
        snprintf(tmp + n, sizeof tmp - (size_t)n, " : %s", strerror(saved));
    memcpy(err_buf, tmp, sizeof err_buf);
}

void err_set(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    err_vset(fmt, ap, 0, 0);
    va_end(ap);
}

void err_set_errno(const char *fmt, ...)
{
    int saved = errno; /* à capturer tout de suite : printf peut modifier errno */
    va_list ap;
    va_start(ap, fmt);
    err_vset(fmt, ap, 1, saved);
    va_end(ap);
}

const char *err_msg(void)
{
    return err_buf[0] ? err_buf : "erreur inconnue";
}

void die(const char *fmt, ...)
{
    va_list ap;
    fputs("trainer: erreur fatale : ", stderr);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(2);
}
