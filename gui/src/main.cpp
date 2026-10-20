// main.cpp — point d'entrée de l'application graphique.
#include <QApplication>
#include <QMessageBox>

#include "engine.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    // Nom utilisé par QSettings pour le fichier de préférences.
    QApplication::setOrganizationName(QStringLiteral("CL-Trainer"));
    QApplication::setApplicationName(QStringLiteral("CL-Trainer"));
    QApplication::setApplicationVersion(QStringLiteral(TRAINER_VERSION));
    QApplication::setDesktopFileName(QStringLiteral("cl-trainer")); // icône sous Wayland

    try {
        Engine engine(QString::fromLocal8Bit(argv[0]));
        MainWindow window(engine);
        window.show();
        return app.exec(); // boucle d'événements : clics, timers, signaux...
    } catch (const std::exception &err) {
        // Erreur au démarrage (dossier d'exercices introuvable...).
        QMessageBox::critical(nullptr, QStringLiteral("CL-Trainer"), QString::fromUtf8(err.what()));
        return 2;
    }
}
