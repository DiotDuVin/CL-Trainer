/* config.c — voir config.h. */
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "error.h"
#include "fs.h"
#include "strbuf.h"
#include "xalloc.h"

#ifndef TRAINER_DATADIR /* fourni par le Makefile : $(PREFIX)/share/trainer */
#define TRAINER_DATADIR "/usr/local/share/trainer"
#endif

static const char *env(const char *name)
{
    const char *v = getenv(name);
    return (v && *v) ? v : NULL;
}

/* Dossier contenant l'exécutable : /proc/self/exe sous Linux, sinon argv[0].
 * Renvoie NULL si on ne peut pas le savoir (programme trouvé via le PATH). */
static char *self_dir(const char *argv0)
{
    char buf[4096];
    strbuf sb = STRBUF_INIT;
    ssize_t n = readlink("/proc/self/exe", buf, sizeof buf - 1);
    if (n > 0) {
        buf[n] = '\0';
        sb_append(&sb, buf);
    } else if (argv0 && strchr(argv0, '/')) {
        if (fs_absolute(argv0, &sb) != 0) {
            sb_free(&sb);
            return NULL;
        }
    } else {
        return NULL;
    }
    char *slash = strrchr(sb.data, '/');
    sb_truncate(&sb, slash ? (size_t)(slash - sb.data) : 0);
    return sb_release(&sb);
}

static char *absolute(const char *path)
{
    strbuf sb = STRBUF_INIT;
    if (fs_absolute(path, &sb) != 0) {
        sb_free(&sb);
        return NULL;
    }
    return sb_release(&sb);
}

int config_resolve(const char *opt_exercises, const char *opt_workspace,
                   const char *argv0, trainer_config *cfg)
{
    memset(cfg, 0, sizeof *cfg);
    char *dir = self_dir(argv0);
    strbuf sb = STRBUF_INIT;
    int rc = -1;

    /* Exercices */
    const char *explicit_dir = opt_exercises ? opt_exercises : env("TRAINER_EXERCISES");
    if (explicit_dir) {
        cfg->exercises_dir = absolute(explicit_dir); /* inexistant : signalé au chargement */
    } else {
        if (dir) {
            path_join(&sb, dir, "exercises");
            if (fs_is_dir(sb.data))
                cfg->exercises_dir = xstrdup(sb.data);
        }
        if (!cfg->exercises_dir && fs_is_dir(TRAINER_DATADIR "/exercises"))
            cfg->exercises_dir = xstrdup(TRAINER_DATADIR "/exercises");
        if (!cfg->exercises_dir)
            cfg->exercises_dir = absolute("exercises");
    }

    /* Workspace et données */
    const char *home = env("HOME");
    const char *ws = opt_workspace ? opt_workspace : env("TRAINER_WORKSPACE");
    if (ws) {
        cfg->workspace_dir = absolute(ws);
    } else if (home) {
        path_join(&sb, home, "trainer-workspace");
        cfg->workspace_dir = xstrdup(sb.data);
    }
    const char *xdg = env("XDG_DATA_HOME");
    if (xdg) {
        path_join(&sb, xdg, "trainer");
        cfg->data_dir = xstrdup(sb.data);
    } else if (home) {
        path_join(&sb, home, ".local/share/trainer");
        cfg->data_dir = xstrdup(sb.data);
    }
    if (!cfg->exercises_dir || !cfg->workspace_dir || !cfg->data_dir) {
        err_set("impossible de déterminer les dossiers de travail "
                "(variable HOME non définie ?)");
        goto out;
    }
    path_join(&sb, cfg->data_dir, "progress.txt");
    cfg->progress_path = xstrdup(sb.data);

    /* Le terminal et l'application graphique sont installés côte à côte :
     * on cherche « trainer » dans notre propre dossier, sinon dans le PATH. */
    if (dir) {
        path_join(&sb, dir, "trainer");
        if (access(sb.data, X_OK) == 0)
            cfg->cli_path = xstrdup(sb.data);
    }
    if (!cfg->cli_path)
        cfg->cli_path = xstrdup("trainer");
    rc = 0;

out:
    free(dir);
    sb_free(&sb);
    return rc;
}

void config_free(trainer_config *cfg)
{
    free(cfg->exercises_dir);
    free(cfg->workspace_dir);
    free(cfg->data_dir);
    free(cfg->progress_path);
    free(cfg->cli_path);
    memset(cfg, 0, sizeof *cfg);
}
