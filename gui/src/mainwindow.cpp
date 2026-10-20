// mainwindow.cpp — voir mainwindow.h.
#include "mainwindow.h"

#include <QActionGroup>
#include <QCloseEvent>
#include <QDir>
#include <QGuiApplication>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QProgressBar>
#include <QSettings>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyleHints>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#include "catalogview.h"
#include "engine.h"
#include "exerciseview.h"
#include "lessonview.h"

MainWindow::MainWindow(Engine &engine, QWidget *parent)
    : QMainWindow(parent), engine_(engine)
{
    setWindowTitle(tr("CL-Trainer"));

    // --- Colonne de gauche : recherche, catalogue, résumé de progression
    search_ = new QLineEdit;
    search_->setObjectName(QStringLiteral("search"));
    search_->setPlaceholderText(tr("Rechercher un exercice, un thème…  (Ctrl+F)"));
    search_->setClearButtonEnabled(true);
    catalog_ = new CatalogView;
    auto *progressTitle = new QLabel(tr("Progression"));
    progressTitle->setObjectName(QStringLiteral("sectionTitle"));
    progressLabel_ = new QLabel;
    progressLabel_->setObjectName(QStringLiteral("muted"));
    progressBar_ = new QProgressBar;
    progressBar_->setObjectName(QStringLiteral("progressBar"));
    progressBar_->setTextVisible(false); // le texte est dans progressLabel_
    weakLabel_ = new QLabel;
    weakLabel_->setObjectName(QStringLiteral("muted"));
    weakLabel_->setWordWrap(true);
    auto *left = new QWidget;
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(10, 4, 4, 10);
    leftLayout->setSpacing(8);
    leftLayout->addWidget(search_);
    leftLayout->addWidget(catalog_, 1);
    leftLayout->addSpacing(4);
    leftLayout->addWidget(progressTitle);
    leftLayout->addWidget(progressLabel_);
    leftLayout->addWidget(progressBar_);
    leftLayout->addWidget(weakLabel_);

    // --- Colonne de droite : l'exercice ou le cours sélectionné. Un
    // QStackedWidget empile des widgets et n'en montre qu'un à la fois.
    exercise_ = new ExerciseView(engine_);
    lesson_ = new LessonView(engine_);
    stack_ = new QStackedWidget;
    stack_->addWidget(exercise_);
    stack_->addWidget(lesson_);

    auto *splitter = new QSplitter;
    splitter->setObjectName(QStringLiteral("splitter"));
    splitter->addWidget(left);
    splitter->addWidget(stack_);
    splitter->setStretchFactor(1, 3);
    splitter->setSizes({ 360, 840 }); // largeurs initiales (remplacées par QSettings)
    splitter->setHandleWidth(8);
    setCentralWidget(splitter);

    buildToolbar();

    // Recherche : filtre à chaque frappe ; Ctrl+F y place le curseur ;
    // Échap l'efface.
    connect(search_, &QLineEdit::textChanged, catalog_, &CatalogView::setFilter);
    connect(catalog_, &CatalogView::filterCleared, search_, &QLineEdit::clear);
    auto *find = new QAction(this);
    find->setShortcut(QKeySequence::Find);
    connect(find, &QAction::triggered, search_, [this] {
        search_->setFocus();
        search_->selectAll();
    });
    addAction(find);
    auto *clearSearch = new QAction(search_);
    clearSearch->setShortcut(QKeySequence(Qt::Key_Escape));
    clearSearch->setShortcutContext(Qt::WidgetShortcut);
    connect(clearSearch, &QAction::triggered, search_, &QLineEdit::clear);
    search_->addAction(clearSearch);

    // --- Liaisons entre les vues
    connect(catalog_, &CatalogView::exerciseSelected, this, [this](const QString &id) {
        exercise_->showExercise(id);
        stack_->setCurrentWidget(exercise_);
    });
    connect(catalog_, &CatalogView::lessonSelected, this, [this](const QString &id) {
        lesson_->showLesson(id);
        stack_->setCurrentWidget(lesson_);
    });
    auto showMessage = [this](const QString &text) { statusBar()->showMessage(text, 10000); };
    connect(exercise_, &ExerciseView::progressChanged, this, &MainWindow::refreshProgress);
    connect(exercise_, &ExerciseView::message, this, showMessage);
    connect(exercise_, &ExerciseView::openLesson, catalog_, &CatalogView::selectId);
    connect(lesson_, &LessonView::progressChanged, this, &MainWindow::refreshProgress);
    connect(lesson_, &LessonView::message, this, showMessage);
    connect(lesson_, &LessonView::openExercise, catalog_, &CatalogView::selectId);

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
    if (!catalog_->selectId(settings.value(QStringLiteral("exercise/last")).toString()))
        goToNext();
    if (!settings.contains(QStringLiteral("window/geometry")))
        resize(1150, 720);
}

