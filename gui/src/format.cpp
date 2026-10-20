// format.cpp — voir format.h.
#include "format.h"

#include <cstring>

#include "engine.h" // qs()
#include "fs.h"     // libtrainer : path_join, fs_read_file
#include "text.h"   // libtrainer : escape_visible

namespace format {

namespace {

constexpr int kMaxLines = 40; // lignes affichées au maximum par bloc

QString pre(const QString &content)
{
    QStringList lines = content.split(QLatin1Char('\n'));
    if (!lines.isEmpty() && lines.last().isEmpty())
        lines.removeLast();
    QString note;
    if (lines.size() > kMaxLines) {
        note = QStringLiteral("<br><i>… %1 ligne(s) de plus</i>").arg(lines.size() - kMaxLines);
        lines = lines.mid(0, kMaxLines);
    }
    if (lines.isEmpty())
        return QStringLiteral("<p><i>(vide)</i></p>");
    return QStringLiteral("<pre>%1</pre>%2")
        .arg(lines.join(QLatin1Char('\n')).toHtmlEscaped(), note);
}

// Rend visibles les caractères invisibles (\t, \r...) d'une ligne.
QString visible(const char *line)
{
    if (!line)
        return QStringLiteral("<i>(fin de la sortie)</i>");
    strbuf sb = STRBUF_INIT;
    escape_visible(&sb, line, std::strlen(line));
    QString s = QStringLiteral("<code>«%1»</code>").arg(qs(sb_cstr(&sb)).toHtmlEscaped());
    sb_free(&sb);
    return s;
}

QString fenceLang(const char *lang)
{
    return std::strcmp(lang, "csharp") == 0 ? QStringLiteral("csharp") : qs(lang);
}

} // namespace

QString statusSymbol(exercise_status st, bool unlocked)
{
    switch (st) {
    case STATUS_PASSED:  return QStringLiteral("✓");
    case STATUS_FAILED:  return QStringLiteral("✗");
    case STATUS_STARTED: return QStringLiteral("●");
    case STATUS_NEW:     break;
    }
    return unlocked ? QStringLiteral("○") : QStringLiteral("–");
}

QColor statusColor(exercise_status st, bool unlocked)
{
    switch (st) {
    case STATUS_PASSED:  return QColor(0x2e, 0x9e, 0x44);
    case STATUS_FAILED:  return QColor(0xd0, 0x3a, 0x3a);
    case STATUS_STARTED: return QColor(0xd0, 0x8a, 0x10);
    case STATUS_NEW:     break;
    }
    return unlocked ? QColor() : QColor(0x88, 0x88, 0x88); // QColor() : couleur du thème
}

QString levelStars(int level)
{
    QString s;
    for (int i = 1; i <= 5; i++)
        s += i <= level ? QStringLiteral("★") : QStringLiteral("☆");
    return s;
}

QString langName(const char *lang)
{
    if (const runner *r = runner_for(lang))
        return qs(r->display_name);
    if (std::strcmp(lang, "csharp") == 0)
        return QStringLiteral("C#");
    if (std::strcmp(lang, "rust") == 0)
        return QStringLiteral("Rust");
    if (std::strcmp(lang, "git") == 0)
        return QStringLiteral("Git");
    return qs(lang);
}

QString statementMarkdown(const exercise &ex, int hintsShown)
{
    QString md = qs(ex.statement);
    const int total = static_cast<int>(ex.hints.len);
    for (int i = 0; i < hintsShown && i < total; i++)
        md += QStringLiteral("\n\n---\n\n**💡 Indice %1/%2**\n\n%3")
                  .arg(i + 1).arg(total).arg(qs(ex.hints.items[i]));
    return md;
}

QString correctionMarkdown(const exercise &ex)
{
    QString md = QStringLiteral("## Correction\n\n") + qs(ex.explanation);
    if (ex.common_mistakes)
        md += QStringLiteral("\n\n## Erreurs fréquentes\n\n") + qs(ex.common_mistakes);
    return md;
}

QString solutionMarkdown(const exercise &ex)
{
    QString md;
    strvec files = STRVEC_INIT;
    for (size_t i = 0; i < ex.files.len; i++)
        sv_push(&files, ex.files.items[i]);
    if (files.len == 0)
        if (const runner *r = runner_for(ex.lang))
            sv_push(&files, r->default_file);

    strbuf base = STRBUF_INIT, path = STRBUF_INIT, content = STRBUF_INIT;
    path_join(&base, ex.dir, "solution");
    for (size_t i = 0; i < files.len; i++) {
        path_join(&path, base.data, files.items[i]);
        sb_clear(&content);
        if (fs_read_file(path.data, &content) != 0)
            continue;
        md += QStringLiteral("## %1\n\n```%2\n%3```\n\n")
                  .arg(qs(files.items[i]), fenceLang(ex.lang), qs(sb_cstr(&content)));
    }
    sv_free(&files);
    sb_free(&base);
    sb_free(&path);
    sb_free(&content);
    return md + correctionMarkdown(ex);
}

QString testDetailsHtml(const exercise &ex, const test_case &tc, const test_outcome &o)
{
    QString html;
    if (o.result == VERDICT_PASS) {
        html += QStringLiteral("<h3 style=\"color:#2e9e44\">✓ %1</h3><p>Réussi en %2 ms.</p>")
                    .arg(qs(tc.name).toHtmlEscaped()).arg(o.res.elapsed_ms);
    } else {
        html += QStringLiteral("<h3 style=\"color:#d03a3a\">✗ %1 — %2</h3>")
                    .arg(qs(tc.name).toHtmlEscaped(), qs(verdict_label(o.result)));
        strbuf why = STRBUF_INIT;
        check_explain(&ex, &tc, &o, &why);
        if (why.len)
            html += QStringLiteral("<p>%1</p>").arg(qs(why.data).toHtmlEscaped());
        sb_free(&why);
    }

    if (tc.hidden) {
        // Même règle que dans le terminal : on ne montre pas un test caché.
        if (o.result != VERDICT_PASS)
            html += QStringLiteral("<p><i>Test caché : pense aux cas limites (valeurs "
                                   "extrêmes, entrée vide, négatifs, espaces...).</i></p>");
        return html;
    }

    if (tc.args.len > 0) {
        QStringList args;
        for (size_t i = 0; i < tc.args.len; i++)
            args << QStringLiteral("«%1»").arg(qs(tc.args.items[i]).toHtmlEscaped());
        html += QStringLiteral("<p><b>Arguments :</b> %1</p>").arg(args.join(QLatin1Char(' ')));
    }
    html += QStringLiteral("<p><b>Entrée</b></p>") + pre(qs(tc.input));

    if (o.result == VERDICT_WRONG_OUTPUT) {
        output_diff d;
        compare_first_diff(tc.expected, sb_cstr(&o.res.out), ex.compare, &d);
        if (d.empty_output)
            html += QStringLiteral("<p>Le programme n'a rien affiché.</p>");
        else if (d.same_lines)
            html += QStringLiteral("<p>Les lignes sont identiques mais les fins de ligne "
                                   "diffèrent (retour à la ligne final, \\r\\n ?).</p>");
        else
            html += QStringLiteral("<p><b>Première différence à la ligne %1</b></p>"
                                   "<table cellspacing=\"4\"><tr><td>attendu</td><td>%2</td></tr>"
                                   "<tr><td>obtenu</td><td>%3</td></tr></table>")
                        .arg(d.line).arg(visible(d.expected_line), visible(d.actual_line));
        output_diff_free(&d);
        html += QStringLiteral("<p><b>Sortie attendue</b></p>") + pre(qs(tc.expected));
    }
    html += QStringLiteral("<p><b>Sortie obtenue</b></p>") + pre(qs(sb_cstr(&o.res.out)));
    if (o.res.err.len > 0)
        html += QStringLiteral("<p><b>Erreurs (stderr)</b></p>") + pre(qs(sb_cstr(&o.res.err)));
    return html;
}

QString buildLogHtml(const char *log, bool failed)
{
    QString html = failed
        ? QStringLiteral("<h3 style=\"color:#d03a3a\">✗ La compilation a échoué</h3>"
                         "<p>Corrige d'abord la <b>première</b> erreur : les suivantes en "
                         "découlent souvent.</p>")
        : QStringLiteral("<h3 style=\"color:#d08a10\">Avertissements du compilateur</h3>"
                         "<p>Un avertissement signale souvent un vrai bug : en entreprise, on "
                         "compile généralement avec -Werror.</p>");
    return html + pre(qs(log));
}

} // namespace format
