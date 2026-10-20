// theme.cpp — voir theme.h.
#include "theme.h"

#include <QApplication>
#include <QPalette>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>
#include <QWidget>

namespace theme {

namespace {

// Une table de couleurs par thème. Les teintes sont choisies pour rester
// lisibles (contraste suffisant) sur le fond de leur thème.
struct Colors {
    QColor window, surface, alternate, text, muted, border, button;
    QColor accent, accentText, success, failure, warning;
    QColor soft, code, output, notice;
};

const Colors kLight = {
    QColor(0xf4, 0xf5, 0xf7), QColor(0xff, 0xff, 0xff), QColor(0xf6, 0xf8, 0xfa),
    QColor(0x1f, 0x23, 0x28), QColor(0x65, 0x6d, 0x76), QColor(0xd0, 0xd7, 0xde),
    QColor(0xff, 0xff, 0xff),
    QColor(0x2f, 0x6f, 0xeb), QColor(0xff, 0xff, 0xff), QColor(0x1a, 0x7f, 0x37),
    QColor(0xcf, 0x22, 0x2e), QColor(0x9a, 0x67, 0x00),
    QColor(0xea, 0xee, 0xf2), QColor(0xf3, 0xf4, 0xf6), QColor(0xee, 0xf4, 0xff),
    QColor(0xff, 0xf8, 0xe1),
};

const Colors kDark = {
    QColor(0x1b, 0x1e, 0x24), QColor(0x23, 0x27, 0x2e), QColor(0x2a, 0x2e, 0x36),
    QColor(0xe6, 0xe8, 0xeb), QColor(0x95, 0x9d, 0xa8), QColor(0x3a, 0x40, 0x4a),
    QColor(0x2d, 0x32, 0x3a),
    QColor(0x4c, 0x8d, 0xff), QColor(0xff, 0xff, 0xff), QColor(0x3f, 0xb9, 0x50),
    QColor(0xf8, 0x51, 0x49), QColor(0xd2, 0x99, 0x22),
    QColor(0x30, 0x36, 0x3f), QColor(0x18, 0x1b, 0x20), QColor(0x1c, 0x26, 0x38),
    QColor(0x3a, 0x31, 0x1c),
};

Mode g_mode = Mode::Auto;
bool g_dark = false;
QPalette g_systemPalette;   // palette d'origine, pour détecter un système sombre
bool g_systemPaletteSaved = false;

const Colors &current() { return g_dark ? kDark : kLight; }

bool systemPrefersDark()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    // Qt 6.5+ connaît directement le réglage du bureau (KDE, GNOME...).
    switch (QGuiApplication::styleHints()->colorScheme()) {
    case Qt::ColorScheme::Dark:  return true;
    case Qt::ColorScheme::Light: return false;
    case Qt::ColorScheme::Unknown: break;
    }
#endif
    // Sinon : un fond de fenêtre sombre dans la palette du système.
    return g_systemPalette.color(QPalette::Window).lightness() < 128;
}

QPalette makePalette(const Colors &c)
{
    QPalette p;
    p.setColor(QPalette::Window, c.window);
    p.setColor(QPalette::WindowText, c.text);
    p.setColor(QPalette::Base, c.surface);
    p.setColor(QPalette::AlternateBase, c.alternate);
    p.setColor(QPalette::Text, c.text);
    p.setColor(QPalette::Button, c.button);
    p.setColor(QPalette::ButtonText, c.text);
    p.setColor(QPalette::Highlight, c.accent);
    p.setColor(QPalette::HighlightedText, c.accentText);
    p.setColor(QPalette::Link, c.accent);
    p.setColor(QPalette::ToolTipBase, c.surface);
    p.setColor(QPalette::ToolTipText, c.text);
    p.setColor(QPalette::PlaceholderText, c.muted);
    p.setColor(QPalette::Mid, c.border);
    // Widgets désactivés : texte atténué.
    for (QPalette::ColorRole role : { QPalette::WindowText, QPalette::Text, QPalette::ButtonText })
        p.setColor(QPalette::Disabled, role, c.muted);
    p.setColor(QPalette::Disabled, QPalette::Highlight, c.border);
    return p;
}

// Feuille de style : uniquement ce que la palette ne sait pas faire
// (coins arrondis, marges, bouton principal, badges...).
QString makeStyleSheet(const Colors &c)
{
    QString qss = QStringLiteral(R"(
QToolBar { border: none; padding: 4px 8px; spacing: 6px; }
QToolButton { padding: 4px 10px; border-radius: 6px; }
QToolButton:hover { background: @soft; }

QPushButton { padding: 5px 14px; border: 1px solid @border; border-radius: 6px;
              background: @button; }
QPushButton:hover { border-color: @accent; }
QPushButton:pressed { background: @soft; }
QPushButton:disabled { color: @muted; border-color: @border; }
QPushButton[primary="true"] { background: @accent; color: @accentText; border-color: @accent;
                              font-weight: bold; }
QPushButton[primary="true"]:disabled { background: @soft; color: @muted; border-color: @border; }

QTreeWidget, QListWidget, QTextBrowser { border: 1px solid @border; border-radius: 8px;
                                         background: @surface; }
QTreeView::item, QListView::item { padding: 4px 2px; }
QTreeView::item:selected, QListView::item:selected { background: @accent; color: @accentText; }

QLineEdit { border: 1px solid @border; border-radius: 6px; padding: 5px 8px;
            background: @surface; }
QLineEdit:focus { border-color: @accent; }

QTabWidget::pane { border: none; }
QTabBar::tab { padding: 6px 16px; border: none; border-bottom: 2px solid transparent;
               background: transparent; color: @muted; }
QTabBar::tab:selected { color: @text; border-bottom-color: @accent; }
QTabBar::tab:hover:!selected { color: @text; }
QTabBar::tab:disabled { color: @border; }

QProgressBar { border: none; border-radius: 4px; background: @soft; max-height: 8px; }
QProgressBar::chunk { border-radius: 4px; background: @success; }

QLabel#chip { border-radius: 9px; padding: 2px 9px; background: @soft; color: @muted; }
QLabel#chip[kind="success"] { color: @success; }
QLabel#chip[kind="failure"] { color: @failure; }
QLabel#chip[kind="warning"] { color: @warning; }
QLabel#banner { background: @notice; border-left: 3px solid @warning; border-radius: 6px;
                padding: 8px 12px; }
QLabel#muted { color: @muted; }
QLabel#sectionTitle { font-weight: bold; }

QSplitter::handle { background: transparent; }
QStatusBar { color: @muted; }
)");
    const std::pair<const char *, QColor> vars[] = {
        { "@accentText", c.accentText }, { "@accent", c.accent }, { "@border", c.border },
        { "@button", c.button }, { "@soft", c.soft }, { "@muted", c.muted },
        { "@surface", c.surface }, { "@success", c.success }, { "@failure", c.failure },
        { "@warning", c.warning }, { "@notice", c.notice }, { "@text", c.text },
    };
    // « @accentText » est remplacé avant « @accent » (sinon il serait coupé).
    for (const auto &[name, color] : vars)
        qss.replace(QLatin1String(name), color.name());
    return qss;
}

} // namespace

