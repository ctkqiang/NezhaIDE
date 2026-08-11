#include "theme_service.h"
#include "src/configuration.h"
#include "src/utilities/logger.h"
#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QPalette>
#include <QScrollBar>
#include <QStyle>
#include <QStyleFactory>
#include <QToolTip>
#include <QXmlStreamReader>

namespace NezhaIDE::Services {

static const QHash<QString, QString> kFallbackColors = {
    {"bg.primary",         "#FFFFFF"},
    {"bg.secondary",       "#F5F6F7"},
    {"bg.tertiary",        "#FAFAFA"},
    {"border",             "#E0E0E0"},
    {"accent",             "#3370FF"},
    {"accent.hover",       "#2860DF"},
    {"accent.pressed",     "#1E50C8"},
    {"button.text",        "#FFFFFF"},
    {"text.primary",       "#1F2329"},
    {"text.secondary",     "#646A73"},
    {"text.tertiary",      "#999999"},
    {"overlay.hover",      "rgba(0,0,0,0.04)"},
    {"overlay.hover.subtle","rgba(0,0,0,0.02)"},
    {"overlay.selection",  "rgba(51,112,255,0.12)"},
    {"git.modified",       "#E67E22"},
    {"git.added",          "#27AE60"},
    {"git.deleted",        "#E74C3C"},
    {"git.untracked",      "#999999"},
    {"syntax.editor.background", "#FFFFFF"},
    {"syntax.editor.foreground", "#1A1A1A"},
    {"syntax.keyword",     "#CF1F8B"},
    {"syntax.type",        "#0E7A7A"},
    {"syntax.function",    "#2563EB"},
    {"syntax.string",      "#1A8C3A"},
    {"syntax.comment",     "#6B7280"},
    {"syntax.number",      "#C06014"},
    {"syntax.operator",    "#1A1A1A"},
    {"syntax.preprocessor","#7C3AED"},
};

static const QHash<QString, QString> kFallbackColorsDark = {
    {"bg.primary",         "#0D1117"},
    {"bg.secondary",       "#161B22"},
    {"bg.tertiary",        "#1C2128"},
    {"border",             "#30363D"},
    {"accent",             "#58A6FF"},
    {"accent.hover",       "#79C0FF"},
    {"accent.pressed",     "#388BFD"},
    {"button.text",        "#FFFFFF"},
    {"text.primary",       "#E6EDF3"},
    {"text.secondary",     "#8B949E"},
    {"text.tertiary",      "#6E7681"},
    {"overlay.hover",      "rgba(255,255,255,0.08)"},
    {"overlay.hover.subtle","rgba(255,255,255,0.04)"},
    {"overlay.selection",  "rgba(88,166,255,0.20)"},
    {"git.modified",       "#F0883E"},
    {"git.added",          "#3FB950"},
    {"git.deleted",        "#F85149"},
    {"git.untracked",      "#6E7681"},
    {"syntax.editor.background", "#0D1117"},
    {"syntax.editor.foreground", "#E6EDF3"},
    {"syntax.keyword",     "#FF7B72"},
    {"syntax.type",        "#FFA657"},
    {"syntax.function",    "#D2A8FF"},
    {"syntax.string",      "#A5D6FF"},
    {"syntax.comment",     "#8B949E"},
    {"syntax.number",      "#79C0FF"},
    {"syntax.operator",    "#E6EDF3"},
    {"syntax.preprocessor","#F0883E"},
};

static const QHash<QString, QString> kStyleTemplates = {
    {"style.toolbar",
     "QToolBar { border: none; padding: 4px 6px; spacing: 2px;"
     "background: transparent; }"
     "QToolBar QToolButton { border: none; border-radius: 4px; padding: 4px 8px;"
     "color: $text.secondary; font-size: 12px; }"
     "QToolBar QToolButton:hover { background: $overlay.hover; color: $text.primary; }"
     "QToolBar QToolButton:pressed { background: $overlay.selection; }"
     "QToolBar::separator { width: 1px; margin: 4px 6px; background: transparent; }"},

    {"style.tree_view",
     "QTreeView { border: none; background: $bg.secondary; outline: none;"
     "font-size: 13px; }"
     "QTreeView::item { padding: 4px 8px; min-height: 24px; }"
     "QTreeView::item:hover { background: $overlay.hover; }"
     "QTreeView::item:selected { background: $overlay.selection; color: $text.primary; }"
     "QTreeView::branch { background: transparent; }"
     "QTreeView::branch:has-siblings:!adjoins-item { border-image: none; }"
     "QTreeView::branch:has-siblings:adjoins-item { border-image: none; }"
     "QTreeView::branch:!has-children:!has-siblings:adjoins-item { border-image: none; }"},

    {"style.list_widget",
     "QListWidget { border: none; background: transparent; font-size: 13px; }"
     "QListWidget::item { padding: 4px 12px; min-height: 24px; }"
     "QListWidget::item:hover { background: $overlay.hover; }"
     "QListWidget::item:selected { background: $overlay.selection; color: $text.primary; }"},

    {"style.splitter",
     "QSplitter::handle { background: transparent; width: 1px; }"
     "QSplitter::handle:hover { background: $border; }"},

    {"style.tab_widget",
     "QTabWidget::pane { border: none; background: $bg.primary; }"
     "QTabWidget::tab-bar { alignment: left; left: 0; }"
     "QTabBar { background: $bg.secondary; padding: 2px 4px 0 4px; }"
     "QTabBar::tab { background: transparent; color: $text.secondary;"
     "padding: 5px 12px; margin: 2px 1px 0 1px;"
     "border: none; border-radius: 4px 4px 0 0;"
     "font-size: 12px; height: 24px; }"
     "QTabBar::tab:hover { background: $overlay.hover; color: $text.primary; }"
     "QTabBar::tab:selected { background: $bg.primary; color: $text.primary; }"
     "QTabBar::close-button { subcontrol-position: right;"
     "border-radius: 8px; padding: 0; margin: 0 0 0 4px; }"
     "QTabBar::close-button:hover { background: $overlay.hover; }"},

    {"style.panel",
     "QWidget { background: $bg.secondary; }"},

    {"style.stack",
     "QStackedWidget { background: $bg.secondary; }"},

    {"style.header",
     "QWidget { background: $bg.secondary; }"},

    {"style.main_window",
     "QMainWindow { background: $bg.primary; }"},

    {"style.explorer_root",
     "QWidget#ExplorerPanel { background: $bg.secondary; }"},

    {"style.tab_active",
     "QPushButton { background: transparent; color: $text.primary;"
     "border: none; border-bottom: 2px solid $accent;"
     "padding: 8px 12px; font-size: 11px; font-weight: bold;"
     "text-transform: uppercase; letter-spacing: 0.5px; }"},

    {"style.tab_inactive",
     "QPushButton { background: transparent; color: $text.tertiary;"
     "border: none; border-bottom: 2px solid transparent;"
     "padding: 8px 12px; font-size: 11px;"
     "text-transform: uppercase; letter-spacing: 0.5px; }"
     "QPushButton:hover { color: $text.secondary; }"},

    {"style.menu",
     "QMenu { border: none; border-radius: 8px; padding: 4px;"
     "background: $bg.primary; }"
     "QMenu::item { padding: 6px 32px 6px 12px; border-radius: 4px;"
     "font-size: 13px; }"
     "QMenu::item:selected { background: $accent; color: $button.text; }"
     "QMenu::separator { height: 1px; background: $border; margin: 4px 8px; }"},

    {"style.menubar",
     "QMenuBar { background: $bg.secondary;"
     "padding: 2px 0; font-size: 13px; }"
     "QMenuBar::item { padding: 4px 10px; border-radius: 4px; }"
     "QMenuBar::item:selected { background: $overlay.hover; }"},

    {"style.statusbar",
     "QStatusBar { background: $bg.secondary; color: $text.tertiary;"
     "border: none; font-size: 11px; padding: 0 12px; }"
     "QStatusBar::item { border: none; }"
     "QStatusBar QLabel { color: $text.tertiary; padding: 0 8px; }"},

    {"style.branch_label",
     "QLabel { padding: 6px 12px; font-size: 12px; font-weight: bold;"
     "color: $text.primary; background: transparent; }"},

    {"style.commit_frame",
     "QWidget { background: transparent; padding: 8px; }"},

    {"style.commit_input",
     "QTextEdit { border: 1px solid $border; border-radius: 6px; padding: 8px;"
     "font-size: 13px; background: $bg.tertiary; }"
     "QTextEdit:focus { border-color: $accent; background: $bg.primary; }"},

    {"style.primary_button",
     "QPushButton { background: $accent; color: $button.text; border: none; border-radius: 5px;"
     "padding: 5px 14px; font-size: 12px; font-weight: bold; }"
     "QPushButton:hover { background: $accent.hover; }"
     "QPushButton:pressed { background: $accent.pressed; }"},

    {"style.status_label",
     "QLabel { padding: 4px 12px; font-size: 11px; color: $text.tertiary;"
     "background: transparent; }"},

    {"style.editor",
     "QPlainTextEdit { background: $syntax.editor.background; color: $syntax.editor.foreground;"
     "border: none; selection-background-color: $overlay.selection;"
     "font-family: Menlo, 'Courier New', monospace; font-size: 13px;"
     "padding: 8px; }"
     "QWidget#lineNumberArea { background: $bg.secondary; color: $text.tertiary;"
     "font-family: Menlo, 'Courier New', monospace; font-size: 12px; }"},

    {"style.http_panel",
     "QWidget#httpClientRoot { background: $bg.secondary; }"
     "QFrame#httpRequestBar { background: $bg.primary;"
     "border: 1px solid $border; border-radius: 8px; }"
     "QFrame#httpResponseBar { background: $bg.tertiary;"
     "border: 1px solid $border; border-radius: 6px; }"},

    {"style.http_url_input",
     "QLineEdit { border: 1px solid $border; border-radius: 6px; padding: 0 12px;"
     "background: $bg.primary; color: $text.primary; font-size: 13px; }"
     "QLineEdit:focus { border-color: $accent; }"},

    {"style.http_kv_table",
     "QTableWidget { border: 1px solid $border; border-radius: 6px; background: $bg.primary;"
     "color: $text.primary; font-size: 13px; outline: none; }"
     "QTableWidget::item { padding: 0 8px; border: none; }"
     "QTableWidget::item:selected { background: $overlay.selection; color: $text.primary; }"
     "QTableWidget::item:hover { background: $overlay.hover.subtle; }"
     "QHeaderView::section { background: $bg.secondary; color: $text.secondary;"
     "border: none; border-bottom: 1px solid $border; padding: 5px 8px;"
     "font-size: 11px; font-weight: 600; }"
     "QPushButton#httpRemoveRow { border: none; border-radius: 4px; background: transparent;"
     "color: $text.tertiary; font-size: 11px; }"
     "QPushButton#httpRemoveRow:hover { background: $overlay.hover; color: $git.deleted; }"
     "QPushButton#httpRemoveRow:pressed { background: $overlay.selection; }"},

    {"style.http_body_editor",
     "QPlainTextEdit { border: 1px solid $border; border-radius: 6px; background: $bg.primary;"
     "color: $text.primary; font-family: Menlo, monospace; font-size: 13px;"
     "selection-background-color: $overlay.selection; padding: 8px; }"
     "QPlainTextEdit:focus { border-color: $accent; }"},

    {"style.http_combo",
     "QComboBox { border: 1px solid $border; border-radius: 6px; padding: 0 8px;"
     "background: $bg.primary; color: $text.primary; font-size: 12px; min-width: 130px; }"
     "QComboBox:hover { border-color: $text.tertiary; }"
     "QComboBox:focus { border-color: $accent; }"
     "QComboBox::drop-down { border: none; width: 22px; }"
     "QComboBox QAbstractItemView { background: $bg.primary; color: $text.primary;"
     "selection-background-color: $overlay.selection; border: 1px solid $border;"
     "outline: none; }"},

    {"style.http_tabs",
     "QTabWidget::pane { border: none; background: transparent; }"
     "QTabBar { background: transparent; border-bottom: 1px solid $border; }"
     "QTabBar::tab { background: transparent; color: $text.secondary;"
     "padding: 7px 14px; border: none; border-bottom: 2px solid transparent;"
     "font-size: 13px; font-weight: 500; }"
     "QTabBar::tab:selected { color: $accent; border-bottom: 2px solid $accent; }"
     "QTabBar::tab:hover { color: $text.primary; }"},

    {"style.http_response_body",
     "QPlainTextEdit { border: 1px solid $border; border-radius: 6px; background: $bg.primary;"
     "color: $text.primary; font-family: Menlo, monospace; font-size: 12px;"
     "selection-background-color: $overlay.selection; padding: 8px; }"},

    {"style.http_meta_label",
     "QLabel { font-size: 12px; color: $text.secondary; padding: 2px 0; }"},

    {"style.http_state_title",
     "QLabel { font-size: 16px; font-weight: 600; color: $text.secondary; }"},

    {"style.http_state_detail",
     "QLabel { font-size: 13px; color: $text.tertiary; }"},

    {"style.http_cancel_button",
     "QPushButton { background: transparent; color: $git.deleted;"
     "border: 1px solid $git.deleted; border-radius: 5px; padding: 5px 14px;"
     "font-size: 12px; font-weight: bold; }"
     "QPushButton:hover { background: $overlay.selection; }"
     "QPushButton:pressed { background: $overlay.hover; }"},

    {"style.http_ghost_button",
     "QPushButton { background: transparent; color: $text.secondary;"
     "border: 1px solid $border; border-radius: 5px; padding: 3px 10px;"
     "font-size: 12px; }"
     "QPushButton:hover { border-color: $accent; color: $accent; }"
     "QPushButton:pressed { background: $overlay.selection; }"},

    {"style.http_section_label",
     "QLabel { font-size: 11px; font-weight: 600;"
     "text-transform: uppercase; letter-spacing: 0.5px; color: $text.secondary; }"},
};

ThemeService &ThemeService::instance()
{
    static ThemeService svc;
    return svc;
}

void ThemeService::initialize(IDETheme theme)
{
    const auto resolved = resolveTheme(theme);
    current_theme_ = resolved;
    loadXml(xmlPath(resolved));
    rebuildStyles();
    applyGlobalPalette();
}

void ThemeService::applyTheme(IDETheme theme)
{
    const auto resolved = resolveTheme(theme);
    if (resolved == current_theme_) return;

    current_theme_ = resolved;
    loadXml(xmlPath(resolved));
    rebuildStyles();
    applyGlobalPalette();

    NezhaIDE::Configuration::instance().set_theme(resolved);
    NezhaIDE::Configuration::instance().save();

    emit themeChanged(resolved);
}

QString ThemeService::color(const QString &key) const
{
    return colors_.value(key);
}

QColor ThemeService::qcolor(const QString &key) const
{
    const auto val = colors_.value(key);
    if (val.isEmpty()) return {};
    return QColor(val);
}

QString ThemeService::qss(const QString &key) const
{
    return styles_.value(key);
}

QHash<QString, QColor> ThemeService::syntaxColors() const
{
    QHash<QString, QColor> result;
    for (auto it = colors_.cbegin(); it != colors_.cend(); ++it) {
        if (it.key().startsWith(QStringLiteral("syntax."))) {
            result[it.key()] = QColor(it.value());
        }
    }
    return result;
}

IDETheme ThemeService::currentTheme() const noexcept
{
    return current_theme_;
}

QString ThemeService::themeId(IDETheme theme)
{
    switch (theme) {
    case IDETheme::Light:
    case IDETheme::Auto:
        return QStringLiteral("default");
    case IDETheme::Dark:
        return QStringLiteral("nezha_cyber");
    case IDETheme::GitHub:
        return QStringLiteral("github");
    case IDETheme::Xcode:
        return QStringLiteral("xcode");
    }
    return QStringLiteral("default");
}

QString ThemeService::xmlPath(IDETheme theme)
{
#ifdef NEZHA_PROJECT_ROOT
    const auto dir = QStringLiteral(NEZHA_PROJECT_ROOT) + QStringLiteral("/resources/themes");
#else
    const auto dir = QCoreApplication::applicationDirPath() + QStringLiteral("/resources/themes");
#endif
    return dir + QStringLiteral("/") + themeId(theme) + QStringLiteral(".xml");
}

IDETheme ThemeService::resolveTheme(IDETheme theme)
{
    if (theme != IDETheme::Auto) return theme;

    const auto bg = QApplication::palette().color(QPalette::Window);
    return bg.lightness() < 128 ? IDETheme::Dark : IDETheme::Light;
}

void ThemeService::loadXml(const QString &path)
{
    colors_.clear();

    const auto &fallback = (current_theme_ == IDETheme::Dark) ? kFallbackColorsDark : kFallbackColors;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Utilities::Logger::instance().log(
            Utilities::LogLevel::Warn,
            __FILE__, __LINE__, __func__,
            "无法打开主题文件: {}，使用回退调色板",
            path.toStdString()
        );
        for (auto it = fallback.cbegin(); it != fallback.cend(); ++it) {
            colors_[it.key()] = it.value();
        }
        return;
    }

