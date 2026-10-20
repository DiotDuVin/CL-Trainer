// mainwindow.h — fenêtre principale : catalogue à gauche, exercice à droite.
#pragma once

#include <QFileSystemWatcher>
#include <QMainWindow>
#include <QTimer>

class CatalogView;
class Engine;
class ExerciseView;
class QLabel;
class QProgressBar;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(Engine &engine, QWidget *parent = nullptr);

    CatalogView *catalogView() const { return catalog_; }
    ExerciseView *exerciseView() const { return exercise_; }

public slots:
    void goToNext();      // sélectionne l'exercice recommandé
    void reloadAll();     // relit exercices et progression
    void refreshProgress(); // relit la progression (modifiée ici ou par le terminal)

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void updateSummary();
    void watchProgress();

    Engine &engine_;
    CatalogView *catalog_;
    ExerciseView *exercise_;
    QProgressBar *progressBar_;
    QLabel *weakLabel_;
    // Surveille le dossier de la progression : quand le terminal (ou VS Code
    // via Ctrl+Maj+B) enregistre une vérification, la fenêtre se met à jour.
    QFileSystemWatcher watcher_;
    QTimer debounce_;  // regroupe les notifications rapprochées
};
