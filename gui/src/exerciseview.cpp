// exerciseview.cpp — voir exerciseview.h.
#include "exerciseview.h"

#include <algorithm>

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QScrollBar>
#include <QSplitter>
#include <QTabWidget>
#include <QTextBrowser>
#include <QUrl>
#include <QVBoxLayout>
#include <QtConcurrent>

#include "engine.h"
#include "format.h"
#include "theme.h"

namespace {

// Lignes spéciales de la liste des résultats (les autres = numéro du test).
constexpr int kBuildRow = -2;   // journal de compilation (échec)
constexpr int kWarningRow = -1; // avertissements du compilateur
constexpr int kRowRole = Qt::UserRole;

QPushButton *makeButton(const QString &text, const QString &name, QWidget *parent)
{
    auto *b = new QPushButton(text, parent);
    b->setObjectName(name); // permet aux tests de retrouver le bouton
    return b;
}

// Badge : petite étiquette arrondie, mise en forme par la feuille de style
// du thème (objectName « chip », propriété « kind » pour la couleur).
QLabel *makeChip(QWidget *parent)
{
    auto *chip = new QLabel(parent);
    chip->setObjectName(QStringLiteral("chip"));
    return chip;
}

void setChipKind(QLabel *chip, const char *kind)
{
    if (chip->property("kind").toString() == QLatin1String(kind))
        return;
    chip->setProperty("kind", QString::fromLatin1(kind));
    theme::repolish(chip);
}

} // namespace

