// format.cpp — voir format.h.
#include "format.h"

#include <cstring>

#include <QSet>
#include <QTextBlock>
#include <QTextTable>
#include <QTextBrowser>
#include <QTextCursor>

#include "engine.h" // qs()
#include "theme.h"
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
    // Un tableau d'une case : la seule façon fiable d'avoir un fond coloré
    // avec des marges intérieures dans le HTML simplifié de Qt.
    return QStringLiteral("<table width=\"100%\" cellpadding=\"8\" bgcolor=\"%1\"><tr><td>"
                          "<pre>%2</pre></td></tr></table>%3")
        .arg(theme::color(theme::Role::CodeBackground).name(),
             lines.join(QLatin1Char('\n')).toHtmlEscaped(), note);
}

// Qt découpe un bloc de code en un paragraphe par ligne : deux blocs de code
// consécutifs se fondraient en un seul. On glisse entre eux un paragraphe
// invisible (une espace insécable).
QString separateCodeBlocks(const QString &markdown)
{
    const QStringList lines = markdown.split(QLatin1Char('\n'));
    QStringList out;
    bool inFence = false, justClosed = false;
    for (const QString &line : lines) {
        const bool fence = line.trimmed().startsWith(QLatin1String("```"));
        if (fence && !inFence && justClosed)
            out << QStringLiteral("\u00a0") << QString();
        if (fence)
            inFence = !inFence;
        if (fence && !inFence)
            justClosed = true;
        else if (!line.trimmed().isEmpty())
            justClosed = false;
        out << line;
    }
    return out.join(QLatin1Char('\n'));
}

QString colored(theme::Role role)
{
    return theme::color(role).name();
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
    case STATUS_PASSED:  return theme::color(theme::Role::Success);
    case STATUS_FAILED:  return theme::color(theme::Role::Failure);
    case STATUS_STARTED: return theme::color(theme::Role::Warning);
    case STATUS_NEW:     break;
    }
    // QColor() invalide : couleur de texte normale du thème.
    return unlocked ? QColor() : theme::color(theme::Role::Muted);
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

void setMarkdown(QTextBrowser *view, const QString &markdown)
{
    QTextDocument *doc = view->document();
    doc->setMarkdown(separateCodeBlocks(markdown), QTextDocument::MarkdownFeatures(QTextDocument::MarkdownDialectGitHub |
                                                               QTextDocument::MarkdownNoHTML));
    doc->setDocumentMargin(16);

    // Le Markdown de Qt n'a pas de feuille de style : on parcourt le document
    // pour donner un fond aux blocs de code (et un autre aux sorties de
    // programme, marquées « text »), au code en ligne et aux tableaux.
    const QColor code = theme::color(theme::Role::CodeBackground);
    const QColor output = theme::color(theme::Role::OutputBackground);
    const QColor inlineCode = theme::color(theme::Role::SoftBackground);

    // Texte en chasse fixe : marqué « fixed pitch » (code en ligne) ou par la
    // famille « monospace » (blocs de code indentés), selon le cas.
    const auto isMono = [](const QTextCharFormat &cf) {
        if (cf.fontFixedPitch())
            return true;
        const QStringList families = cf.fontFamilies().toStringList();
        return families.contains(QLatin1String("monospace"));
    };

    // Ligne d'un bloc de code : bloc « ``` », ou bloc indenté de 4 espaces.
    // Ce dernier n'a aucune marque : on le reconnaît à ses marges nulles (un
    // paragraphe a des marges), hors liste, et à son texte tout en chasse fixe.
    const auto isCode = [&isMono](const QTextBlock &b) {
        if (!b.isValid())
            return false;
        const QTextBlockFormat bf = b.blockFormat();
        if (bf.hasProperty(QTextFormat::BlockCodeFence))
            return true;
        if (b.textList() || b.text().isEmpty() || bf.topMargin() > 0 || bf.bottomMargin() > 0)
            return false;
        for (QTextBlock::iterator it = b.begin(); !it.atEnd(); ++it)
            if (!isMono(it.fragment().charFormat()))
                return false;
        return true;
    };

    // D'abord repérer les lignes de code, PUIS modifier : les marges ajoutées
    // fausseraient la détection des lignes suivantes.
    QSet<int> codeBlocks;
    for (QTextBlock b = doc->begin(); b.isValid(); b = b.next())
        if (isCode(b))
            codeBlocks.insert(b.blockNumber());
    const auto inCode = [&codeBlocks](const QTextBlock &b) {
        return b.isValid() && codeBlocks.contains(b.blockNumber());
    };

    QTextCursor cursor(doc);
    cursor.beginEditBlock();
    for (QTextBlock b = doc->begin(); b.isValid(); b = b.next()) {
        if (inCode(b)) {
            QTextBlockFormat bf = b.blockFormat();
            const bool isOutput = bf.stringProperty(QTextFormat::BlockCodeLanguage) == QLatin1String("text");
            bf.setBackground(isOutput ? output : code);
            bf.setLeftMargin(4);
            // Un peu d'air avant la première ligne et après la dernière.
            if (!inCode(b.previous()))
                bf.setTopMargin(8);
            if (!inCode(b.next()))
                bf.setBottomMargin(8);
            cursor.setPosition(b.position());
            cursor.setBlockFormat(bf);
            continue;
        }
        for (QTextBlock::iterator it = b.begin(); !it.atEnd(); ++it) {
            const QTextFragment frag = it.fragment();
            if (!frag.isValid() || !isMono(frag.charFormat()))
                continue;
            QTextCharFormat cf;
            cf.setBackground(inlineCode);
            cursor.setPosition(frag.position());
            cursor.setPosition(frag.position() + frag.length(), QTextCursor::KeepAnchor);
            cursor.mergeCharFormat(cf);
        }
    }

    // Tableaux : bordure fine et discrète, cellules aérées, en-tête teinté.
    for (QTextFrame *frame : doc->rootFrame()->childFrames()) {
        auto *table = qobject_cast<QTextTable *>(frame);
        if (!table)
            continue;
        QTextTableFormat tf = table->format();
        tf.setBorder(1);
        tf.setBorderBrush(theme::color(theme::Role::Border));
        tf.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
        tf.setBorderCollapse(true);
        tf.setCellSpacing(0);
        tf.setCellPadding(6);
        table->setFormat(tf);
        for (int col = 0; col < table->columns(); col++) {
            QTextTableCell cell = table->cellAt(0, col);
            QTextCharFormat cf = cell.format();
            cf.setBackground(inlineCode);
            cell.setFormat(cf);
        }
    }
    cursor.endEditBlock();
    // Le curseur de la vue est resté en fin de document : Qt ferait défiler
    // jusqu'à lui à l'affichage. On le replace au début.
    view->moveCursor(QTextCursor::Start);
}

