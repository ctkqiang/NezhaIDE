#include "editor_tab_host.h"
#include "code_editor.h"
#include "views/http_client_panel.h"
#include "src/configuration.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
#include <QLabel>
#include <QFileInfo>
#include <QTabBar>
#include <QVBoxLayout>

namespace NezhaIDE::Editor {

EditorTabHost::EditorTabHost(QWidget *parent)
    : QTabWidget(parent)
{
    setTabsClosable(true);
    setMovable(true);
    setDocumentMode(false);
    setUsesScrollButtons(true);
    tabBar()->setDrawBase(false);
    tabBar()->setExpanding(false);
    tabBar()->setElideMode(Qt::ElideLeft);
    tabBar()->setObjectName(QStringLiteral("editorTabBar"));

    ensureWelcomeTab();

    connect(this, &QTabWidget::tabCloseRequested,
            this, &EditorTabHost::onTabCloseRequested);

    connect(&NezhaIDE::Services::ThemeService::instance(),
            &NezhaIDE::Services::ThemeService::themeChanged,
            this, &EditorTabHost::applyStyles);

    applyStyles();
}

void EditorTabHost::openFile(const QString &path)
{
    const auto canonical = QFileInfo(path).canonicalFilePath();
    if (canonical.isEmpty()) return;

    if (auto it = editors_.find(canonical); it != editors_.end()) {
        setCurrentWidget(it.value());
        return;
    }

    auto *editor = new CodeEditor(canonical, this);
    if (!editor->load()) {
        delete editor;
        return;
    }

    editors_[canonical] = editor;

    const auto title = QFileInfo(canonical).fileName();
    const auto index = addTab(editor, title);

    connect(editor, &CodeEditor::titleChanged, this, [this](const QString &title) {
        auto *ed = qobject_cast<CodeEditor *>(sender());
        if (ed) {
            const int idx = indexOf(ed);
            if (idx >= 0) setTabText(idx, title);
        }
    });

    if (count() == 2) {
        const auto welcomeIdx = indexOf(findChild<QLabel *>());
        if (welcomeIdx >= 0) {
            QWidget *w = widget(welcomeIdx);
            removeTab(welcomeIdx);
            delete w;
        }
    }

    setCurrentIndex(index);
}

void EditorTabHost::openHttpClient()
{
    if (http_panel_) {
        setCurrentWidget(http_panel_);
        return;
    }

    http_panel_ = new NezhaIDE::Views::HttpClientPanel(this);
    const auto idx = addTab(http_panel_, QStringLiteral("HTTP Client"));

    if (count() == 2) {
        const auto welcomeIdx = indexOf(findChild<QLabel *>());
        if (welcomeIdx >= 0) {
            QWidget *w = widget(welcomeIdx);
            removeTab(welcomeIdx);
            delete w;
        }
    }

    setCurrentIndex(idx);
}

void EditorTabHost::onTabCloseRequested(int index)
{
    auto *w = widget(index);
    if (!w) return;

    if (auto *editor = qobject_cast<CodeEditor *>(w)) {
        editors_.remove(editor->filePath());
    }

    if (w == http_panel_) {
        http_panel_ = nullptr;
    }

    removeTab(index);
    w->deleteLater();

    if (count() == 0) {
        ensureWelcomeTab();
    }
}

void EditorTabHost::ensureWelcomeTab()
{
    const auto welcomeTitle = LOC("editor.welcome");
    for (int i = 0; i < count(); ++i) {
        if (tabText(i) == welcomeTitle) return;
    }

    auto *welcome = new QWidget();
    auto *wl = new QVBoxLayout(welcome);
    wl->setAlignment(Qt::AlignCenter);
    wl->setSpacing(8);

    auto *title = new QLabel(QString::fromUtf8(
        NezhaIDE::Constants::ApplicationName.data(),
        static_cast<int>(NezhaIDE::Constants::ApplicationName.size())));
    title->setStyleSheet(QStringLiteral("QLabel { font-size: 24px; font-weight: bold;"
        "color: $c; }").replace(QStringLiteral("$c"),
        NezhaIDE::Services::ThemeService::instance().color(QStringLiteral("text.primary"))));
    title->setAlignment(Qt::AlignCenter);
    wl->addWidget(title);

    auto *ver = new QLabel(QStringLiteral("v") + QString::fromUtf8(
        NezhaIDE::Constants::ApplicationVersion.data(),
        static_cast<int>(NezhaIDE::Constants::ApplicationVersion.size())));
    ver->setAlignment(Qt::AlignCenter);
    ver->setStyleSheet(QStringLiteral("QLabel { font-size: 13px; color: $c; }")
        .replace(QStringLiteral("$c"),
        NezhaIDE::Services::ThemeService::instance().color(QStringLiteral("text.tertiary"))));
    wl->addWidget(ver);

    wl->addSpacing(16);

    auto *hint = new QLabel(LOC("editor.open_to_edit"));
    hint->setAlignment(Qt::AlignCenter);
    hint->setStyleSheet(QStringLiteral("QLabel { font-size: 13px; color: $c; }")
        .replace(QStringLiteral("$c"),
        NezhaIDE::Services::ThemeService::instance().color(QStringLiteral("text.secondary"))));
    wl->addWidget(hint);

    const auto idx = addTab(welcome, welcomeTitle);
    tabBar()->setTabButton(idx, QTabBar::RightSide, nullptr);
}

void EditorTabHost::applyStyles()
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    setStyleSheet(ts.qss(QStringLiteral("style.tab_widget")));
    for (auto *editor : editors_) {
        editor->applyTheme();
    }
}

} // namespace NezhaIDE::Editor
