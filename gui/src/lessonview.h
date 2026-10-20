// lessonview.h — panneau de droite quand un cours est sélectionné.
#pragma once

#include <QWidget>

class Engine;
class QLabel;
class QPushButton;
class QTextBrowser;

class LessonView : public QWidget {
    Q_OBJECT

public:
    explicit LessonView(Engine &engine, QWidget *parent = nullptr);

    void showLesson(const QString &id);
    QString lessonId() const { return id_; }

public slots:
    void refresh();

signals:
    void progressChanged();
    void openExercise(const QString &id);   // « Passer aux exercices »
    void message(const QString &text);

private slots:
    void onMarkRead();
    void onPractice();

private:
    QString firstExerciseToDo() const;      // premier exercice non réussi du chapitre

    Engine &engine_;
    QString id_;
    QLabel *title_;
    QLabel *langChip_;
    QLabel *timeChip_;
    QLabel *readChip_;
    QTextBrowser *body_;
    QPushButton *readButton_;
    QPushButton *practiceButton_;
};
