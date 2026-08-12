#include "editor_tab_host.h"
#include "code_editor.h"
#include "data_view.h"
#include "views/hex_editor/hex_editor.h"
#include "views/http_view_panel/http_view_panel.h"
#include "views/hydra/hydra_view_panel.h"
#include "src/configuration.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
#include "src/utilities/logger.h"
#include <QFile>
#include <QJsonDocument>
#include <QLabel>
#include <QFileInfo>
#include <QMessageBox>
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

    // .hydra 后缀作为 Hydra 工具入口，从 Explorer 双击即打开工具面板
    if (QFileInfo(canonical).suffix().compare(QStringLiteral("hydra"), Qt::CaseInsensitive) == 0) {
        openHydra();
        return;
    }

    if (auto it = editors_.find(canonical); it != editors_.end()) {
        setCurrentWidget(it.value());
        return;
    }
    if (auto it = hex_editors_.find(canonical); it != hex_editors_.end()) {
        setCurrentWidget(it.value());
        return;
    }
    if (auto it = data_views_.find(canonical); it != data_views_.end()) {
        setCurrentWidget(it.value());
        return;
    }

    const auto suffix = QFileInfo(canonical).suffix().toLower();
    if (suffix == QStringLiteral("db") || suffix == QStringLiteral("sqlite")
        || suffix == QStringLiteral("sqlite3")) {
        openDataFile(canonical);
        return;
    }
    if (suffix == QStringLiteral("csv")) {
        openDataFile(canonical);
        return;
    }
    if (suffix == QStringLiteral("json")) {
        openJsonFile(canonical);
        return;
    }

    auto binaryCheck = Model::BinaryParser::open(canonical);
    if (binaryCheck.has_value()) {
        openBinaryFile(canonical);
        return;
    }

    openCodeFile(canonical);
}

void EditorTabHost::openCodeFile(const QString &path)
{
    const auto canonical = QFileInfo(path).canonicalFilePath();
    if (canonical.isEmpty()) return;

    NezhaIDE::Utilities::Logger::instance().log(
        NezhaIDE::Utilities::LogLevel::Info, __FILE__, __LINE__, __func__,
        "打开文件: {}", canonical.toStdString());

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

    connect(editor->document(), &QTextDocument::undoAvailable,
            this, &EditorTabHost::editActionsChanged);
    connect(editor->document(), &QTextDocument::redoAvailable,
            this, &EditorTabHost::editActionsChanged);
    connect(editor, &CodeEditor::copyAvailable,
            this, &EditorTabHost::editActionsChanged);
    connect(editor, &CodeEditor::modificationChanged,
            this, &EditorTabHost::editActionsChanged);

    removeWelcomeTab();
    setCurrentIndex(index);
    emit editActionsChanged();
}

void EditorTabHost::openDataFile(const QString &path)
{
    const auto canonical = QFileInfo(path).canonicalFilePath();
    if (canonical.isEmpty()) return;

    auto *dataView = new NezhaIDE::Views::DataView(canonical, this);
    if (!dataView->load()) {
        delete dataView;
        openCodeFile(canonical);
        return;
    }

    data_views_[canonical] = dataView;

    const auto idx = addTab(dataView, QFileInfo(canonical).fileName());

    connect(dataView, &NezhaIDE::Views::DataView::titleChanged, this,
            [this](const QString &t) {
        auto *dv = qobject_cast<NezhaIDE::Views::DataView *>(sender());
        if (dv) {
            const int i = indexOf(dv);
            if (i >= 0) setTabText(i, t);
        }
    });

    removeWelcomeTab();
    setCurrentIndex(idx);
    emit editActionsChanged();
}

void EditorTabHost::openJsonFile(const QString &path)
{
    const auto canonical = QFileInfo(path).canonicalFilePath();
    if (canonical.isEmpty()) return;

    QFile file(canonical);
    if (file.open(QIODevice::ReadOnly)) {
        const auto doc = QJsonDocument::fromJson(file.readAll());
        if (!doc.isNull()) {
            auto *editor = new CodeEditor(canonical, this);
            if (!editor->load()) {
                delete editor;
                return;
            }
            editor->setPlainText(QString::fromUtf8(doc.toJson(QJsonDocument::Indented)));
            editor->document()->setModified(false);

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

            connect(editor->document(), &QTextDocument::undoAvailable,
                    this, &EditorTabHost::editActionsChanged);
            connect(editor->document(), &QTextDocument::redoAvailable,
                    this, &EditorTabHost::editActionsChanged);
            connect(editor, &CodeEditor::copyAvailable,
                    this, &EditorTabHost::editActionsChanged);
            connect(editor, &CodeEditor::modificationChanged,
                    this, &EditorTabHost::editActionsChanged);

            removeWelcomeTab();
            setCurrentIndex(index);
            emit editActionsChanged();
            return;
        }
    }
    openCodeFile(canonical);
}

