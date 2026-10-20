// test_gui.cpp — test de l'application graphique avec QTest : on simule le
// parcours d'un élève (clics sur les boutons) sans écran, avec la plateforme
// « offscreen ». Les dossiers sont temporaires : la vraie progression n'est
// jamais touchée.
#include <QFile>
#include <QListWidget>
#include <QProgressBar>
#include <QPushButton>
#include <QSignalSpy>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextBrowser>
#include <QtTest>

#include "catalogview.h"
#include "engine.h"
#include "exerciseview.h"
#include "mainwindow.h"

class GuiTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void studentJourney();

private:
    QTemporaryDir tmp_;  // supprimé automatiquement à la fin (RAII)
    QString root_;       // racine du dépôt (TRAINER_ROOT, fourni par CMake)
};

void GuiTest::initTestCase()
{
    QVERIFY(tmp_.isValid());
    root_ = qEnvironmentVariable("TRAINER_ROOT");
    QVERIFY2(!root_.isEmpty(), "TRAINER_ROOT non défini (lancer via ctest)");
    qputenv("TRAINER_EXERCISES", (root_ + "/exercises").toUtf8());
    qputenv("TRAINER_WORKSPACE", tmp_.filePath("ws").toUtf8());
    qputenv("XDG_DATA_HOME", tmp_.filePath("data").toUtf8());
    qputenv("XDG_CONFIG_HOME", tmp_.filePath("config").toUtf8()); // QSettings
}

// Capture d'écran si TRAINER_SCREENSHOTS désigne un dossier (documentation).
static void screenshot(QWidget *w, const char *name)
{
    const QString dir = qEnvironmentVariable("TRAINER_SCREENSHOTS");
    if (!dir.isEmpty())
        w->grab().save(dir + QLatin1Char('/') + QLatin1String(name) + QStringLiteral(".png"));
}

// Clique sur « Vérifier » et attend la fin ; renvoie « tous les tests passent ».
static bool runCheck(ExerciseView *view)
{
    QSignalSpy spy(view, &ExerciseView::checkFinished);
    view->findChild<QPushButton *>("checkButton")->click();
    if (!spy.wait(120000)) // la compilation peut être lente en CI
        return false;
    return spy.takeFirst().at(0).toBool();
}

void GuiTest::studentJourney()
{
    Engine engine(QStringLiteral("test-gui"));
    MainWindow window(engine);
    window.show();
    ExerciseView *view = window.exerciseView();
    view->confirm = [](const QString &) { return true; }; // pas de boîte de dialogue

    // Au premier lancement, l'exercice recommandé est sélectionné.
    QCOMPARE(view->exerciseId(), QStringLiteral("cpp.basics.hello"));
    auto *checkButton = view->findChild<QPushButton *>("checkButton");
    QVERIFY(!checkButton->isEnabled()); // pas encore commencé

    // Commencer : le code de départ est copié.
    view->findChild<QPushButton *>("startButton")->click();
    const exercise *hello = engine.find(QStringLiteral("cpp.basics.hello"));
    QVERIFY(hello);
    const QString workdir = engine.workdir(*hello);
    QVERIFY(QFile::exists(workdir + "/main.cpp"));
    QVERIFY(checkButton->isEnabled());

    // Le code de départ échoue aux tests...
    QVERIFY(!runCheck(view));
    auto *tests = view->findChild<QListWidget *>("tests");
    QCOMPARE(tests->count(), 4);
    QVERIFY(tests->item(0)->text().startsWith(QStringLiteral("✗")));
    QVERIFY(view->findChild<QTextBrowser *>("details")->toPlainText().contains("Entrée"));
    screenshot(&window, "echec");

    // ... la solution de référence réussit.
    QVERIFY(QFile::remove(workdir + "/main.cpp"));
    QVERIFY(QFile::copy(root_ + "/exercises/cpp/01-basics/hello/solution/main.cpp",
                        workdir + "/main.cpp"));
    QVERIFY(runCheck(view));
    for (int i = 0; i < tests->count(); i++)
        QVERIFY(tests->item(i)->text().startsWith(QStringLiteral("✓")));
    QVERIFY(window.catalogView()->currentItem()->text(0).startsWith(QStringLiteral("✓")));
    QCOMPARE(engine.status(*hello), STATUS_PASSED);
    auto *progressBar = window.findChild<QProgressBar *>("progressBar");
    QCOMPARE(progressBar->value(), 1);
    QCOMPARE(progressBar->maximum(), 2);
    screenshot(&window, "reussite");

    // « Suivant » propose l'exercice débloqué.
    window.goToNext();
    QCOMPARE(view->exerciseId(), QStringLiteral("cpp.basics.sum"));

    // Indice : révélé dans l'énoncé et enregistré.
    view->findChild<QPushButton *>("hintButton")->click();
    QVERIFY(view->findChild<QTextBrowser *>("statement")->toPlainText().contains("Indice 1/2"));
    QCOMPARE(view->findChild<QPushButton *>("hintButton")->text(), QStringLiteral("Indice (1/2)"));
    screenshot(&window, "indice");

    // Solution : onglet Correction avec le code de référence.
    view->findChild<QPushButton *>("solutionButton")->click();
    QCOMPARE(view->findChild<QTabWidget *>("tabs")->currentIndex(), 2);
    QVERIFY(view->findChild<QTextBrowser *>("correction")->toPlainText().contains("std::int64_t"));
    const exercise *sum = engine.find(QStringLiteral("cpp.basics.sum"));
    QVERIFY(sum && engine.entry(*sum) && engine.entry(*sum)->solution_seen == 1);

    // Une vérification faite depuis le TERMINAL (ici simulée par l'API C)
    // est détectée par la surveillance du dossier : la fenêtre se met à jour.
    const QByteArray path = QByteArray(engine.config().progress_path);
    QCOMPARE(progress_record(path.constData(), "cpp.basics.sum", EVENT_CHECK_PASSED, 0), 0);
    QTRY_COMPARE_WITH_TIMEOUT(progressBar->value(), 2, 5000);
}

QTEST_MAIN(GuiTest)
#include "test_gui.moc" // code généré par moc pour la classe définie dans ce .cpp