    QXmlStreamReader xml(&file);
    bool in_syntax = false;
    while (!xml.atEnd() && !xml.hasError()) {
        const auto token = xml.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (xml.name() == QStringLiteral("color")) {
                const auto key = xml.attributes().value("key").toString();
                const auto value = xml.readElementText();
                if (!key.isEmpty()) {
                    colors_[key] = value;
                }
            } else if (xml.name() == QStringLiteral("syntax")) {
                in_syntax = true;
            } else if (in_syntax && xml.name() == QStringLiteral("token")) {
                const auto name = xml.attributes().value("name").toString();
                const auto value = xml.readElementText();
                if (!name.isEmpty()) {
                    colors_[QStringLiteral("syntax.") + name] = value;
                }
            }
        } else if (token == QXmlStreamReader::EndElement && xml.name() == QStringLiteral("syntax")) {
            in_syntax = false;
        }
    }

    if (xml.hasError()) {
        Utilities::Logger::instance().log(
            Utilities::LogLevel::Warn,
            __FILE__, __LINE__, __func__,
            "主题 XML 解析错误: {}",
            xml.errorString().toStdString()
        );
    }

    if (colors_.isEmpty()) {
        Utilities::Logger::instance().log(
            Utilities::LogLevel::Warn,
            __FILE__, __LINE__, __func__,
            "主题文件为空: {}，使用回退调色板",
            path.toStdString()
        );
        const auto &fb = (current_theme_ == IDETheme::Dark) ? kFallbackColorsDark : kFallbackColors;
        for (auto it = fb.cbegin(); it != fb.cend(); ++it) {
            colors_[it.key()] = it.value();
        }
    }
}

