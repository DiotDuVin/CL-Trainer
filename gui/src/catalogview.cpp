// catalogview.cpp — voir catalogview.h.
#include "catalogview.h"

#include <cstring>

#include <QHeaderView>
#include <QHash>
#include <QTreeWidgetItemIterator>

#include "engine.h"
#include "format.h"

namespace {
// Rôles Qt où l'on range, dans chaque ligne de l'arbre, l'id de l'élément et
// s'il s'agit d'un cours.
constexpr int kIdRole = Qt::UserRole;
constexpr int kLessonRole = Qt::UserRole + 1;
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
                const QString id = current ? current->data(0, kIdRole).toString() : QString();
                if (current && current->data(0, kLessonRole).toBool())
                    emit lessonSelected(id);
                else
                    emit exerciseSelected(id);
            });
}

void CatalogView::populate(const Engine &engine)
{
    const QString previous = currentId();
    // Pendant la reconstruction, on bloque les signaux : sinon chaque
    // suppression de ligne émettrait « sélection changée ».
    blockSignals(true);
    clear();

    // Clés : "cpp" (langage), "lesson:<id>" (chapitre), "cpp/basics" (thème).
    QHash<QString, QTreeWidgetItem *> groups;
    auto langNode = [&](const char *lang) {
        const QString key = qs(lang);
        QTreeWidgetItem *item = groups.value(key);
        if (!item) {
            // L'arbre devient PROPRIÉTAIRE de la ligne : pas de delete à faire.
            item = new QTreeWidgetItem(this, { format::langName(lang) });
            QFont bold = item->font(0);
            bold.setBold(true);
            item->setFont(0, bold);
            item->setExpanded(true);
            groups.insert(key, item);
        }
        return item;
    };

    // Cours et exercices sont triés par dossier : on les fusionne dans cet
    // ordre. Le dossier d'un chapitre précède celui de ses exercices.
    const catalog &cat = engine.exercises();
    size_t j = 0;
    for (size_t i = 0; i <= cat.len; i++) {
        const exercise *ex = i < cat.len ? &cat.items[i] : nullptr;
        for (; j < cat.nlessons && (!ex || std::strcmp(cat.lessons[j].dir, ex->dir) < 0); j++) {
            const lesson &l = cat.lessons[j];
            auto *chapter = new QTreeWidgetItem(langNode(l.lang), { qs(l.title) });
            chapter->setExpanded(true);
            groups.insert(QStringLiteral("lesson:") + qs(l.id), chapter);
            auto *item = new QTreeWidgetItem(chapter);
            item->setData(0, kIdRole, qs(l.id));
            item->setData(0, kLessonRole, true);
            decorate(item, engine);
        }
        if (!ex)
            break;
        QTreeWidgetItem *parent = ex->lesson
            ? groups.value(QStringLiteral("lesson:") + qs(ex->lesson)) : nullptr;
        if (!parent) { // pas de cours : regroupement par thème
            const QString key = qs(ex->lang) + QLatin1Char('/') + qs(ex->topic);
            parent = groups.value(key);
            if (!parent) {
                parent = new QTreeWidgetItem(langNode(ex->lang), { qs(ex->topic) });
                parent->setExpanded(true);
                groups.insert(key, parent);
            }
        }
        auto *item = new QTreeWidgetItem(parent);
        item->setData(0, kIdRole, qs(ex->id));
        decorate(item, engine);
    }
    blockSignals(false);
    if (!previous.isEmpty())
        selectId(previous);
}

void CatalogView::refreshStatuses(const Engine &engine)
{
    for (QTreeWidgetItemIterator it(this); *it; ++it)
        if (!(*it)->data(0, kIdRole).toString().isEmpty())
            decorate(*it, engine);
}

void CatalogView::decorate(QTreeWidgetItem *item, const Engine &engine)
{
    if (item->data(0, kLessonRole).toBool()) {
        const lesson *l = engine.findLesson(item->data(0, kIdRole).toString());
        if (!l)
            return;
        const bool read = engine.lessonRead(*l);
        item->setText(0, read ? tr("✓  📖 Cours (lu)") : tr("📖  Cours"));
        item->setForeground(0, read ? QBrush(format::statusColor(STATUS_PASSED, true)) : QBrush());
        item->setToolTip(0, qs(l->id));
        return;
    }
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

bool CatalogView::selectId(const QString &id)
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

QString CatalogView::currentId() const
{
    return currentItem() ? currentItem()->data(0, kIdRole).toString() : QString();
}
