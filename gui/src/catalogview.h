// catalogview.h — arbre : langage > chapitre (cours puis exercices).
// Les exercices sans cours sont regroupés par thème.
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
    // Sélectionne un exercice ou un cours ; false s'il n'existe pas.
    bool selectId(const QString &id);
    QString currentId() const;

    // Filtre : ne montre que les éléments dont le titre, l'id, le thème ou
    // les tags contiennent le texte (sans tenir compte des majuscules ni des
    // accents). Texte vide : tout est affiché.
    void setFilter(const QString &text);
    // Nombre d'exercices et de cours visibles (pour les tests).
    int visibleCount() const;

signals:
    // Émis quand l'utilisateur choisit un exercice (id vide : un chapitre ou
    // un langage est sélectionné)...
    void exerciseSelected(const QString &id);
    // ... ou un cours.
    void lessonSelected(const QString &id);
    // Le filtre a été effacé pour montrer un élément (champ de recherche à vider).
    void filterCleared();

private:
    QTreeWidgetItem *findItem(const QString &id) const;
    static void decorate(QTreeWidgetItem *item, const Engine &engine);
    // Chapitre ou langage : compteur « réussis/total » dans la 2e colonne.
    static void decorateGroup(QTreeWidgetItem *item, const Engine &engine);
    bool applyFilter(QTreeWidgetItem *item, bool parentMatches);

    QString filter_;   // texte du filtre, déjà « replié » (voir fold())
};