void EditorTabHost::openBinaryFile(const QString &path)
{
    const auto canonical = QFileInfo(path).canonicalFilePath();
    if (canonical.isEmpty()) return;

    if (auto it = hex_editors_.find(canonical); it != hex_editors_.end()) {
        setCurrentWidget(it.value());
        return;
    }

    auto *hexEditor = new NezhaIDE::Views::HexEditor(canonical, this);
    if (!hexEditor->load()) {
        delete hexEditor;
        openFile(path);
        return;
    }

    hex_editors_[canonical] = hexEditor;

    const auto title = QStringLiteral("Hex: ") + QFileInfo(canonical).fileName();
    const auto idx = addTab(hexEditor, title);

    connect(hexEditor, &NezhaIDE::Views::HexEditor::titleChanged, this,
            [this](const QString &t) {
        auto *he = qobject_cast<NezhaIDE::Views::HexEditor *>(sender());
        if (he) {
            const int i = indexOf(he);
            if (i >= 0) setTabText(i, t);
        }
    });

    removeWelcomeTab();
    setCurrentIndex(idx);
    emit editActionsChanged();
}

void EditorTabHost::openHttpClient()
{
    if (http_panel_) {
        setCurrentWidget(http_panel_);
        return;
    }

    http_panel_ = new NezhaIDE::Views::HttpViewPanel(this);
    const auto idx = addTab(http_panel_, LOC("http.tab_title"));

    removeWelcomeTab();
    setCurrentIndex(idx);
}

void EditorTabHost::openHydra()
{
    if (hydra_panel_) {
        setCurrentWidget(hydra_panel_);
        return;
    }

    hydra_panel_ = new NezhaIDE::Views::HydraViewPanel(this);
    const auto idx = addTab(hydra_panel_, LOC("hydra.tab_title"));

    removeWelcomeTab();
    setCurrentIndex(idx);
}

CodeEditor *EditorTabHost::currentEditor() const
{
    return qobject_cast<CodeEditor *>(currentWidget());
}

void EditorTabHost::removeWelcomeTab()
{
    if (!welcome_tab_) return;

    const auto idx = indexOf(welcome_tab_);
    if (idx >= 0) removeTab(idx);
    delete welcome_tab_;
    welcome_tab_ = nullptr;
}

void EditorTabHost::onTabCloseRequested(int index)
{
    auto *w = widget(index);
    if (!w) return;

    if (auto *editor = qobject_cast<CodeEditor *>(w)) {
        if (editor->isModified()) {
            QMessageBox box(QMessageBox::Question, LOC("editor.save_title"),
                LOC("editor.save_prompt").arg(QFileInfo(editor->filePath()).fileName()),
                QMessageBox::NoButton, this);
            auto *saveBtn = box.addButton(LOC("editor.save"), QMessageBox::AcceptRole);
            box.addButton(LOC("editor.discard"), QMessageBox::DestructiveRole);
            auto *cancelBtn = box.addButton(LOC("editor.cancel"), QMessageBox::RejectRole);
            box.exec();
            if (box.clickedButton() == cancelBtn) return;
            if (box.clickedButton() == saveBtn && !editor->save()) return;
        }
        editors_.remove(editor->filePath());
    }

    if (w == http_panel_) {
        http_panel_ = nullptr;
    }

    if (w == hydra_panel_) {
        hydra_panel_ = nullptr;
    }

    if (auto *hex = qobject_cast<NezhaIDE::Views::HexEditor *>(w)) {
        hex_editors_.remove(hex->filePath());
    }

    if (auto *dv = qobject_cast<NezhaIDE::Views::DataView *>(w)) {
        data_views_.remove(dv->filePath());
    }

    removeTab(index);
    w->deleteLater();

    if (count() == 0) {
        ensureWelcomeTab();
    } else {
        emit editActionsChanged();
    }
}

void EditorTabHost::ensureWelcomeTab()
{
    if (welcome_tab_) return;

    const auto welcomeTitle = LOC("editor.welcome");
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

    welcome_tab_ = welcome;
    const auto idx = addTab(welcome, welcomeTitle);
    tabBar()->setTabButton(idx, QTabBar::RightSide, nullptr);
    emit editActionsChanged();
}

void EditorTabHost::applyStyles()
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    setStyleSheet(ts.qss(QStringLiteral("style.tab_widget")));
    for (auto *editor : editors_) {
        editor->applyTheme();
    }
    for (auto *hex : hex_editors_) {
        hex->applyTheme();
    }
    for (auto *dv : data_views_) {
        dv->applyTheme();
    }
}

} // namespace NezhaIDE::Editor
