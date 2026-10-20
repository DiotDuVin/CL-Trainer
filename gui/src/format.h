// format.h — mise en forme des textes affichés (Markdown / HTML).
// Fonctions libres, sans état : faciles à tester et à réutiliser.
#pragma once

#include <QColor>
#include <QString>

#include "trainer.h"

namespace format {

QString statusSymbol(exercise_status st, bool unlocked);   // ✓ ✗ ● ○ –
QColor statusColor(exercise_status st, bool unlocked);
QString levelStars(int level);                             // ★★☆☆☆
QString langName(const char *lang);                        // "C++"

// Énoncé + indices déjà révélés (Markdown).
QString statementMarkdown(const exercise &ex, int hintsShown);
// Correction affichée après réussite : explication + erreurs fréquentes.
QString correctionMarkdown(const exercise &ex);
// Solution complète : code des fichiers de solution/ + correction.
QString solutionMarkdown(const exercise &ex);

// Détail d'un test (HTML) : explication, entrée, différence, erreurs.
QString testDetailsHtml(const exercise &ex, const test_case &tc, const test_outcome &o);
// Journal de compilation (HTML).
QString buildLogHtml(const char *log, bool failed);

} // namespace format
