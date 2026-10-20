// lessonview.cpp — voir lessonview.h.
#include "lessonview.h"

#include <cstring>

#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollBar>
#include <QTextBrowser>
#include <QVBoxLayout>

#include "engine.h"
#include "format.h"
#include "theme.h"

LessonView::LessonView(Engine &engine, QWidget *parent) : QWidget(parent), engine_(engine)
{
    title_ = new QLabel(this);
    QFont big = title_->font();
    big.setPointSizeF(big.pointSizeF() * 1.6);
    big.setBold(true);
    title_->setFont(big);
    title_->setObjectName(QStringLiteral("lessonTitle"));
    title_->setWordWrap(true);
    langChip_ = new QLabel(this);
    timeChip_ = new QLabel(this);
    readChip_ = new QLabel(this);
    for (QLabel *chip : { langChip_, timeChip_, readChip_ })
        chip->setObjectName(QStringLiteral("chip"));

    readButton_ = new QPushButton(tr("Marquer comme lu"), this);
    readButton_->setObjectName(QStringLiteral("markReadButton"));
    practiceButton_ = new QPushButton(tr("Passer aux exercices"), this);
    practiceButton_->setObjectName(QStringLiteral("practiceButton"));
    practiceButton_->setDefault(true);
    practiceButton_->setProperty("primary", true);

    // Ligne sous le titre : badges à gauche, actions à droite.
    auto *row = new QHBoxLayout;
    row->setSpacing(6);
    for (QLabel *chip : { langChip_, timeChip_, readChip_ })
        row->addWidget(chip);
    row->addStretch();
    row->addWidget(readButton_);
    row->addWidget(practiceButton_);

    body_ = new QTextBrowser(this);
    body_->setObjectName(QStringLiteral("lessonBody"));
    body_->setOpenExternalLinks(true);
    // Texte de cours : un peu plus grand que l'interface, pour la lecture.
    QFont reading = body_->font();
    reading.setPointSizeF(reading.pointSizeF() * 1.08);
    body_->setFont(reading);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 10, 14, 10);
    layout->setSpacing(10);
    layout->addWidget(title_);
    layout->addLayout(row);
    layout->addWidget(body_, 1);

    connect(readButton_, &QPushButton::clicked, this, &LessonView::onMarkRead);
    connect(practiceButton_, &QPushButton::clicked, this, &LessonView::onPractice);
}

void LessonView::showLesson(const QString &id)
{
    const bool changed = id != id_;
    id_ = id;
    refresh();
    if (changed)
        body_->verticalScrollBar()->setValue(0); // nouveau cours : en haut
}

QString LessonView::firstExerciseToDo() const
{
    const catalog &cat = engine_.exercises();
    QString first;
    for (size_t i = 0; i < cat.len; i++) {
        const exercise &ex = cat.items[i];
        if (!ex.lesson || qs(ex.lesson) != id_)
            continue;
        if (first.isEmpty())
            first = qs(ex.id);              // à défaut : le premier du chapitre
        if (engine_.status(ex) != STATUS_PASSED)
            return qs(ex.id);
    }
    return first;
}

void LessonView::refresh()
{
    const lesson *l = engine_.findLesson(id_);
    readButton_->setEnabled(l != nullptr);
    practiceButton_->setEnabled(l != nullptr);
    for (QLabel *chip : { langChip_, timeChip_, readChip_ })
        chip->setVisible(l != nullptr);
    if (!l) {
        title_->clear();
        body_->clear();
        return;
    }
    const bool read = engine_.lessonRead(*l);
    title_->setText(QStringLiteral("📖 ") + qs(l->title));
    langChip_->setText(tr("Cours %1").arg(format::langName(l->lang)));
    timeChip_->setText(tr("~%1 min de lecture").arg(l->estimated_min));
    timeChip_->setVisible(l->estimated_min > 0);
    readChip_->setText(read ? tr("✓ lu") : tr("pas encore lu"));
    readChip_->setProperty("kind", read ? QStringLiteral("success") : QString());
    theme::repolish(readChip_);
    readButton_->setText(read ? tr("Lu ✓") : tr("Marquer comme lu"));
    readButton_->setEnabled(!read);
    practiceButton_->setEnabled(!firstExerciseToDo().isEmpty());

    // Le cours, suivi de la liste des exercices du chapitre.
    QString md = format::lessonMarkdown(*l);
    const catalog &cat = engine_.exercises();
    QStringList exercises;
    for (size_t i = 0; i < cat.len; i++) {
        const exercise &ex = cat.items[i];
        if (ex.lesson && qs(ex.lesson) == id_)
            exercises << QStringLiteral("- %1 %2")
                             .arg(format::statusSymbol(engine_.status(ex), engine_.unlocked(ex)),
                                  qs(ex.title));
    }
    if (!exercises.isEmpty())
        md += tr("\n\n---\n\n## Exercices de ce chapitre\n\n") + exercises.join(QLatin1Char('\n'));
    const int scroll = body_->verticalScrollBar()->value();
    format::setMarkdown(body_, md);
    body_->verticalScrollBar()->setValue(scroll);
}

void LessonView::onMarkRead()
{
    const lesson *l = engine_.findLesson(id_);
    if (!l)
        return;
    try {
        engine_.markLessonRead(*l);
    } catch (const std::exception &err) {
        QMessageBox::warning(this, tr("CL-Trainer"), QString::fromUtf8(err.what()));
        return;
    }
    emit message(tr("Cours lu. Place aux exercices !"));
    emit progressChanged();
    refresh();
}

void LessonView::onPractice()
{
    const QString id = firstExerciseToDo();
    if (!id.isEmpty())
        emit openExercise(id);
}
