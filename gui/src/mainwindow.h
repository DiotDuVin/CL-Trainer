// mainwindow.h — fenêtre principale : catalogue à gauche, exercice à droite.
#pragma once

#include <QFileSystemWatcher>
#include <QMainWindow>
#include <QTimer>

#include "theme.h"

class CatalogView;
class Engine;
class ExerciseView;
class LessonView;
class QStackedWidget;
class QLabel;
class QLineEdit;
class QProgressBar;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(Engine &engine, QWidget *parent = nullptr);

    CatalogView *catalogView() const { return catalog_; }
    ExerciseView *exerciseView() const { return exercise_; }
    LessonView *lessonView() const { return lesson_; }
    bool showingLesson() const;

public slots:
    void goToNext();      // sélectionne l'exercice recommandé
    void reloadAll();     // relit exercices et progression
    void refreshProgress(); // relit la progression (modifiée ici ou par le terminal)
    void setThemeMode(theme::Mode mode); // applique, enregistre et redessine

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void updateSummary();
    void watchProgress();
    void buildToolbar();
    void restyle();       // redessine les vues après un changement de thème

    Engine &engine_;
    CatalogView *catalog_;
    ExerciseView *exercise_;
    LessonView *lesson_;
    QStackedWidget *stack_;   // à droite : l'exercice OU le cours sélectionné
    QLineEdit *search_;
    QProgressBar *progressBar_;
    QLabel *progressLabel_;
    QLabel *weakLabel_;
    // Surveille le dossier de la progression : quand le terminal (ou VS Code
    // via Ctrl+Maj+B) enregistre une vérification, la fenêtre se met à jour.
    QFileSystemWatcher watcher_;
    QTimer debounce_;  // regroupe les notifications rapprochées
};
