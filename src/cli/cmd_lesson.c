/* cmd_lesson.c — sous-commande learn : affiche un cours dans le terminal. */
#include <stdio.h>
#include <string.h>

#include "cli.h"
#include "term.h"
#include "text.h"

/* Rendu simple du Markdown pour le terminal : titres en gras, blocs de code
 * en retrait et encadrés. Le reste est déjà lisible tel quel. */
static void print_markdown(const char *md)
{
    const char *cursor = md, *line;
    size_t len;
    int in_code = 0;
    while (text_next_line(&cursor, &line, &len)) {
        if (span_starts_with(line, len, "```")) {
            if (!in_code && span_eq(line, len, "```output"))
                printf("    %sSortie du programme :%s\n", term(T_DIM), term(T_RESET));
            in_code = !in_code;
            printf("    %s%s%s\n", term(T_DIM), in_code ? "┌────" : "└────", term(T_RESET));
        } else if (in_code) {
            printf("    %s│%s %.*s\n", term(T_DIM), term(T_RESET), (int)len, line);
        } else if (len > 0 && line[0] == '#') {
            size_t level = 0;
            while (level < len && line[level] == '#')
                level++;
            const char *title = line + level;
            size_t tn = len - level;
            span_trim(&title, &tn);
            printf("%s%s%.*s%s\n", term(T_BOLD), level <= 2 ? term(T_CYAN) : "", (int)tn,
                   title, term(T_RESET));
        } else {
            printf("%.*s\n", (int)len, line);
        }
    }
}

void print_lesson(const lesson *l, const catalog *cat, const progress *p)
{
    printf("%s📖 %s%s\n", term(T_BOLD), l->title, term(T_RESET));
    printf("%s%s · cours · %s", term(T_DIM), l->id, lang_name(l->lang));
    if (l->estimated_min > 0)
        printf(" · ~%d min de lecture", l->estimated_min);
    printf("%s\n\n", term(T_RESET));
    print_markdown(l->body);

    /* Les exercices du chapitre, pour enchaîner. */
    const exercise *next = NULL;
    printf("\n%sExercices de ce chapitre%s\n", term(T_BOLD), term(T_RESET));
    for (size_t i = 0; i < cat->len; i++) {
        const exercise *ex = &cat->items[i];
        if (!ex->lesson || strcmp(ex->lesson, l->id) != 0)
            continue;
        exercise_status st = progress_status(p, ex->id);
        printf("  ");
        print_status_symbol(st, exercise_unlocked(cat, p, ex));
        printf(" %-28s %s\n", ex->id, ex->title);
        if (!next && st != STATUS_PASSED)
            next = ex;
    }
    if (next)
        printf("\nPour pratiquer : %strainer start %s%s\n", term(T_CYAN), next->id,
               term(T_RESET));
}

int cmd_learn(const trainer_config *cfg, int argc, char **argv)
{
    if (argc > 2) {
        fprintf(stderr, "usage : trainer learn [<id du cours>]\n");
        return TR_EXIT_ERROR;
    }
    catalog cat;
    int rc = cli_load_catalog(cfg, &cat);
    if (rc != TR_EXIT_OK)
        return rc;
    progress p;
    cli_load_progress(cfg, &p);

    if (argc == 1) { /* sans argument : la liste des cours */
        printf("%sCours%s\n", term(T_BOLD), term(T_RESET));
        for (size_t i = 0; i < cat.nlessons; i++) {
            const lesson *l = &cat.lessons[i];
            printf("  %s %-28s %s\n",
                   progress_status(&p, l->id) == STATUS_PASSED ? "✓" : "📖", l->id, l->title);
        }
        if (cat.nlessons == 0)
            printf("  (aucun cours)\n");
        printf("\nLire un cours : trainer learn <id>\n");
    } else {
        const lesson *l = catalog_find_lesson(&cat, argv[1]);
        if (!l) {
            fprintf(stderr, "trainer: cours inconnu « %s ». La commande « trainer learn » "
                            "liste les cours.\n", argv[1]);
            rc = TR_EXIT_ERROR;
        } else {
            print_lesson(l, &cat, &p);
            /* Afficher le cours en entier vaut lecture. */
            cli_record(cfg, l->id, EVENT_LESSON_READ, 0);
        }
    }
    progress_free(&p);
    catalog_free(&cat);
    return rc;
}
