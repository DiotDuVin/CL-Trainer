/* runner.c — registre des runners (voir runner.h). */
#include "runner.h"

#include <string.h>

/* Chaque runner est défini dans src/runners/runner_<lang>.c. */
extern const runner runner_cpp;

static const runner *const runners[] = {
    &runner_cpp,
};

const runner *runner_for(const char *lang)
{
    for (size_t i = 0; i < sizeof runners / sizeof runners[0]; i++)
        if (strcmp(runners[i]->lang, lang) == 0)
            return runners[i];
    return NULL;
}