ExerciseView::ExerciseView(Engine &engine, QWidget *parent)
    : QWidget(parent), engine_(engine)
{
    confirm = [this](const QString &question) {
        return QMessageBox::question(this, tr("CL-Trainer"), question) == QMessageBox::Yes;
    };

    // Chaque widget créé avec un parent est détruit automatiquement avec
    // lui : c'est l'arbre de propriété de Qt (pas de delete à écrire).
    title_ = new QLabel(this);
    QFont big = title_->font();
    big.setPointSizeF(big.pointSizeF() * 1.6);
    big.setBold(true);
    title_->setFont(big);
    title_->setObjectName(QStringLiteral("title"));
    langChip_ = makeChip(this);
    levelChip_ = makeChip(this);
    topicChip_ = makeChip(this);
    timeChip_ = makeChip(this);
    statusChip_ = makeChip(this);
    attemptsChip_ = makeChip(this);
    idLabel_ = new QLabel(this);
    idLabel_->setObjectName(QStringLiteral("muted"));
    idLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse); // id copiable
    banner_ = new QLabel(this);
    banner_->setObjectName(QStringLiteral("banner"));
    banner_->setWordWrap(true);
    lessonButton_ = makeButton(tr("📖 Lire le cours"), QStringLiteral("lessonButton"), this);

    startButton_ = makeButton(tr("Commencer"), QStringLiteral("startButton"), this);
    editorButton_ = makeButton(tr("Ouvrir dans VS Code"), QStringLiteral("editorButton"), this);
    folderButton_ = makeButton(tr("Dossier"), QStringLiteral("folderButton"), this);
    hintButton_ = makeButton(tr("Indice"), QStringLiteral("hintButton"), this);
    solutionButton_ = makeButton(tr("Solution"), QStringLiteral("solutionButton"), this);
    checkButton_ = makeButton(tr("Vérifier"), QStringLiteral("checkButton"), this);
    checkButton_->setDefault(true);
    checkButton_->setProperty("primary", true); // bouton principal : mis en avant
    checkButton_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return));
    checkButton_->setToolTip(tr("Compiler et lancer les tests (Ctrl+Entrée)"));

    auto *buttons = new QHBoxLayout;
    buttons->addWidget(startButton_);
    buttons->addWidget(editorButton_);
    buttons->addWidget(folderButton_);
    buttons->addStretch();
    buttons->addWidget(hintButton_);
    buttons->addWidget(solutionButton_);
    buttons->addWidget(checkButton_);

    statement_ = new QTextBrowser(this);
    statement_->setObjectName(QStringLiteral("statement"));
    statement_->setOpenExternalLinks(true);

    tests_ = new QListWidget(this);
    tests_->setObjectName(QStringLiteral("tests"));
    tests_->setMinimumWidth(220);
    tests_->setWordWrap(true); // noms longs : retour à la ligne plutôt que défilement
    tests_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    details_ = new QTextBrowser(this);
    details_->setObjectName(QStringLiteral("details"));
    auto *results = new QSplitter(Qt::Horizontal, this);
    results->addWidget(tests_);
    results->addWidget(details_);
    results->setStretchFactor(1, 2);
    results->setHandleWidth(8);

    correction_ = new QTextBrowser(this);
    correction_->setObjectName(QStringLiteral("correction"));

    tabs_ = new QTabWidget(this);
    tabs_->setObjectName(QStringLiteral("tabs"));
    tabs_->addTab(statement_, tr("Énoncé"));
    tabs_->addTab(results, tr("Résultats"));
    tabs_->addTab(correction_, tr("Correction"));

    auto *chips = new QHBoxLayout;
    chips->setSpacing(6);
    for (QLabel *chip : { langChip_, levelChip_, topicChip_, timeChip_, statusChip_, attemptsChip_ })
        chips->addWidget(chip);
    chips->addStretch();
    chips->addWidget(idLabel_);

    auto *bannerRow = new QHBoxLayout;
    bannerRow->addWidget(banner_, 1);
    bannerRow->addWidget(lessonButton_);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 10, 14, 10);
    layout->setSpacing(10);
    layout->addWidget(title_);
    layout->addLayout(chips);
    layout->addLayout(bannerRow);
    layout->addLayout(buttons);
    layout->addWidget(tabs_, 1);

    // Signaux -> slots. Syntaxe moderne (pointeurs de fonctions membres) :
    // une faute de frappe est détectée à la compilation.
    connect(startButton_, &QPushButton::clicked, this, &ExerciseView::onStart);
    connect(editorButton_, &QPushButton::clicked, this, &ExerciseView::onOpenEditor);
    connect(folderButton_, &QPushButton::clicked, this, &ExerciseView::onOpenFolder);
    connect(checkButton_, &QPushButton::clicked, this, &ExerciseView::onCheck);
    connect(hintButton_, &QPushButton::clicked, this, &ExerciseView::onHint);
    connect(solutionButton_, &QPushButton::clicked, this, &ExerciseView::onSolution);
    connect(tests_, &QListWidget::currentRowChanged, this, &ExerciseView::onTestSelected);
    connect(lessonButton_, &QPushButton::clicked, this, [this] {
        if (const exercise *ex = engine_.find(id_); ex && ex->lesson)
            emit openLesson(qs(ex->lesson));
    });
    connect(&watcher_, &QFutureWatcher<std::shared_ptr<CheckResult>>::finished, this,
            &ExerciseView::onCheckFinished);

    refresh();
}

ExerciseView::~ExerciseView()
{
    // Ne pas laisser une vérification tourner après la destruction de la vue.
    watcher_.waitForFinished();
}

bool ExerciseView::isChecking() const
{
    return watcher_.isRunning();
}

void ExerciseView::showExercise(const QString &id)
{
    if (id == id_)
        return;
    id_ = id;
    solutionShown_ = false;
    last_.reset();
    clearResults();
    tabs_->setCurrentIndex(0);
    refresh();
}

void ExerciseView::clearResults()
{
    tests_->clear();
    details_->clear();
}

