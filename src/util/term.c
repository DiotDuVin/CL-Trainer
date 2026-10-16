/* term.c — voir term.h. */
#include "term.h"

#include <stdlib.h>
#include <unistd.h>

static int color_enabled = 0;

void term_init(int force)
{
    if (force >= 0) {
        color_enabled = force;
        return;
    }
    const char *no_color = getenv("NO_COLOR");
    color_enabled = isatty(STDOUT_FILENO) && !(no_color && no_color[0]);
}

const char *term(term_style s)
{
    /* Séquences d'échappement ANSI, comprises par tous les terminaux Linux. */
    static const char *const codes[] = {
        [T_RESET] = "\033[0m",   [T_BOLD] = "\033[1m",
        [T_DIM] = "\033[2m",     [T_RED] = "\033[31m",
        [T_GREEN] = "\033[32m",  [T_YELLOW] = "\033[33m",
        [T_CYAN] = "\033[36m",
    };
    return color_enabled ? codes[s] : "";
}
