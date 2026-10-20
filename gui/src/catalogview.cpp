// catalogview.cpp — voir catalogview.h.
#include "catalogview.h"

#include <QHeaderView>
#include <QHash>
#include <QTreeWidgetItemIterator>

#include "engine.h"
#include "format.h"

namespace {
// Rôle Qt où l'on range l'id de l'exercice dans chaque ligne de l'arbre.
constexpr int kIdRole = Qt::UserRole;
}

CatalogView::CatalogView(QWidget *parent) : QTreeWidget(parent)
{
    setObjectName(QStringLiteral("catalog"));
    setColumnCount(2);
    setHeaderLabels({ tr("Exercice"), tr("Niveau") });
    header()->setSectionResizeMode(0, QHeaderView::Stretch);
    header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header()->setStretchLastSection(false);
    setUniformRowHeights(true);

    // Connexion signal -> slot : quand la sélection change, on émet notre
    // propre signal avec l'id. La lambda capture « this » pour appeler nos
    // méthodes.
    connect(this, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem *current, QTreeWidgetItem *) {
                emit exerciseSelected(current ? current->data(0, kIdRole).toString() : QString());
            });
}

void CatalogView::populate(const Engine &engine)
{
    const QString previous = currentExerciseId();
    // Pendant la reconstruction, on bloque les signaux : sinon chaque
    // suppression de ligne émettrait « sélection changée ».
    blockSignals(true);
    clear();

    QHash<QString, QTreeWidgetItem *> groups; // "cpp" ou "cpp/basics" -> ligne
    const catalog &cat = engine.exercises();
    for (size_t i = 0; i < cat.len; i++) {
        const exercise &ex = cat.items[i];
        const QString lang = qs(ex.lang);
        const QString topicKey = lang + QLatin1Char('/') + qs(ex.topic);

        QTreeWidgetItem *langItem = groups.value(lang);
        if (!langItem) {
            // L'arbre devient PROPRIÉTAIRE de la ligne : pas de delete à faire.
            langItem = new QTreeWidgetItem(this, { format::langName(ex.lang) });
            QFont bold = langItem->font(0);
            bold.setBold(true);
            langItem->setFont(0, bold);
            langItem->setExpanded(true);
            groups.insert(lang, langItem);
        }
        QTreeWidgetItem *topicItem = groups.value(topicKey);
        if (!topicItem) {
            topicItem = new QTreeWidgetItem(langItem, { qs(ex.topic) });
            topicItem->setExpanded(true);
            groups.insert(topicKey, topicItem);
        }
        auto *item = new QTreeWidgetItem(topicItem);
        item->setData(0, kIdRole, qs(ex.id));
        decorate(item, engine);
    }
    blockSignals(false);
    if (!previous.isEmpty())
        selectExercise(previous);
}

void CatalogView::refreshStatuses(const Engine &engine)
{
    for (QTreeWidgetItemIterator it(this); *it; ++it)
        if (!(*it)->data(0, kIdRole).toString().isEmpty())
            decorate(*it, engine);
}

void CatalogView::decorate(QTreeWidgetItem *item, const Engine &engine)
{
    const exercise *ex = engine.find(item->data(0, kIdRole).toString());
    if (!ex)
        return;
    const exercise_status st = engine.status(*ex);
    const bool unlocked = engine.unlocked(*ex);
    item->setText(0, format::statusSymbol(st, unlocked) + QStringLiteral("  ") + qs(ex->title));
    item->setText(1, format::levelStars(ex->level));
    const QColor color = format::statusColor(st, unlocked);
    item->setForeground(0, color.isValid() ? QBrush(color) : QBrush());

    QString tip = qs(ex->id);
    if (!unlocked) {
        QStringList missing;
        for (size_t i = 0; i < ex->prereqs.len; i++)
            missing << qs(ex->prereqs.items[i]);
        tip += tr("\nPrérequis à réussir d'abord : %1").arg(missing.join(QStringLiteral(", ")));
    }
    item->setToolTip(0, tip);
}

QTreeWidgetItem *CatalogView::findItem(const QString &id) const
{
    for (QTreeWidgetItemIterator it(const_cast<CatalogView *>(this)); *it; ++it)
        if ((*it)->data(0, kIdRole).toString() == id)
            return *it;
    return nullptr;
}

bool CatalogView::selectExercise(const QString &id)
{
    // Les lignes « langage » et « thème » ont un id vide : ne pas les confondre
    // avec un exercice (cas du premier lancement, sans dernier exercice connu).
    if (id.isEmpty())
        return false;
    QTreeWidgetItem *item = findItem(id);
    if (!item)
        return false;
    setCurrentItem(item);
    scrollToItem(item);
    return true;
}

QString CatalogView::currentExerciseId() const
{
    return currentItem() ? currentItem()->data(0, kIdRole).toString() : QString();
}
