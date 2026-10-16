/* commands.c — sous-commandes list, show et start. */
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "app.h"
#include "error.h"
#include "fs.h"
#include "process.h"
#include "runner.h"
#include "term.h"
#include "text.h"

static void print_level(int level)
{
    for (int i = 1; i <= 5; i++)
        fputs(i <= level ? "★" : "☆", stdout);
}

/* ---------------------------------------------------------------- list */

int cmd_list(const app_ctx *app, int argc, char **argv)
{
    const char *lang = NULL, *topic = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--lang") == 0 && i + 1 < argc)
            lang = argv[++i];
        else if (strcmp(argv[i], "--topic") == 0 && i + 1 < argc)
            topic = argv[++i];
        else {
            fprintf(stderr, "usage : trainer list [--lang LANG] [--topic THÈME]\n");
            return TR_EXIT_ERROR;
        }
    }

    catalog cat;
    int rc = app_load_catalog(app, &cat);
    if (rc != TR_EXIT_OK)
        return rc;

    const char *current_lang = "";
    size_t shown = 0;
    for (size_t i = 0; i < cat.len; i++) {
        const exercise *ex = &cat.items[i];
        if ((lang && strcmp(ex->lang, lang) != 0) || (topic && strcmp(ex->topic, topic) != 0))
            continue;
        if (strcmp(ex->lang, current_lang) != 0) { /* nouveau groupe */
            const runner *r = runner_for(ex->lang);
            printf("%s%s%s%s\n", shown ? "\n" : "", term(T_BOLD),
                   r ? r->display_name : ex->lang, term(T_RESET));
            current_lang = ex->lang;
        }
        printf("  %-28s ", ex->id);
        print_level(ex->level);
        printf("  %s %s(%s)%s\n", ex->title, term(T_DIM), ex->topic, term(T_RESET));
        shown++;
    }
    if (shown == 0)
        printf("Aucun exercice trouvé dans %s.\n", app->exercises_dir);
    else
        printf("\n%zu exercice(s). Détails : trainer show <id>\n", shown);

    catalog_free(&cat);
    return TR_EXIT_OK;
}

/* ---------------------------------------------------------------- show */

int cmd_show(const app_ctx *app, int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage : trainer show <id>\n");
        return TR_EXIT_ERROR;
    }
    catalog cat;
    int rc = app_load_catalog(app, &cat);
    if (rc != TR_EXIT_OK)
        return rc;
    const exercise *ex = app_find(&cat, argv[1]);
    if (!ex) {
        catalog_free(&cat);
        return TR_EXIT_ERROR;
    }

    const runner *r = runner_for(ex->lang);
    printf("%s%s%s\n", term(T_BOLD), ex->title, term(T_RESET));
    printf("%s%s · %s · niveau %d/5 · thème %s", term(T_DIM), ex->id,
           r ? r->display_name : ex->lang, ex->level, ex->topic);
    if (ex->estimated_min > 0)
        printf(" · ~%d min", ex->estimated_min);
    printf("%s\n", term(T_RESET));
    if (ex->requires.len > 0) {
        printf("%sPrérequis :", term(T_DIM));
        for (size_t i = 0; i < ex->requires.len; i++)
            printf("%s %s", i ? "," : "", ex->requires.items[i]);
        printf("%s\n", term(T_RESET));
    }
    printf("\n%s\n\n", ex->statement);
    if (ex->hints.len > 0)
        printf("%s%zu indice(s) disponible(s) (commande hint : étape 3).%s\n",
               term(T_DIM), ex->hints.len, term(T_RESET));
    printf("Pour commencer : %strainer start %s%s\n", term(T_CYAN), ex->id, term(T_RESET));

    catalog_free(&cat);
    return TR_EXIT_OK;
}

/* ---------------------------------------------------------------- start */

/* .vscode/tasks.json : Ctrl+Maj+B lance « trainer check <id> » et les
 * erreurs de g++ deviennent cliquables dans l'onglet « Problèmes ».
 * Le problem matcher est défini ici plutôt que d'utiliser « $gcc », qui
 * n'existe que si l'extension Microsoft C/C++ est installée. */
