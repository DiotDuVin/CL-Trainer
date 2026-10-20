// catalogview.h — arbre des exercices : langage > thème > exercice.
#pragma once

#include <QTreeWidget>

class Engine;

// Héritage : CatalogView EST un QTreeWidget (il en a toutes les fonctions)
// auquel on ajoute ce qui est propre à notre application.
class CatalogView : public QTreeWidget {
    Q_OBJECT // nécessaire pour déclarer des signaux (traité par moc)

public:
    explicit CatalogView(QWidget *parent = nullptr);

    // Reconstruit l'arbre (après rechargement du catalogue).
    void populate(const Engine &engine);
    // Met à jour symboles et couleurs (après un changement de progression).
    void refreshStatuses(const Engine &engine);
    // Sélectionne un exercice ; false s'il n'existe pas.
    bool selectExercise(const QString &id);
    QString currentExerciseId() const;

signals:
    // Émis quand l'utilisateur choisit un exercice (id vide : un thème ou
    // un langage est sélectionné).
    void exerciseSelected(const QString &id);

private:
    QTreeWidgetItem *findItem(const QString &id) const;
    static void decorate(QTreeWidgetItem *item, const Engine &engine);
};
