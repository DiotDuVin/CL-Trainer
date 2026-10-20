/* cli.c — fonctions partagées par les sous-commandes (voir cli.h). */
#include "cli.h"

#include <stdio.h>
#include <string.h>

#include "term.h"
#include "text.h"

int cli_load_catalog(const trainer_config *cfg, catalog *cat)
{
    if (catalog_load(cfg->exercises_dir, cat) != 0) {
        fprintf(stderr, "trainer: %s\n", err_msg());
        return TR_EXIT_ERROR;
    }
    for (size_t i = 0; i < cat->errors.len; i++)
        fprintf(stderr, "%sattention%s : %s\n", term(T_YELLOW), term(T_RESET),
                cat->errors.items[i]);
    return TR_EXIT_OK;
}

const exercise *cli_find(const catalog *cat, const char *id)
{
    const exercise *ex = catalog_find(cat, id);
    if (!ex)
        fprintf(stderr, "trainer: exercice inconnu « %s ». "
                        "La commande « trainer list » affiche les exercices.\n", id);
    return ex;
}

void cli_load_progress(const trainer_config *cfg, progress *p)
{
    if (progress_load(cfg->progress_path, p) != 0)
        fprintf(stderr, "%sattention%s : progression illisible, ignorée : %s\n",
                term(T_YELLOW), term(T_RESET), err_msg());
}

void cli_record(const trainer_config *cfg, const char *id, progress_event ev, int arg)
{
    if (progress_record(cfg->progress_path, id, ev, arg) != 0)
        fprintf(stderr, "%sattention%s : progression non enregistrée : %s\n",
                term(T_YELLOW), term(T_RESET), err_msg());
}

void print_block(const char *title, const char *text, int max_lines)
{
    printf("    %s :\n", title);
    if (!text[0]) {
        printf("      %s(vide)%s\n", term(T_DIM), term(T_RESET));
        return;
    }
    const char *cursor = text, *line;
    size_t len;
    int n = 0;
    while (text_next_line(&cursor, &line, &len)) {
        if (n == max_lines) {
            int rest = 1;
            while (text_next_line(&cursor, &line, &len))
                rest++;
            printf("      %s… (%d ligne(s) de plus)%s\n", term(T_DIM), rest, term(T_RESET));
            break;
        }
        printf("      %s│%s %.*s\n", term(T_DIM), term(T_RESET), (int)len, line);
        n++;
    }
}

void print_level(int level)
{
    for (int i = 1; i <= 5; i++)
        fputs(i <= level ? "★" : "☆", stdout);
}

void print_status_symbol(exercise_status st, int unlocked)
{
    switch (st) {
    case STATUS_PASSED:  printf("%s✓%s", term(T_GREEN), term(T_RESET)); break;
    case STATUS_FAILED:  printf("%s✗%s", term(T_RED), term(T_RESET)); break;
    case STATUS_STARTED: printf("%s●%s", term(T_YELLOW), term(T_RESET)); break;
    case STATUS_NEW:
        if (unlocked)
            printf("○");
        else
            printf("%s–%s", term(T_DIM), term(T_RESET));
        break;
    }
}

void print_bar(size_t done, size_t total, int width)
{
    int filled = total ? (int)((done * (size_t)width + total / 2) / total) : 0;
    fputs(term(T_GREEN), stdout);
    for (int i = 0; i < width; i++) {
        if (i == filled)
            fputs(term(T_DIM), stdout);
        fputs(i < filled ? "█" : "░", stdout);
    }
    fputs(term(T_RESET), stdout);
}

const char *lang_name(const char *lang)
{
    const runner *r = runner_for(lang);
    if (r)
        return r->display_name;
    if (strcmp(lang, "csharp") == 0)
        return "C#";
    if (strcmp(lang, "rust") == 0)
        return "Rust";
    if (strcmp(lang, "git") == 0)
        return "Git";
    return lang;
}