void ExerciseView::refresh()
{
    const exercise *ex = engine_.find(id_);
    const bool checking = isChecking();
    for (QPushButton *b : { startButton_, editorButton_, folderButton_, checkButton_,
                            hintButton_, solutionButton_ })
        b->setEnabled(ex != nullptr);
    if (!ex) {
        title_->setText(tr("Choisis un exercice"));
        for (QLabel *chip : { langChip_, levelChip_, topicChip_, timeChip_, statusChip_, attemptsChip_ })
            chip->hide();
        idLabel_->setText(tr("Dans la liste de gauche, ou avec « Suivant » dans la barre d'outils."));
        banner_->hide();
        lessonButton_->hide();
        statement_->clear();
        correction_->clear();
        return;
    }

    const exercise_status st = engine_.status(*ex);
    const progress_entry *e = engine_.entry(*ex);
    const bool started = engine_.started(*ex);
    static const char *const statusText[] = { "nouveau", "commencé", "à reprendre", "réussi" };

    title_->setText(qs(ex->title));
    langChip_->setText(format::langName(ex->lang));
    levelChip_->setText(format::levelStars(ex->level));
    levelChip_->setToolTip(tr("Niveau %1 sur 5").arg(ex->level));
    topicChip_->setText(tr("thème : %1").arg(qs(ex->topic)));
    timeChip_->setText(tr("~%1 min").arg(ex->estimated_min));
    timeChip_->setVisible(ex->estimated_min > 0);
    statusChip_->setText(format::statusSymbol(st, true) + QLatin1Char(' ') + tr(statusText[st]));
    static const char *const statusKind[] = { "", "warning", "failure", "success" };
    setChipKind(statusChip_, statusKind[st]);
    attemptsChip_->setText(tr("%n vérification(s)", nullptr, e ? e->attempts : 0));
    attemptsChip_->setVisible(e && e->attempts > 0);
    for (QLabel *chip : { langChip_, levelChip_, topicChip_, statusChip_ })
        chip->show();
    idLabel_->setText(qs(ex->id));

    // Bandeau : cours pas encore lu, prérequis pas encore réussis.
    QStringList notes;
    const lesson *l = ex->lesson ? engine_.findLesson(qs(ex->lesson)) : nullptr;
    const bool lessonUnread = l && !engine_.lessonRead(*l);
    if (lessonUnread)
        notes << tr("Cours du chapitre pas encore lu : « %1 ».").arg(qs(l->title));
    if (!engine_.unlocked(*ex)) {
        QStringList missing;
        for (size_t i = 0; i < ex->prereqs.len; i++)
            missing << qs(ex->prereqs.items[i]);
        notes << tr("Prérequis conseillés, pas encore réussis : %1")
                     .arg(missing.join(QStringLiteral(", ")));
    }
    banner_->setText(notes.join(QLatin1Char('\n')));
    banner_->setVisible(!notes.isEmpty());
    lessonButton_->setVisible(l != nullptr);
    lessonButton_->setText(lessonUnread ? tr("📖 Lire le cours") : tr("📖 Revoir le cours"));

    // Énoncé + indices déjà révélés
    const int hintsTotal = static_cast<int>(ex->hints.len);
    const int hintsShown = e ? std::min(e->hints, hintsTotal) : 0;
    const int scroll = statement_->verticalScrollBar()->value();
    format::setMarkdown(statement_, format::statementMarkdown(*ex, hintsShown));
    statement_->verticalScrollBar()->setValue(scroll);

    startButton_->setText(started ? tr("Recommencer") : tr("Commencer"));
    editorButton_->setEnabled(started);
    folderButton_->setEnabled(started);
    checkButton_->setEnabled(started && !checking);
    checkButton_->setText(checking ? tr("Vérification…") : tr("Vérifier"));
    hintButton_->setEnabled(hintsShown < hintsTotal);
    hintButton_->setText(hintsTotal ? tr("Indice (%1/%2)").arg(hintsShown).arg(hintsTotal)
                                    : tr("Pas d'indice"));

    // La correction est visible après réussite ou si la solution a été vue.
    const bool passed = st == STATUS_PASSED;
    const bool showSolution = solutionShown_ || (e && e->solution_seen);
    tabs_->setTabEnabled(2, passed || showSolution);
    if (showSolution)
        format::setMarkdown(correction_, format::solutionMarkdown(*ex));
    else if (passed)
        format::setMarkdown(correction_, format::correctionMarkdown(*ex));
    else
        correction_->clear();
}