static int write_vscode_tasks(const app_ctx *app, const exercise *ex, const char *workdir)
{
    strbuf dir = STRBUF_INIT, path = STRBUF_INIT, json = STRBUF_INIT;
    path_join(&dir, workdir, ".vscode");
    int rc = fs_mkdir_p(dir.data);
    if (rc != 0)
        goto out;

    sb_append(&json, "{\n  \"version\": \"2.0.0\",\n  \"tasks\": [\n    {\n"
                     "      \"label\": \"trainer check\",\n"
                     "      \"type\": \"process\",\n      \"command\": \"");
    json_escape(&json, app->self_path);
    sb_append(&json, "\",\n      \"args\": [\"--exercises\", \"");
    json_escape(&json, app->exercises_dir);
    sb_append(&json, "\", \"--workspace\", \"");
    json_escape(&json, app->workspace_dir);
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

static void open_in_vscode(const char *workdir)
{
    const char *argv[] = { "code", workdir, NULL };
    proc_opts opts = { 0 };
    opts.timeout_ms = 30000;
    proc_result res;
    int exec_errno;
    if (proc_run(argv, &opts, &res, &exec_errno) != 0) {
        if (exec_errno == ENOENT)
            fprintf(stderr, "trainer: commande « code » introuvable : VS Code est-il "
                            "installé et dans le PATH ?\n");
        else
            fprintf(stderr, "trainer: %s\n", err_msg());
    }
    proc_result_free(&res);
}

int cmd_start(const app_ctx *app, int argc, char **argv)
{
    const char *id = NULL;
    int force = 0, open_editor = 0, bad = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--force") == 0)
            force = 1;
        else if (strcmp(argv[i], "--open") == 0)
            open_editor = 1;
        else if (!id && argv[i][0] != '-')
            id = argv[i];
        else
            bad = 1;
    }
    if (!id || bad) {
        fprintf(stderr, "usage : trainer start <id> [--force] [--open]\n"
                        "  --force  recopie le code de départ (écrase vos fichiers)\n"
                        "  --open   ouvre le dossier dans VS Code\n");
        return TR_EXIT_ERROR;
    }

    catalog cat;
    int rc = app_load_catalog(app, &cat);
    if (rc != TR_EXIT_OK)
        return rc;
    strbuf workdir = STRBUF_INIT, starter = STRBUF_INIT, path = STRBUF_INIT;
    const exercise *ex = app_find(&cat, id);
    const runner *r = ex ? runner_for(ex->lang) : NULL;
    rc = TR_EXIT_ERROR;
    if (!ex)
        goto out;
    if (!r) {
        fprintf(stderr, "trainer: le langage « %s » n'est pas encore pris en charge.\n",
                ex->lang);
        goto out;
    }

    app_workdir(app, ex, &workdir);
    path_join(&starter, ex->dir, "starter");
    int existed = fs_is_dir(workdir.data);
    if (existed && !force) {
        printf("Exercice déjà commencé : vos fichiers sont conservés.\n");
    } else {
        if (fs_copy_tree(starter.data, workdir.data, force) != 0)
            goto fail;
        path_join(&path, workdir.data, ".gitignore");
        if (!fs_exists(path.data) && fs_write_file(path.data, ".build/\n", 8) != 0)
            goto fail;
        printf("%s %s\n", existed ? "Code de départ recopié dans" : "Exercice créé dans",
               workdir.data);
    }
    /* Les fichiers d'éditeur sont régénérés à chaque fois : ils dépendent de
     * l'emplacement de trainer, qui peut changer (installation...). */
    if ((r->prepare_workspace && r->prepare_workspace(ex, workdir.data) != 0) ||
        write_vscode_tasks(app, ex, workdir.data) != 0)
        goto fail;

    printf("\n  Dossier  : %s%s%s\n", term(T_CYAN), workdir.data, term(T_RESET));
    printf("  Énoncé   : trainer show %s\n", ex->id);
    printf("  Vérifier : trainer check %s   (ou Ctrl+Maj+B dans VS Code)\n", ex->id);
    if (open_editor)
        open_in_vscode(workdir.data);
    rc = TR_EXIT_OK;
    goto out;

fail:
    fprintf(stderr, "trainer: %s\n", err_msg());
out:
    sb_free(&workdir);
    sb_free(&starter);
    sb_free(&path);
    catalog_free(&cat);
    return rc;
}
