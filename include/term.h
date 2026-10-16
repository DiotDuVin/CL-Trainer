/*
 * term.h — couleurs du terminal.
 *
 * Les couleurs ne sont activées que si la sortie est un terminal et que la
 * variable NO_COLOR n'est pas définie (convention https://no-color.org).
 * Quand elles sont désactivées, term() renvoie "" : le code appelant n'a
 * jamais besoin de tester quoi que ce soit.
 */
#ifndef TRAINER_TERM_H
#define TRAINER_TERM_H

typedef enum {
    T_RESET, T_BOLD, T_DIM, T_RED, T_GREEN, T_YELLOW, T_CYAN
} term_style;

/* force : -1 = détection automatique, 0 = jamais, 1 = toujours. */
void term_init(int force);
const char *term(term_style s);

#endif
