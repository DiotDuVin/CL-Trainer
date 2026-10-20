// catalogview.cpp — voir catalogview.h.
#include "catalogview.h"

#include <cstring>
#include <functional>

#include <QHeaderView>
#include <QHash>
#include <QTreeWidgetItemIterator>

#include "engine.h"
#include "format.h"
#include "theme.h"

namespace {
// Rôles Qt où l'on range, dans chaque ligne de l'arbre, l'id de l'élément,
// s'il s'agit d'un cours, et le texte utilisé par la recherche.
constexpr int kIdRole = Qt::UserRole;
constexpr int kLessonRole = Qt::UserRole + 1;
constexpr int kSearchRole = Qt::UserRole + 2;

// « Replie » un texte pour la recherche : minuscules et sans accents
// (« Références » et « references » deviennent identiques). La forme NFD
// sépare chaque lettre de son accent, qu'on supprime ensuite.
QString fold(const QString &text)
{
    QString out;
    for (const QChar c : text.normalized(QString::NormalizationForm_D))
        if (c.category() != QChar::Mark_NonSpacing)
            out += c.toLower();
    return out;
}
} // namespace

CatalogView::CatalogView(QWidget *parent) : QTreeWidget(parent)
{
    setObjectName(QStringLiteral("catalog"));
    setColumnCount(2);
    setHeaderHidden(true); // les colonnes parlent d'elles-mêmes (titre, niveau)
    header()->setSectionResizeMode(0, QHeaderView::Stretch);
    header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header()->setStretchLastSection(false);
    setUniformRowHeights(true);
    setIndentation(16);

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
    QHash<QString, int> chapterCount; // numéro du dernier chapitre, par langage
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
            const int number = ++chapterCount[qs(l.lang)];
            const QString title = QStringLiteral("%1 · %2").arg(number).arg(qs(l.title));
            auto *chapter = new QTreeWidgetItem(langNode(l.lang), { title });
            chapter->setData(0, kSearchRole, fold(title));
            groups.insert(QStringLiteral("lesson:") + qs(l.id), chapter);
            auto *item = new QTreeWidgetItem(chapter);
            item->setData(0, kIdRole, qs(l.id));
            item->setData(0, kLessonRole, true);
            item->setData(0, kSearchRole, fold(tr("cours ") + qs(l.id)));
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
                parent->setData(0, kSearchRole, fold(qs(ex->topic)));
                groups.insert(key, parent);
            }
        }
        auto *item = new QTreeWidgetItem(parent);
        item->setData(0, kIdRole, qs(ex->id));
        QStringList words = { qs(ex->title), qs(ex->id), qs(ex->topic) };
        for (size_t t = 0; t < ex->tags.len; t++)
            words << qs(ex->tags.items[t]);
        item->setData(0, kSearchRole, fold(words.join(QLatin1Char(' '))));
        decorate(item, engine);
    }
    refreshStatuses(engine);  // compteurs des chapitres
    applyFilter(invisibleRootItem(), false);
    blockSignals(false);
    if (!previous.isEmpty())
        selectId(previous);
}

void CatalogView::refreshStatuses(const Engine &engine)
{
    for (QTreeWidgetItemIterator it(this); *it; ++it) {
        if (!(*it)->data(0, kIdRole).toString().isEmpty())
            decorate(*it, engine);
        else
            decorateGroup(*it, engine);
    }
}

void CatalogView::decorate(QTreeWidgetItem *item, const Engine &engine)
{
    const QColor muted = theme::color(theme::Role::Muted);
    if (item->data(0, kLessonRole).toBool()) {
        const lesson *l = engine.findLesson(item->data(0, kIdRole).toString());
        if (!l)
            return;
        const bool read = engine.lessonRead(*l);
        item->setText(0, read ? tr("📖  Cours (lu)") : tr("📖  Cours"));
        item->setForeground(0, read ? QBrush(muted) : QBrush());
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
    item->setForeground(1, muted);
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

void CatalogView::decorateGroup(QTreeWidgetItem *item, const Engine &engine)
{
    // Compte les exercices de tout le sous-arbre (un langage contient des
    // chapitres, qui contiennent des exercices) : fonction récursive.
    int passed = 0, total = 0;
    std::function<void(const QTreeWidgetItem *)> count = [&](const QTreeWidgetItem *node) {
        for (int i = 0; i < node->childCount(); i++) {
            const QTreeWidgetItem *child = node->child(i);
            if (child->childCount() > 0)
                count(child);
            else if (!child->data(0, kLessonRole).toBool())
                if (const exercise *ex = engine.find(child->data(0, kIdRole).toString())) {
                    total++;
                    passed += engine.status(*ex) == STATUS_PASSED;
                }
        }
    };
    count(item);
    if (total == 0)
        return;
    item->setText(1, QStringLiteral("%1/%2").arg(passed).arg(total));
    item->setForeground(1, passed == total ? theme::color(theme::Role::Success)
                                           : theme::color(theme::Role::Muted));
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
    if (item->isHidden()) {     // masqué par le filtre : on l'efface
        setFilter(QString());
        emit filterCleared();
    }
    setCurrentItem(item);
    scrollToItem(item);
    return true;
}

QString CatalogView::currentId() const
{
    return currentItem() ? currentItem()->data(0, kIdRole).toString() : QString();
}

void CatalogView::setFilter(const QString &text)
{
    filter_ = fold(text.trimmed());
    applyFilter(invisibleRootItem(), false);
}

// Renvoie true si l'élément reste visible. Un groupe dont le titre
// correspond montre tout son contenu ; sinon il est visible si l'un de ses
// enfants l'est.
bool CatalogView::applyFilter(QTreeWidgetItem *item, bool parentMatches)
{
    const bool matches = parentMatches || filter_.isEmpty() ||
                         item->data(0, kSearchRole).toString().contains(filter_);
    if (item->childCount() == 0) {
        item->setHidden(!matches);
        return matches;
    }
    bool anyChild = false;
    for (int i = 0; i < item->childCount(); i++)
        anyChild |= applyFilter(item->child(i), matches);
    if (item != invisibleRootItem()) {
        item->setHidden(!anyChild);
        // Pendant une recherche, tout est déplié pour montrer les résultats.
        item->setExpanded(true);
    }
    return anyChild;
}

int CatalogView::visibleCount() const
{
    int n = 0;
    for (QTreeWidgetItemIterator it(const_cast<CatalogView *>(this)); *it; ++it)
        if (!(*it)->isHidden() && !(*it)->data(0, kIdRole).toString().isEmpty())
            n++;
    return n;
}
