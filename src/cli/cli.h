/*
 * cli.h — interface en ligne de commande : sous-commandes et fonctions
 * d'affichage partagées. Toute la logique est dans libtrainer (src/core) ;
 * ce dossier ne fait que lire les arguments et présenter les résultats.
 */
#ifndef TRAINER_CLI_H
#define TRAINER_CLI_H

#include "trainer.h"

/* Codes de sortie du programme (utilisables dans des scripts / la CI). */
enum {
    TR_EXIT_OK = 0,    /* succès */
    TR_EXIT_FAIL = 1,  /* échec de l'élève (tests...) ou contenu invalide */
    TR_EXIT_ERROR = 2, /* erreur d'utilisation ou d'environnement */
};

/* ---- Aides (cli.c) ---- */

/* Charge le catalogue ; affiche les avertissements. TR_EXIT_OK / TR_EXIT_ERROR. */
int cli_load_catalog(const trainer_config *cfg, catalog *cat);
/* Cherche un exercice ; affiche un message s'il n'existe pas. */
const exercise *cli_find(const catalog *cat, const char *id);
/* Charge la progression. En cas d'erreur, affiche un avertissement et
 * renvoie une progression vide : la consultation reste possible. */
void cli_load_progress(const trainer_config *cfg, progress *p);
/* Enregistre un événement ; un échec n'est qu'un avertissement. */
void cli_record(const trainer_config *cfg, const char *id, progress_event ev, int arg);

/* Affiche un texte en retrait, préfixé par « │ », tronqué à max_lines. */
void print_block(const char *title, const char *text, int max_lines);
/* ★★☆☆☆ */
void print_level(int level);
/* Symbole coloré du statut : ✓ ✗ ● (commencé), ○ (nouveau), – (verrouillé). */
void print_status_symbol(exercise_status st, int unlocked);
/* Barre de progression ████░░░░ de largeur width. */
void print_bar(size_t done, size_t total, int width);
/* Nom lisible d'un langage : "C++" pour "cpp". */
const char *lang_name(const char *lang);

/* ---- Sous-commandes : argv[0] est le nom de la commande ---- */
int cmd_list(const trainer_config *cfg, int argc, char **argv);
int cmd_show(const trainer_config *cfg, int argc, char **argv);
int cmd_start(const trainer_config *cfg, int argc, char **argv);
int cmd_check(const trainer_config *cfg, int argc, char **argv);
int cmd_hint(const trainer_config *cfg, int argc, char **argv);
int cmd_solution(const trainer_config *cfg, int argc, char **argv);
int cmd_next(const trainer_config *cfg, int argc, char **argv);
int cmd_stats(const trainer_config *cfg, int argc, char **argv);
int cmd_validate(const trainer_config *cfg, int argc, char **argv);
int cmd_doctor(const trainer_config *cfg, int argc, char **argv);

#endif
