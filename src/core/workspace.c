/* workspace.c — voir workspace.h. */
#include "workspace.h"

#include <errno.h>

#include "error.h"
#include "fs.h"
#include "process.h"
#include "runner.h"
#include "text.h"

void workspace_dir(const trainer_config *cfg, const exercise *ex, strbuf *out)
{
    strbuf tmp = STRBUF_INIT;
    path_join(&tmp, cfg->workspace_dir, ex->lang);
    path_join(out, tmp.data, ex->id);
    sb_free(&tmp);
}

int workspace_exists(const trainer_config *cfg, const exercise *ex)
{
    strbuf dir = STRBUF_INIT;
    workspace_dir(cfg, ex, &dir);
    int exists = fs_is_dir(dir.data);
    sb_free(&dir);
    return exists;
}

/* .vscode/tasks.json : Ctrl+Maj+B lance « trainer check <id> » et les
 * erreurs du compilateur deviennent cliquables dans l'onglet « Problèmes ».
 * Le problem matcher est défini ici plutôt que d'utiliser « $gcc », qui
 * n'existe que si l'extension Microsoft C/C++ est installée. */
static int write_vscode_tasks(const trainer_config *cfg, const exercise *ex,
                              const char *workdir)
{
    strbuf dir = STRBUF_INIT, path = STRBUF_INIT, json = STRBUF_INIT;
    path_join(&dir, workdir, ".vscode");
    int rc = fs_mkdir_p(dir.data);
    if (rc != 0)
        goto out;

    sb_append(&json, "{\n  \"version\": \"2.0.0\",\n  \"tasks\": [\n    {\n"
                     "      \"label\": \"trainer check\",\n"
                     "      \"type\": \"process\",\n      \"command\": \"");
    json_escape(&json, cfg->cli_path);
    sb_append(&json, "\",\n      \"args\": [\"--exercises\", \"");
    json_escape(&json, cfg->exercises_dir);
    sb_append(&json, "\", \"--workspace\", \"");
    json_escape(&json, cfg->workspace_dir);
    sb_append(&json, "\", \"check\", \"");
    json_escape(&json, ex->id);
    sb_append(&json,
              "\"],\n"
              "      \"group\": { \"kind\": \"build\", \"isDefault\": true },\n"
              "      \"presentation\": { \"reveal\": \"always\", \"clear\": true },\n"
              "      \"problemMatcher\": {\n"
              "        \"owner\": \"trainer\",\n"
              "        \"fileLocation\": [\"relative\", \"${workspaceFolder}\"],\n"
              "        \"pattern\": {\n"
              "          \"regexp\": \"^(.*?):(\\\\d+):(\\\\d+):\\\\s+(?:fatal\\\\s+)?"
              "(error|warning):\\\\s+(.*)$\",\n"
              "          \"file\": 1, \"line\": 2, \"column\": 3, \"severity\": 4, \"message\": 5\n"
              "        }\n"
              "      }\n"
              "    }\n  ]\n}\n");
    path_join(&path, dir.data, "tasks.json");
    rc = fs_write_file(path.data, json.data, json.len);
out:
    sb_free(&dir);
    sb_free(&path);
    sb_free(&json);
    return rc;
}

int workspace_start(const trainer_config *cfg, const exercise *ex, int force,
                    start_result *result)
{
    const runner *r = runner_for(ex->lang);
    if (!r) {
        err_set("le langage « %s » n'est pas encore pris en charge", ex->lang);
        return -1;
    }
    strbuf workdir = STRBUF_INIT, starter = STRBUF_INIT, path = STRBUF_INIT;
    int rc = -1;

    workspace_dir(cfg, ex, &workdir);
    path_join(&starter, ex->dir, "starter");
    int existed = fs_is_dir(workdir.data);
    if (existed && !force) {
        *result = START_EXISTED;
    } else {
        if (fs_copy_tree(starter.data, workdir.data, force) != 0)
            goto out;
        *result = existed ? START_RESET : START_CREATED;
    }
    path_join(&path, workdir.data, ".gitignore");
    if (!fs_exists(path.data) && fs_write_file(path.data, ".build/\n", 8) != 0)
        goto out;
    /* Les fichiers d'éditeur sont régénérés à chaque fois : ils dépendent de
     * l'emplacement de trainer, qui peut changer (installation...). */
    if (r->prepare_workspace && r->prepare_workspace(ex, workdir.data) != 0)
        goto out;
    if (write_vscode_tasks(cfg, ex, workdir.data) != 0)
        goto out;
    rc = 0;
out:
    sb_free(&workdir);
    sb_free(&starter);
    sb_free(&path);
    return rc;
}

int workspace_open_editor(const char *dir)
{
    const char *argv[] = { "code", dir, NULL };
    proc_opts opts = { 0 };
    opts.timeout_ms = 30000;
    proc_result res;
    int exec_errno;
    int rc = proc_run(argv, &opts, &res, &exec_errno);
    proc_result_free(&res);
    if (rc != 0 && exec_errno == ENOENT)
        err_set("commande « code » introuvable : VS Code est-il installé et dans le PATH ?");
    return rc;
}
