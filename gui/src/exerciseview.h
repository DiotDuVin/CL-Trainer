// exerciseview.h — panneau de droite : énoncé, actions, résultats, correction.
#pragma once

#include <functional>
#include <memory>

#include <QFutureWatcher>
#include <QWidget>

class Engine;
class CheckResult;
class QLabel;
class QListWidget;
class QPushButton;
class QTabWidget;
class QTextBrowser;

class ExerciseView : public QWidget {
    Q_OBJECT

public:
    explicit ExerciseView(Engine &engine, QWidget *parent = nullptr);
    ~ExerciseView() override;

    void showExercise(const QString &id);  // id vide : rien de sélectionné
    QString exerciseId() const { return id_; }
    bool isChecking() const;

    // Demande de confirmation (oui/non). Remplaçable : les tests y mettent
    // une fonction qui répond toujours « oui », au lieu d'une boîte de
    // dialogue qui bloquerait. C'est de l'« injection de dépendance ».
    std::function<bool(const QString &question)> confirm;

public slots:
    void refresh(); // relit la progression et met à jour boutons et énoncé

signals:
    void progressChanged();            // la progression a été modifiée ici
    void checkFinished(bool passed);   // fin d'une vérification
    void message(const QString &text); // texte pour la barre d'état

private slots:
    void onStart();
    void onOpenEditor();
    void onOpenFolder();
    void onCheck();
    void onCheckFinished();
    void onHint();
    void onSolution();
    void onTestSelected(int row);

private:
    void showError(const QString &text);
    void clearResults();

    Engine &engine_;   // référence : la vue ne possède pas l'Engine
    QString id_;       // exercice affiché (jamais de pointeur : voir engine.h)

    QLabel *title_;
    QLabel *meta_;
    QLabel *banner_;
    QTabWidget *tabs_;
    QTextBrowser *statement_;
    QListWidget *tests_;
    QTextBrowser *details_;
    QTextBrowser *correction_;
    QPushButton *startButton_;
    QPushButton *editorButton_;
    QPushButton *folderButton_;
    QPushButton *checkButton_;
    QPushButton *hintButton_;
    QPushButton *solutionButton_;

    // Vérification en arrière-plan : QtConcurrent lance check_run dans un
    // autre thread, le QFutureWatcher prévient la fenêtre quand c'est fini.
    QFutureWatcher<std::shared_ptr<CheckResult>> watcher_;
    std::shared_ptr<CheckResult> last_;  // dernier résultat affiché
    bool solutionShown_ = false;         // solution affichée dans l'onglet Correction
};
