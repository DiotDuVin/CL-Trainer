// screenshots.cpp — captures d'écran de la documentation (docs/images), avec
// le VRAI catalogue et une progression fictive, en thème clair et sombre.
// Ce n'est pas un test : « make gui-screenshots » le lance à la demande.
// Comme le test, il tourne sans écran (QT_QPA_PLATFORM=offscreen) et dans
// des dossiers temporaires : la vraie progression n'est jamais touchée.
#include <cstdio>

#include <QApplication>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "catalogview.h"
#include "engine.h"
#include "exerciseview.h"
#include "mainwindow.h"
#include "theme.h"

namespace {

void save(QWidget *w, const QString &dir, const char *name)
{
    QApplication::processEvents();
    const QString path = dir + QStringLiteral("/gui-%1.png").arg(QLatin1String(name));
    if (!w->grab().save(path))
        std::fprintf(stderr, "impossible d'écrire %s\n", qPrintable(path));
    else
        std::printf("%s\n", qPrintable(path));
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    const QString out = qEnvironmentVariable("TRAINER_SCREENSHOTS");
    if (out.isEmpty()) {
        std::fprintf(stderr, "usage : TRAINER_SCREENSHOTS=<dossier> %s\n", argv[0]);
        return 2;
    }
    QTemporaryDir tmp;
    qputenv("TRAINER_WORKSPACE", tmp.filePath("ws").toUtf8());
    qputenv("XDG_DATA_HOME", tmp.filePath("data").toUtf8());
    qputenv("XDG_CONFIG_HOME", tmp.filePath("config").toUtf8());
    theme::apply(theme::Mode::Light);

    Engine engine(QString::fromLocal8Bit(argv[0]));
    // Progression fictive : les trois premiers chapitres lus et réussis.
    const QByteArray progress = engine.config().progress_path;
    const catalog &cat = engine.exercises();
    for (size_t i = 0; i < cat.nlessons && i < 4; i++)
        progress_record(progress.constData(), cat.lessons[i].id, EVENT_LESSON_READ, 0);
    for (size_t i = 0; i < cat.len && i < 9; i++)
        progress_record(progress.constData(), cat.items[i].id, EVENT_CHECK_PASSED, 0);
    engine.reloadProgress();

    MainWindow window(engine);
    window.resize(1240, 780);
    window.show();
    ExerciseView *view = window.exerciseView();
    view->confirm = [](const QString &) { return true; };

    // Un exercice commencé, avec le code de départ qui échoue.
    window.catalogView()->selectId(QStringLiteral("cpp.algorithms.leaderboard"));
    view->findChild<QPushButton *>(QStringLiteral("startButton"))->click();
    QSignalSpy spy(view, &ExerciseView::checkFinished);
    view->findChild<QPushButton *>(QStringLiteral("checkButton"))->click();
    if (!spy.wait(120000)) {
        std::fprintf(stderr, "la vérification n'a pas abouti\n");
        return 1;
    }
    save(&window, out, "echec");

    window.catalogView()->selectId(QStringLiteral("cpp.lesson.algorithms"));
    save(&window, out, "cours");

    window.setThemeMode(theme::Mode::Dark);
    save(&window, out, "cours-sombre");
    window.catalogView()->selectId(QStringLiteral("cpp.algorithms.leaderboard"));
    save(&window, out, "echec-sombre");

    // Recherche dans le catalogue.
    window.setThemeMode(theme::Mode::Light);
    window.findChild<QLineEdit *>(QStringLiteral("search"))->setText(QStringLiteral("pointeur"));
    save(&window, out, "recherche");
    return 0;
}
