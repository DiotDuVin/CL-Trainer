// theme.h — apparence de l'application : thème clair ou sombre.
//
// Toutes les couleurs de l'interface viennent d'ici : une palette Qt
// (couleurs des widgets standard), des couleurs « de sens » (réussi, échoué,
// fond du code...) et une feuille de style (QSS, une sorte de CSS pour Qt).
// Changer de thème = réappliquer les trois, puis demander aux vues de se
// redessiner (le Markdown et le HTML affichés contiennent des couleurs).
#pragma once

#include <QColor>
#include <QString>

class QWidget;

namespace theme {

enum class Mode { Auto, Light, Dark };   // Auto : suit le réglage du système

// Couleurs « de sens », dans une variante adaptée au thème courant.
enum class Role {
    Accent,         // sélection, bouton principal, liens
    Success,        // réussi
    Failure,        // échoué
    Warning,        // commencé, avertissements
    Muted,          // texte secondaire, exercice verrouillé
    Border,
    Surface,        // fond des panneaux (listes, textes)
    SoftBackground, // fond des badges
    CodeBackground, // fond des blocs de code
    OutputBackground, // fond des sorties de programme dans les cours
    NoticeBackground, // fond du bandeau d'information
};

// Applique un mode à toute l'application (style Fusion, palette, QSS).
void apply(Mode mode);
Mode mode();
bool isDark();                 // thème effectivement affiché
QColor color(Role role);

// Recalcule le style d'un widget après le changement d'une propriété
// utilisée par la feuille de style (ex. : setProperty("kind", "success")).
void repolish(QWidget *widget);

// Lecture / écriture du choix dans les préférences (QSettings).
Mode savedMode();
void saveMode(Mode mode);

} // namespace theme
