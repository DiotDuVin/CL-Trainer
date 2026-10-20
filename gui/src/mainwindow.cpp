// mainwindow.cpp — voir mainwindow.h.
#include "mainwindow.h"

#include <QCloseEvent>
#include <QDir>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>

#include "catalogview.h"
#include "engine.h"
#include "exerciseview.h"

MainWindow::MainWindow(Engine &engine, QWidget *parent)
    : QMainWindow(parent), engine_(engine)
{
    setWindowTitle(tr("CL-Trainer"));

    // --- Colonne de gauche : catalogue + résumé de progression
    catalog_ = new CatalogView;
    progressBar_ = new QProgressBar;
    progressBar_->setObjectName(QStringLiteral("progressBar"));
    progressBar_->setFormat(tr("%v/%m réussis"));
    weakLabel_ = new QLabel;
    weakLabel_->setWordWrap(true);
    auto *left = new QWidget;
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(6, 0, 0, 6);
    leftLayout->addWidget(catalog_, 1);
    leftLayout->addWidget(progressBar_);
    leftLayout->addWidget(weakLabel_);

    // --- Colonne de droite : l'exercice sélectionné
    exercise_ = new ExerciseView(engine_);

    auto *splitter = new QSplitter;
    splitter->setObjectName(QStringLiteral("splitter"));
    splitter->addWidget(left);
    splitter->addWidget(exercise_);
    splitter->setStretchFactor(1, 3);
    splitter->setSizes({ 340, 810 }); // largeurs initiales (remplacées par QSettings)
    setCentralWidget(splitter);

    // --- Barre d'outils
    QToolBar *tools = addToolBar(tr("Actions"));
    tools->setObjectName(QStringLiteral("toolbar"));
    tools->setMovable(false);
    QAction *next = tools->addAction(QIcon::fromTheme(QStringLiteral("go-next")), tr("Suivant"),
                                     this, &MainWindow::goToNext);
    next->setObjectName(QStringLiteral("nextAction"));
    next->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_N));
    next->setToolTip(tr("Exercice recommandé (Ctrl+N)"));
    QAction *reload = tools->addAction(QIcon::fromTheme(QStringLiteral("view-refresh")),
                                       tr("Actualiser"), this, &MainWindow::reloadAll);
    reload->setShortcut(QKeySequence::Refresh);
    tools->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    // --- Liaisons entre les vues
    connect(catalog_, &CatalogView::exerciseSelected, exercise_, &ExerciseView::showExercise);
    connect(exercise_, &ExerciseView::progressChanged, this, &MainWindow::refreshProgress);
    connect(exercise_, &ExerciseView::message, this,
            [this](const QString &text) { statusBar()->showMessage(text, 10000); });

    debounce_.setSingleShot(true);
    debounce_.setInterval(300);
    connect(&debounce_, &QTimer::timeout, this, &MainWindow::refreshProgress);
    connect(&watcher_, &QFileSystemWatcher::directoryChanged, &debounce_,
            qOverload<>(&QTimer::start));
    watchProgress();

    catalog_->populate(engine_);
    updateSummary();
    const QStringList warnings = engine_.warnings();
    if (!warnings.isEmpty())
        statusBar()->showMessage(tr("Attention : %1").arg(warnings.join(QStringLiteral(" ; "))));

    // QSettings : préférences enregistrées entre deux lancements
    // (~/.config/CL-Trainer/CL-Trainer.conf sous Linux).
    QSettings settings;
    restoreGeometry(settings.value(QStringLiteral("window/geometry")).toByteArray());
    splitter->restoreState(settings.value(QStringLiteral("window/splitter")).toByteArray());
    if (!catalog_->selectExercise(settings.value(QStringLiteral("exercise/last")).toString()))
        goToNext();
    if (!settings.contains(QStringLiteral("window/geometry")))
        resize(1150, 720);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings;
    settings.setValue(QStringLiteral("window/geometry"), saveGeometry());
    if (auto *splitter = findChild<QSplitter *>(QStringLiteral("splitter")))
        settings.setValue(QStringLiteral("window/splitter"), splitter->saveState());
    settings.setValue(QStringLiteral("exercise/last"), exercise_->exerciseId());
    QMainWindow::closeEvent(event);
}

void MainWindow::watchProgress()
{
    // On surveille le DOSSIER et pas le fichier : progress.txt est remplacé
    // par rename() à chaque écriture, et un fichier remplacé n'est plus suivi.
    const QString dir = qs(engine_.config().data_dir);
    QDir().mkpath(dir);
    if (!watcher_.directories().contains(dir))
        watcher_.addPath(dir);
}

void MainWindow::goToNext()
{
    const recommendation r = engine_.next();
    switch (r.reason) {
    case RECO_ALL_DONE:
        statusBar()->showMessage(tr("Tous les exercices sont réussis, bravo !"), 10000);
        return;
    case RECO_LOCKED:
        statusBar()->showMessage(tr("Les exercices restants ont des prérequis non réussis."), 10000);
        return;
    case RECO_RESUME:
        statusBar()->showMessage(tr("Exercice commencé : termine-le d'abord."), 10000);
        break;
    case RECO_WEAK_TOPIC:
        statusBar()->showMessage(tr("Le thème « %1 » te pose des difficultés : on le renforce.")
                                     .arg(qs(r.topic)), 10000);
        break;
    case RECO_NEXT:
        statusBar()->showMessage(tr("Prochain exercice de la progression."), 10000);
        break;
    }
    if (r.ex)
        catalog_->selectExercise(qs(r.ex->id));
}

void MainWindow::reloadAll()
{
    try {
        engine_.reload();
    } catch (const std::exception &err) {
        QMessageBox::warning(this, tr("CL-Trainer"), QString::fromUtf8(err.what()));
        return;
    }
    catalog_->populate(engine_);
    exercise_->refresh();
    updateSummary();
    statusBar()->showMessage(tr("Exercices et progression rechargés."), 5000);
}

void MainWindow::refreshProgress()
{
    engine_.reloadProgress();
    catalog_->refreshStatuses(engine_);
    exercise_->refresh();
    updateSummary();
    watchProgress(); // le dossier a pu être créé entre-temps
}

void MainWindow::updateSummary()
{
    const Engine::Summary s = engine_.summary();
    progressBar_->setRange(0, static_cast<int>(s.total));
    progressBar_->setValue(static_cast<int>(s.passed));
    weakLabel_->setText(s.weakTopics.isEmpty()
                            ? tr("Points faibles : aucun pour l'instant")
                            : tr("Points faibles : %1").arg(s.weakTopics.join(QStringLiteral(", "))));
}
