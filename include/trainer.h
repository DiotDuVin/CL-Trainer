/*
 * trainer.h — en-tête unique de la bibliothèque libtrainer, pour les
 * programmes qui l'utilisent (terminal, application graphique Qt...).
 *
 * Toutes les fonctions publiques suivent la convention : 0 = succès,
 * -1 = erreur avec message dans err_msg(). Chaque structure *_INIT ou
 * remplie par une fonction *_load / *_run / *_compute se libère avec la
 * fonction *_free correspondante.
 *
 * Utilisable depuis le C++ : chaque en-tête déclare ses fonctions dans un
 * bloc extern "C" (pas de « name mangling »).
 */
#ifndef TRAINER_H
#define TRAINER_H

#include "catalog.h"
#include "check.h"
#include "compare.h"
#include "config.h"
#include "doctor.h"
#include "error.h"
#include "exercise.h"
#include "progress.h"
#include "runner.h"
#include "stats.h"
#include "strbuf.h"
#include "strvec.h"
#include "testspec.h"
#include "validate.h"
#include "workspace.h"

#endif