void ThemeService::rebuildStyles()
{
    styles_.clear();

    for (auto it = kStyleTemplates.cbegin(); it != kStyleTemplates.cend(); ++it) {
        QString qss = it.value();
        for (auto ci = colors_.cbegin(); ci != colors_.cend(); ++ci) {
            qss.replace(QStringLiteral("$") + ci.key(), ci.value());
        }
        styles_[it.key()] = qss;
    }
}

void ThemeService::applyGlobalPalette()
{
    auto *app = qApp;
    if (!app) return;

    const auto bgPrimary = qcolor("bg.primary");
    const auto bgSecondary = qcolor("bg.secondary");
    const auto bgTertiary = qcolor("bg.tertiary");
    const auto textPrimary = qcolor("text.primary");
    const auto textSecondary = qcolor("text.secondary");
    const auto textTertiary = qcolor("text.tertiary");
    const auto accent = qcolor("accent");
    const auto border = qcolor("border");
    const auto buttonText = qcolor("button.text");

    if (!bgPrimary.isValid()) return;

    QPalette p;
    p.setColor(QPalette::Window, bgPrimary);
    p.setColor(QPalette::WindowText, textPrimary);
    p.setColor(QPalette::Base, bgPrimary);
    p.setColor(QPalette::AlternateBase, bgSecondary);
    p.setColor(QPalette::ToolTipBase, bgTertiary);
    p.setColor(QPalette::ToolTipText, textPrimary);
    p.setColor(QPalette::Text, textPrimary);
    p.setColor(QPalette::Button, bgSecondary);
    p.setColor(QPalette::ButtonText, textPrimary);
    p.setColor(QPalette::BrightText, accent);
    p.setColor(QPalette::Link, accent);
    p.setColor(QPalette::LinkVisited, accent);
    p.setColor(QPalette::Highlight, accent);
    p.setColor(QPalette::HighlightedText, buttonText.isValid() ? buttonText : QColor("#FFFFFF"));
    p.setColor(QPalette::PlaceholderText, textTertiary);
    p.setColor(QPalette::Mid, border);
    p.setColor(QPalette::Dark, border);
    p.setColor(QPalette::Shadow, QColor(0, 0, 0, 60));

    p.setColor(QPalette::Disabled, QPalette::WindowText, textTertiary);
    p.setColor(QPalette::Disabled, QPalette::Text, textTertiary);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, textTertiary);

    app->setPalette(p);

    const auto menuQss = styles_.value(QStringLiteral("style.menu"));

    QString globalQss;
    if (!menuQss.isEmpty()) globalQss += menuQss + QStringLiteral("\n");

    globalQss += QStringLiteral(
        "QToolTip {"
        "  background: %1; color: %2; border: 1px solid %3;"
        "  border-radius: 4px; padding: 4px 8px; font-size: 12px; }"
    ).arg(bgTertiary.name(), textPrimary.name(), border.name());

    app->setStyleSheet(globalQss);
}

} // namespace NezhaIDE::Services
