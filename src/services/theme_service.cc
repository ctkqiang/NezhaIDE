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
    {"syntax.editor.foreground", "#1F2329"},
    {"syntax.keyword",     "#A626A4"},
    {"syntax.type",        "#0B7285"},
    {"syntax.function",    "#3574F0"},
    {"syntax.string",      "#50A14F"},
    {"syntax.comment",     "#A0A1A7"},
    {"syntax.number",      "#986801"},
    {"syntax.operator",    "#1F2329"},
    {"syntax.preprocessor","#7A3E9D"},
};

static const QHash<QString, QString> kStyleTemplates = {
    {"style.toolbar",
     "QToolBar { border: none; border-bottom: 1px solid $border; padding: 4px 8px; spacing: 4px; }"},

    {"style.tree_view",
     "QTreeView { border: none; background: $bg.secondary; }"
     "QTreeView::item { padding: 4px 8px; border-radius: 4px; }"
     "QTreeView::item:hover { background: $overlay.hover; }"
     "QTreeView::item:selected { background: $overlay.selection; color: $accent; }"
     "QTreeView::branch:has-siblings:!adjoins-item { border-image: none; }"
     "QTreeView::branch:has-siblings:adjoins-item { border-image: none; }"
     "QTreeView::branch:!has-children:!has-siblings:adjoins-item { border-image: none; }"},

    {"style.list_widget",
     "QListWidget { border: none; background: $bg.secondary; }"
     "QListWidget::item { padding: 4px 12px; border-radius: 2px; }"
     "QListWidget::item:hover { background: $overlay.hover; }"
     "QListWidget::item:selected { background: $overlay.selection; color: $accent; }"},

    {"style.splitter",
     "QSplitter::handle { background: $border; }"
     "QSplitter::handle:hover { background: $accent; }"},

    {"style.tab_widget",
     "QTabWidget::pane { border: none; background: $bg.primary; }"
     "QTabBar::tab { padding: 8px 16px; border: none; border-right: 1px solid $border;"
     "min-width: 120px; font-size: 12px; color: $text.secondary; }"
     "QTabBar::tab:selected { color: $text.primary; font-weight: bold; background: $bg.primary; }"
     "QTabBar::tab:hover { background: $overlay.hover.subtle; }"},

    {"style.panel",
     "QWidget { background: $bg.secondary; }"},

    {"style.stack",
     "QStackedWidget { background: $bg.secondary; }"},

    {"style.header",
     "QWidget { background: $bg.primary; border-bottom: 1px solid $border; }"},

    {"style.main_window",
     "QMainWindow { background: $bg.primary; }"},

    {"style.explorer_root",
     "QWidget#ExplorerPanel { background: $bg.secondary; }"},

    {"style.tab_active",
     "QPushButton { background: $overlay.selection; color: $accent;"
     "border: none; border-bottom: 2px solid $accent;"
     "padding: 6px 16px; font-size: 12px; font-weight: bold; border-radius: 0; }"},

    {"style.tab_inactive",
     "QPushButton { background: transparent; color: $text.secondary;"
     "border: none; border-bottom: 2px solid transparent;"
     "padding: 6px 16px; font-size: 12px; border-radius: 0; }"
     "QPushButton:hover { background: $overlay.hover; color: $text.primary; }"},

    {"style.menu",
     "QMenu { border: 1px solid $border; border-radius: 6px; padding: 4px; }"
     "QMenu::item { padding: 6px 24px; border-radius: 4px; }"
     "QMenu::item:hover { background: $overlay.hover; }"},

    {"style.branch_label",
     "QLabel { padding: 8px 12px; font-size: 12px; color: $text.secondary;"
     "border-bottom: 1px solid $border; background: $bg.primary; }"},

    {"style.commit_frame",
     "QWidget { border-top: 1px solid $border; background: $bg.primary; }"},

    {"style.commit_input",
     "QTextEdit { border: 1px solid $border; border-radius: 6px; padding: 6px;"
     "font-size: 12px; background: $bg.tertiary; }"
     "QTextEdit:focus { border-color: $accent; background: $bg.primary; }"},

    {"style.primary_button",
     "QPushButton { background: $accent; color: $button.text; border: none; border-radius: 6px;"
     "padding: 6px 16px; font-size: 12px; font-weight: bold; }"
     "QPushButton:hover { background: $accent.hover; }"
     "QPushButton:pressed { background: $accent.pressed; }"},

    {"style.status_label",
     "QLabel { padding: 4px 12px; font-size: 11px; color: $text.tertiary; background: $bg.secondary; }"},

    {"style.editor",
     "QPlainTextEdit { background: $syntax.editor.background; color: $syntax.editor.foreground;"
     "border: none; selection-background-color: $overlay.selection;"
     "font-family: 'JetBrains Mono', 'SF Mono', Menlo, monospace; font-size: 13px; }"
     "QWidget#lineNumberArea { background: $bg.secondary; color: $text.tertiary; }"},
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
