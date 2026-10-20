/*
 * validate.h — contrôle qualité d'un exercice (commande validate, CI).
 *
 * Un exercice est accepté si :
 *   1. tests.txt est valide et contient au moins un test caché ;
 *   2. la solution de référence compile et réussit TOUS les tests ;
 *   3. le code de départ (starter) compile mais échoue à au moins un test
 *      (sinon l'exercice serait déjà résolu, ou les tests trop faibles).
 * Chaque vérification se fait dans un dossier temporaire, supprimé ensuite.
 */
#ifndef TRAINER_VALIDATE_H
#define TRAINER_VALIDATE_H

#include "exercise.h"
#include "strvec.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Ajoute à problems une description de chaque problème trouvé (aucun
 * ajout = exercice valide). -1 seulement si la validation n'a pas pu se
 * faire (compilateur absent...). */
int validate_exercise(const exercise *ex, strvec *problems);

#ifdef __cplusplus
}
#endif

#endif
