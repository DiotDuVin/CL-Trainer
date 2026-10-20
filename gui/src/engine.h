// engine.h — enveloppe C++ de libtrainer (la bibliothèque C du moteur).
//
// Deux idées C++ importantes ici :
//
// 1. RAII (« Resource Acquisition Is Initialization ») : le constructeur
//    acquiert les ressources (catalog_load...), le destructeur les libère
//    (catalog_free...). Impossible d'oublier un free, même si une exception
//    est levée : le destructeur est appelé automatiquement.
//
// 2. Frontière C / C++ : la bibliothèque C signale les erreurs par -1 +
//    err_msg() ; ici on les transforme en exceptions (std::runtime_error),
//    le mécanisme idiomatique du C++.
#pragma once

#include <memory>
#include <QString>
#include <QStringList>

#include "trainer.h"

// Résultat d'une vérification, produit dans un thread séparé. Il possède sa
// PROPRE copie de l'exercice : le thread ne touche jamais au catalogue de la
// fenêtre, qui peut être rechargé pendant ce temps.
class CheckResult {
public:
    CheckResult();
    ~CheckResult();
    // Non copiable : deux copies libéreraient deux fois les mêmes pointeurs C.
    CheckResult(const CheckResult &) = delete;
    CheckResult &operator=(const CheckResult &) = delete;

    bool ok = false;        // la vérification a pu avoir lieu
    QString error;          // sinon, pourquoi
    exercise ex;            // copie chargée depuis le disque
    check_report report;
};

class Engine {
public:
    // argv0 : chemin du programme (pour trouver les dossiers voisins).
    // Lève std::runtime_error si la configuration est impossible.
    explicit Engine(const QString &argv0);
    ~Engine();
    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;

    void reload();          // catalogue + progression (lève une exception)
    void reloadProgress();  // progression seule (fichier illisible = vide)

    const trainer_config &config() const { return cfg_; }
    const catalog &exercises() const { return cat_; }
    QStringList warnings() const;        // problèmes du catalogue / de la progression

    // Les pointeurs renvoyés restent valides jusqu'au prochain reload() :
    // les vues mémorisent donc un id (QString), jamais un pointeur.
    const exercise *find(const QString &id) const;
    const lesson *findLesson(const QString &id) const;
    bool lessonRead(const lesson &l) const;
    void markLessonRead(const lesson &l);                              // lève une exception
    exercise_status status(const exercise &ex) const;
    const progress_entry *entry(const exercise &ex) const;
    bool unlocked(const exercise &ex) const;
    bool started(const exercise &ex) const;
    QString workdir(const exercise &ex) const;

    start_result start(const exercise &ex, bool force);               // lève une exception
    void record(const exercise &ex, progress_event ev, int arg = 0);  // lève une exception
    recommendation next() const;

    struct Summary {
        size_t passed = 0, total = 0;
        QStringList weakTopics;
    };
    Summary summary() const;

    // Thread-safe : n'utilise que ses arguments (pas l'état de l'Engine).
    static std::shared_ptr<CheckResult> runCheck(const QString &exerciseDir,
                                                 const QString &workdir);

private:
    trainer_config cfg_{};
    catalog cat_{};
    progress prog_{};
    QString progressWarning_;
};

// Conversion des chaînes C (UTF-8) en QString.
inline QString qs(const char *s) { return QString::fromUtf8(s ? s : ""); }