void MainWindow::buildToolbar()
{
    QToolBar *tools = addToolBar(tr("Actions"));
    tools->setObjectName(QStringLiteral("toolbar"));
    tools->setMovable(false);
    tools->setToolButtonStyle(Qt::ToolButtonTextOnly);
    QAction *next = tools->addAction(tr("▶  Suivant"), this, &MainWindow::goToNext);
    next->setObjectName(QStringLiteral("nextAction"));
    next->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_N));
    next->setToolTip(tr("Exercice ou cours recommandé (Ctrl+N)"));
    QAction *reload = tools->addAction(tr("⟳  Actualiser"), this, &MainWindow::reloadAll);
    reload->setShortcut(QKeySequence::Refresh);
    reload->setToolTip(tr("Relire exercices et progression (F5)"));

    auto *spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    tools->addWidget(spacer);

    // Menu du thème : trois choix exclusifs (QActionGroup).
    auto *menu = new QMenu(this);
    auto *group = new QActionGroup(menu);
    const std::pair<QString, theme::Mode> modes[] = {
        { tr("Automatique (comme le système)"), theme::Mode::Auto },
        { tr("Clair"), theme::Mode::Light },
        { tr("Sombre"), theme::Mode::Dark },
    };
    for (const auto &[label, mode] : modes) {
        QAction *a = menu->addAction(label);
        a->setCheckable(true);
        a->setChecked(theme::mode() == mode);
        group->addAction(a);
        connect(a, &QAction::triggered, this, [this, m = mode] { setThemeMode(m); });
    }
    auto *themeButton = new QToolButton;
    themeButton->setObjectName(QStringLiteral("themeButton"));
    themeButton->setText(tr("◐  Thème"));
    themeButton->setMenu(menu);
    themeButton->setPopupMode(QToolButton::InstantPopup);
    tools->addWidget(themeButton);

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    // Le bureau passe en sombre (ou en clair) : on suit, en mode automatique.
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, [this] {
        if (theme::mode() == theme::Mode::Auto)
            setThemeMode(theme::Mode::Auto);
    });
#endif
}

void MainWindow::setThemeMode(theme::Mode mode)
{
    theme::apply(mode);
    theme::saveMode(mode);
    restyle();
}

void MainWindow::restyle()
{
    // Les couleurs des statuts et le fond des blocs de code sont inscrits
    // dans les textes affichés : il faut les reconstruire.
    catalog_->refreshStatuses(engine_);
    exercise_->restyle();
    lesson_->refresh();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings;
    settings.setValue(QStringLiteral("window/geometry"), saveGeometry());
    if (auto *splitter = findChild<QSplitter *>(QStringLiteral("splitter")))
        settings.setValue(QStringLiteral("window/splitter"), splitter->saveState());
    settings.setValue(QStringLiteral("exercise/last"), catalog_->currentId());
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

bool MainWindow::showingLesson() const
{
    return stack_->currentWidget() == lesson_;
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
    case RECO_LESSON:
        statusBar()->showMessage(tr("Avant « %1 », lis le cours du chapitre.")
                                     .arg(qs(r.ex->title)), 10000);
        catalog_->selectId(qs(r.lesson_to_read->id));
        return;
    }
    if (r.ex)
        catalog_->selectId(qs(r.ex->id));
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
    lesson_->refresh();
    updateSummary();
    statusBar()->showMessage(tr("Exercices et progression rechargés."), 5000);
}

void MainWindow::refreshProgress()
{
    engine_.reloadProgress();
    catalog_->refreshStatuses(engine_);
    exercise_->refresh();
    lesson_->refresh();
    updateSummary();
    watchProgress(); // le dossier a pu être créé entre-temps
}

void MainWindow::updateSummary()
{
    const Engine::Summary s = engine_.summary();
    progressBar_->setRange(0, static_cast<int>(s.total));
    progressBar_->setValue(static_cast<int>(s.passed));
    QString text = tr("%1/%2 exercices réussis").arg(s.passed).arg(s.total);
    if (s.lessons > 0)
        text += tr(" · %1/%2 cours lus").arg(s.lessonsRead).arg(s.lessons);
    progressLabel_->setText(text);
    weakLabel_->setText(s.weakTopics.isEmpty()
                            ? tr("Points faibles : aucun pour l'instant")
                            : tr("Points faibles : %1").arg(s.weakTopics.join(QStringLiteral(", "))));
}
