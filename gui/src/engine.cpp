// engine.cpp — voir engine.h.
#include "engine.h"

#include <cstring>
#include <stdexcept>

namespace {

// Transforme le dernier message d'erreur C en exception C++.
[[noreturn]] void throwLastError(const char *context)
{
    throw std::runtime_error(std::string(context) + " : " + err_msg());
}

} // namespace

CheckResult::CheckResult()
{
    // Structures C : on les met à zéro pour que *_free soit toujours sûr.
    std::memset(&ex, 0, sizeof ex);
    std::memset(&report, 0, sizeof report);
}

CheckResult::~CheckResult()
{
    check_report_free(&report);
    exercise_free(&ex);
}

Engine::Engine(const QString &argv0)
{
    const QByteArray arg = argv0.toUtf8();
    if (config_resolve(nullptr, nullptr, arg.constData(), &cfg_) != 0) {
        config_free(&cfg_);
        throwLastError("configuration");
    }
    // Piège du C++ : si un constructeur lève une exception, l'objet n'est
    // pas construit et son destructeur n'est JAMAIS appelé. Il faut donc
    // libérer ici ce qui a déjà été acquis.
    try {
        reload();
    } catch (...) {
        progress_free(&prog_);
        catalog_free(&cat_);
        config_free(&cfg_);
        throw; // relance la même exception
    }
}

Engine::~Engine()
{
    progress_free(&prog_);
    catalog_free(&cat_);
    config_free(&cfg_);
}

void Engine::reload()
{
    catalog fresh{};
    if (catalog_load(cfg_.exercises_dir, &fresh) != 0)
        throwLastError("chargement des exercices");
    // On ne remplace l'ancien catalogue qu'une fois le nouveau chargé : en cas
    // d'erreur, l'Engine reste dans un état cohérent (« garantie forte »).
    catalog_free(&cat_);
    cat_ = fresh;
    reloadProgress();
}

void Engine::reloadProgress()
{
    progress_free(&prog_);
    progressWarning_.clear();
    if (progress_load(cfg_.progress_path, &prog_) != 0)
        progressWarning_ = QStringLiteral("progression illisible, ignorée : ") + qs(err_msg());
}

QStringList Engine::warnings() const
{
    QStringList list;
    for (size_t i = 0; i < cat_.errors.len; i++)
        list << qs(cat_.errors.items[i]);
    if (!progressWarning_.isEmpty())
        list << progressWarning_;
    return list;
}

const exercise *Engine::find(const QString &id) const
{
    return catalog_find(&cat_, id.toUtf8().constData());
}

const lesson *Engine::findLesson(const QString &id) const
{
    return catalog_find_lesson(&cat_, id.toUtf8().constData());
}

bool Engine::lessonRead(const lesson &l) const
{
    return progress_status(&prog_, l.id) == STATUS_PASSED;
}

void Engine::markLessonRead(const lesson &l)
{
    if (progress_record(cfg_.progress_path, l.id, EVENT_LESSON_READ, 0) != 0)
        throwLastError("enregistrement de la progression");
    reloadProgress();
}

exercise_status Engine::status(const exercise &ex) const
{
    return progress_status(&prog_, ex.id);
}

const progress_entry *Engine::entry(const exercise &ex) const
{
    return progress_find(&prog_, ex.id);
}

bool Engine::unlocked(const exercise &ex) const
{
    return exercise_unlocked(&cat_, &prog_, &ex);
}

bool Engine::started(const exercise &ex) const
{
    return workspace_exists(&cfg_, &ex);
}

QString Engine::workdir(const exercise &ex) const
{
    strbuf sb = STRBUF_INIT;
    workspace_dir(&cfg_, &ex, &sb);
    QString dir = qs(sb_cstr(&sb));
    sb_free(&sb);
    return dir;
}

start_result Engine::start(const exercise &ex, bool force)
{
    start_result result;
    if (workspace_start(&cfg_, &ex, force ? 1 : 0, &result) != 0)
        throwLastError("préparation de l'exercice");
    record(ex, EVENT_START);
    return result;
}

void Engine::record(const exercise &ex, progress_event ev, int arg)
{
    if (progress_record(cfg_.progress_path, ex.id, ev, arg) != 0)
        throwLastError("enregistrement de la progression");
    reloadProgress();
}

recommendation Engine::next() const
{
    recommendation r;
    recommend_next(&cat_, &prog_, nullptr, &r);
    return r;
}

Engine::Summary Engine::summary() const
{
    stats st;
    stats_compute(&cat_, &prog_, &st);
    Summary s;
    s.passed = st.passed;
    s.total = st.total;
    s.lessons = cat_.nlessons;
    for (size_t i = 0; i < cat_.nlessons; i++)
        if (lessonRead(cat_.lessons[i]))
            s.lessonsRead++;
    for (size_t i = 0; i < st.len; i++)
        if (stats_topic_is_weak(&st.topics[i]))
            s.weakTopics << qs(st.topics[i].topic);
    stats_free(&st);
    return s;
}

std::shared_ptr<CheckResult> Engine::runCheck(const QString &exerciseDir, const QString &workdir)
{
    // make_shared : un seul bloc mémoire pour l'objet et son compteur de
    // références. Le shared_ptr peut être copié entre threads sans danger.
    auto result = std::make_shared<CheckResult>();
    if (exercise_load(exerciseDir.toUtf8().constData(), &result->ex) != 0) {
        result->error = qs(err_msg());
        return result;
    }
    if (check_run(&result->ex, workdir.toUtf8().constData(), &result->report) != 0) {
        result->error = qs(err_msg());
        return result;
    }
    result->ok = true;
    return result;
}