QString lessonMarkdown(const lesson &l)
{
    QString md = qs(l.body);
    // « example » ne sert qu'à la validation : inutile de l'afficher.
    md.replace(QStringLiteral("```%1 example").arg(qs(l.lang)), QStringLiteral("```%1").arg(qs(l.lang)));
    // Sans libellé, la sortie se confond avec le code qui la précède.
    md.replace(QStringLiteral("\n```output\n"),
               QStringLiteral("\n*Sortie du programme :*\n\n```text\n"));
    return md;
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
        html += QStringLiteral("<h3 style=\"color:%1\">✓ %2</h3><p>Réussi en %3 ms.</p>")
                    .arg(colored(theme::Role::Success), qs(tc.name).toHtmlEscaped())
                    .arg(o.res.elapsed_ms);
    } else {
        html += QStringLiteral("<h3 style=\"color:%1\">✗ %2 — %3</h3>")
                    .arg(colored(theme::Role::Failure), qs(tc.name).toHtmlEscaped(),
                         qs(verdict_label(o.result)));
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
                                   "<table cellspacing=\"0\" cellpadding=\"4\">"
                                   "<tr><td style=\"color:%2\">attendu</td><td>%4</td></tr>"
                                   "<tr><td style=\"color:%3\">obtenu</td><td>%5</td></tr></table>")
                        .arg(d.line)
                        .arg(colored(theme::Role::Success), colored(theme::Role::Failure),
                             visible(d.expected_line), visible(d.actual_line));
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
        ? QStringLiteral("<h3 style=\"color:%1\">✗ La compilation a échoué</h3>"
                         "<p>Corrige d'abord la <b>première</b> erreur : les suivantes en "
                         "découlent souvent.</p>").arg(colored(theme::Role::Failure))
        : QStringLiteral("<h3 style=\"color:%1\">Avertissements du compilateur</h3>"
                         "<p>Un avertissement signale souvent un vrai bug : en entreprise, on "
                         "compile généralement avec -Werror.</p>").arg(colored(theme::Role::Warning));
    return html + pre(qs(log));
}

} // namespace format
