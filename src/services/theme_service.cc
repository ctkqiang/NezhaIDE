#include "theme_service.h"
#include "src/configuration.h"
#include "src/utilities/logger.h"
#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QPalette>
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
     "font-family: 'SF Mono', Menlo, 'Courier New', monospace; font-size: 13px;"
     "padding: 8px; }"
     "QWidget#lineNumberArea { background: $bg.secondary; color: $text.tertiary;"
     "font-family: 'SF Mono', Menlo, 'Courier New', monospace; font-size: 12px; }"},

    {"style.http_panel",
     "QWidget#httpClientRoot { background: $bg.secondary; }"},

    {"style.http_url_input",
     "QLineEdit { border: 1px solid $border; border-radius: 4px; padding: 4px 8px;"
     "background: $bg.tertiary; color: $text.primary; font-size: 12px; }"
     "QLineEdit:focus { border-color: $accent; }"},

    {"style.http_input",
     "QPlainTextEdit { border: none; background: $bg.tertiary; color: $text.primary;"
     "font-family: 'SF Mono', Menlo, monospace; font-size: 12px; }"
     "QComboBox { border: 1px solid $border; border-radius: 4px; padding: 4px 8px;"
     "background: $bg.tertiary; color: $text.primary; font-size: 12px; }"
     "QComboBox::drop-down { border: none; }"
     "QComboBox QAbstractItemView { background: $bg.tertiary; color: $text.primary;"
     "selection-background-color: $overlay.selection; }"
     "QTableWidget { border: 1px solid $border; gridline-color: $border;"
     "background: $bg.tertiary; color: $text.primary; font-size: 12px; }"
     "QHeaderView::section { background: $bg.secondary; color: $text.primary;"
     "border: none; padding: 4px 8px; font-size: 11px; font-weight: bold; }"},

    {"style.http_response",
     "QPlainTextEdit { border: none; background: $bg.tertiary; color: $text.primary;"
     "font-family: 'SF Mono', Menlo, monospace; font-size: 12px; }"},

    {"style.http_request_list",
     "QListWidget { border: none; background: transparent; font-size: 11px; }"
     "QListWidget::item { padding: 3px 6px; color: $text.secondary; }"
     "QListWidget::item:hover { background: $overlay.hover; }"
     "QListWidget::item:selected { background: $overlay.selection; color: $text.primary; }"},

    {"style.http_tab",
     "QTabWidget::pane { border: 1px solid $border; background: $bg.tertiary; }"
     "QTabBar::tab { background: transparent; color: $text.tertiary; padding: 4px 12px;"
     "border: none; border-bottom: 2px solid transparent; font-size: 11px; }"
     "QTabBar::tab:selected { color: $accent; border-bottom: 2px solid $accent; }"
     "QTabBar::tab:hover { color: $text.primary; }"},

    {"style.http_status_label",
     "QLabel { font-size: 13px; font-weight: bold; color: $accent; padding: 2px 0; }"},
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
}

void ThemeService::applyTheme(IDETheme theme)
{
    const auto resolved = resolveTheme(theme);
    if (resolved == current_theme_) return;

    current_theme_ = resolved;
    loadXml(xmlPath(resolved));
    rebuildStyles();

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

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Utilities::Logger::instance().log(
            Utilities::LogLevel::Warn,
            __FILE__, __LINE__, __func__,
            "无法打开主题文件: {}，使用回退调色板",
            path.toStdString()
        );
        for (auto it = kFallbackColors.cbegin(); it != kFallbackColors.cend(); ++it) {
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
        for (auto it = kFallbackColors.cbegin(); it != kFallbackColors.cend(); ++it) {
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

} // namespace NezhaIDE::Services