void ExerciseView::showError(const QString &text)
{
    QMessageBox::warning(this, tr("CL-Trainer"), text);
}

void ExerciseView::onStart()
{
    const exercise *ex = engine_.find(id_);
    if (!ex)
        return;
    const bool force = engine_.started(*ex);
    if (force && !confirm(tr("Recopier le code de départ ?\nTes modifications seront perdues.")))
        return;
    try {
        engine_.start(*ex, force);
        emit message(tr("Code prêt dans %1").arg(engine_.workdir(*ex)));
        emit progressChanged();
    } catch (const std::exception &err) {
        showError(QString::fromUtf8(err.what()));
    }
    refresh();
}

void ExerciseView::onOpenEditor()
{
    const exercise *ex = engine_.find(id_);
    if (!ex)
        return;
    // QProcess::startDetached : lance VS Code sans attendre qu'il se ferme.
    if (!QProcess::startDetached(QStringLiteral("code"), { engine_.workdir(*ex) }))
        showError(tr("Impossible de lancer VS Code (commande « code » introuvable).\n"
                     "Ouvre le dossier à la main : %1").arg(engine_.workdir(*ex)));
}

void ExerciseView::onOpenFolder()
{
    const exercise *ex = engine_.find(id_);
    if (ex)
        QDesktopServices::openUrl(QUrl::fromLocalFile(engine_.workdir(*ex)));
}

void ExerciseView::onCheck()
{
    const exercise *ex = engine_.find(id_);
    if (!ex || isChecking() || !engine_.started(*ex))
        return;
    clearResults();
    last_.reset();
    tests_->addItem(tr("Compilation et tests en cours…"));
    tabs_->setCurrentIndex(1);
    emit message(tr("Vérification de %1…").arg(qs(ex->id)));

    // On copie dans la lambda tout ce dont le thread a besoin (QString :
    // copies sûres entre threads). Surtout pas le pointeur ex : le catalogue
    // pourrait être rechargé pendant la vérification.
    const QString dir = qs(ex->dir);
    const QString workdir = engine_.workdir(*ex);
    watcher_.setFuture(QtConcurrent::run([dir, workdir] { return Engine::runCheck(dir, workdir); }));
    refresh();
}

void ExerciseView::onCheckFinished()
{
    std::shared_ptr<CheckResult> r = watcher_.result();
    clearResults();
    if (!r->ok) {
        refresh();
        showError(tr("La vérification n'a pas pu avoir lieu :\n%1").arg(r->error));
        emit checkFinished(false);
        return;
    }

    const check_report &rep = r->report;
    const bool passed = rep.status == CHECK_PASSED;
    // On enregistre pour l'exercice VÉRIFIÉ, même si l'élève a changé
    // d'exercice entre-temps.
    if (const exercise *ex = engine_.find(qs(r->ex.id))) {
        const progress_event ev = passed ? EVENT_CHECK_PASSED
                                  : rep.status == CHECK_BUILD_FAILED ? EVENT_CHECK_BUILD_FAILED
                                                                     : EVENT_CHECK_FAILED;
        try {
            engine_.record(*ex, ev);
        } catch (const std::exception &err) {
            emit message(tr("Progression non enregistrée : %1").arg(QString::fromUtf8(err.what())));
        }
    }
    emit progressChanged();

    if (qs(r->ex.id) == id_) {
        last_ = r;
        showResults();
        if (passed)
            emit message(tr("Bravo, tous les tests passent (%1/%2) ! La correction est "
                            "disponible.").arg(rep.passed).arg(rep.tests.len));
        else if (rep.status == CHECK_BUILD_FAILED)
            emit message(tr("La compilation a échoué."));
        else
            emit message(tr("%1/%2 tests réussis.").arg(rep.passed).arg(rep.tests.len));
    }
    refresh();
    emit checkFinished(passed);
}

