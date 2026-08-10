#include "editor_tab_host.h"
#include "code_editor.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
#include <QLabel>
#include <QFileInfo>
#include <QTabBar>

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

void EditorTabHost::onTabCloseRequested(int index)
{
    auto *w = widget(index);
    if (!w) return;

    if (auto *editor = qobject_cast<CodeEditor *>(w)) {
        editors_.remove(editor->filePath());
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
    const auto idx = addTab(new QLabel(LOC("editor.open_to_edit")), welcomeTitle);
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