void apply(Mode mode)
{
    auto *app = qobject_cast<QApplication *>(QCoreApplication::instance());
    if (!app)
        return;
    if (!g_systemPaletteSaved) {   // avant de remplacer la palette du système
        g_systemPalette = QApplication::palette();
        g_systemPaletteSaved = true;
    }
    g_mode = mode;
    g_dark = mode == Mode::Dark || (mode == Mode::Auto && systemPrefersDark());

    // Fusion : style fourni par Qt, identique sur tous les bureaux, et qui
    // respecte entièrement la palette (les styles natifs l'ignorent parfois).
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QApplication::setPalette(makePalette(current()));
    app->setStyleSheet(makeStyleSheet(current()));
}

Mode mode() { return g_mode; }
bool isDark() { return g_dark; }

QColor color(Role role)
{
    const Colors &c = current();
    switch (role) {
    case Role::Accent:           return c.accent;
    case Role::Success:          return c.success;
    case Role::Failure:          return c.failure;
    case Role::Warning:          return c.warning;
    case Role::Muted:            return c.muted;
    case Role::Border:           return c.border;
    case Role::Surface:          return c.surface;
    case Role::SoftBackground:   return c.soft;
    case Role::CodeBackground:   return c.code;
    case Role::OutputBackground: return c.output;
    case Role::NoticeBackground: return c.notice;
    }
    return c.text;
}

void repolish(QWidget *widget)
{
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
}

Mode savedMode()
{
    const QString v = QSettings().value(QStringLiteral("ui/theme")).toString();
    if (v == QLatin1String("light"))
        return Mode::Light;
    if (v == QLatin1String("dark"))
        return Mode::Dark;
    return Mode::Auto;
}

void saveMode(Mode mode)
{
    const char *v = mode == Mode::Light ? "light" : mode == Mode::Dark ? "dark" : "auto";
    QSettings().setValue(QStringLiteral("ui/theme"), QString::fromLatin1(v));
}

} // namespace theme