void ExerciseView::showResults()
{
    clearResults();
    if (!last_ || !last_->ok)
        return;
    const check_report &rep = last_->report;
    auto addRow = [this](const QString &text, int row, theme::Role role) {
        auto *item = new QListWidgetItem(text, tests_);
        item->setData(kRowRole, row);
        item->setForeground(theme::color(role));
    };
    int firstFailure = -1;
    if (rep.status == CHECK_BUILD_FAILED) {
        addRow(tr("✗ Compilation"), kBuildRow, theme::Role::Failure);
        firstFailure = 0;
    } else {
        if (rep.build_log[0])
            addRow(tr("⚠ Avertissements"), kWarningRow, theme::Role::Warning);
        for (size_t i = 0; i < rep.tests.len; i++) {
            const test_outcome &o = rep.outcomes[i];
            QString name = qs(rep.tests.items[i].name);
            if (rep.tests.items[i].hidden)
                name += tr(" (caché)");
            if (o.result == VERDICT_PASS) {
                addRow(QStringLiteral("✓ ") + name, static_cast<int>(i), theme::Role::Success);
            } else {
                addRow(QStringLiteral("✗ %1 — %2").arg(name, qs(verdict_label(o.result))),
                       static_cast<int>(i), theme::Role::Failure);
                if (firstFailure < 0)
                    firstFailure = tests_->count() - 1;
            }
        }
    }
    tests_->setCurrentRow(firstFailure >= 0 ? firstFailure : 0);
}

void ExerciseView::restyle()
{
    const int row = tests_->currentRow();
    refresh();
    if (last_) {
        showResults();
        tests_->setCurrentRow(row);
    }
}

void ExerciseView::onTestSelected(int row)
{
    QListWidgetItem *item = row >= 0 ? tests_->item(row) : nullptr;
    if (!item || !last_ || !last_->ok) {
        details_->clear();
        return;
    }
    const check_report &rep = last_->report;
    const int index = item->data(kRowRole).toInt();
    if (index == kBuildRow || index == kWarningRow)
        details_->setHtml(format::buildLogHtml(rep.build_log, index == kBuildRow));
    else
        details_->setHtml(format::testDetailsHtml(last_->ex, rep.tests.items[index],
                                                  rep.outcomes[index]));
}

void ExerciseView::onHint()
{
    const exercise *ex = engine_.find(id_);
    if (!ex)
        return;
    const progress_entry *e = engine_.entry(*ex);
    const int next = (e ? e->hints : 0) + 1;
    if (next > static_cast<int>(ex->hints.len))
        return;
    try {
        engine_.record(*ex, EVENT_HINT, next);
    } catch (const std::exception &err) {
        showError(QString::fromUtf8(err.what()));
        return;
    }
    tabs_->setCurrentIndex(0);
    refresh();
    statement_->verticalScrollBar()->setValue(statement_->verticalScrollBar()->maximum());
    emit progressChanged();
}

void ExerciseView::onSolution()
{
    const exercise *ex = engine_.find(id_);
    if (!ex)
        return;
    const bool passed = engine_.status(*ex) == STATUS_PASSED;
    if (!passed) {
        if (!confirm(tr("Tu n'as pas encore réussi cet exercice. Voir la solution quand même ?\n"
                        "Ce sera noté dans ta progression.")))
            return;
        try {
            engine_.record(*ex, EVENT_SOLUTION);
        } catch (const std::exception &err) {
            showError(QString::fromUtf8(err.what()));
            return;
        }
        emit message(tr("Conseil : ferme la solution et réécris le code de mémoire."));
        emit progressChanged();
    }
    solutionShown_ = true;
    refresh();
    tabs_->setCurrentIndex(2);
}
