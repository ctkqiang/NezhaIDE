#include "main_window.h"
#include "activity_bar.h"
#include "preferences_dialog.h"
#include "sidebar_container.h"
#include "explorer_panel.h"
#include "views/git_panel/git_panel.h"
#include "views/terminal/terminal_panel.h"
#include "views/bottom_panel/bottom_panel.h"
#include "src/configuration.h"
#include "src/utilities/logger.h"
#include "views/editor/editor_tab_host.h"
#include "views/editor/code_editor.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
#include "ui_main_window.h"
#include <QAction>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QStatusBar>

namespace NezhaIDE::Views {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupMenuBar();
    setupLayout();
    setupStatusBar();

    const auto projectPath = NezhaIDE::Configuration::instance().project_root();
    updateProjectRoot(projectPath);

    connect(&NezhaIDE::Services::ThemeService::instance(), &NezhaIDE::Services::ThemeService::themeChanged,
            this, [this] { applyStyles(); });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupMenuBar()
{
    auto *file_menu = menuBar()->addMenu(LOC("menu.file"));

    open_project_action_ = file_menu->addAction(LOC("menu.open_project"));
    open_project_action_->setShortcut(QKeySequence::Open);
    open_project_action_->setMenuRole(QAction::NoRole);
    connect(open_project_action_, &QAction::triggered, this, &MainWindow::onOpenProject);

    save_action_ = file_menu->addAction(LOC("menu.save"));
    save_action_->setShortcut(QKeySequence::Save);
    save_action_->setMenuRole(QAction::NoRole);
    save_action_->setEnabled(false);
    connect(save_action_, &QAction::triggered, this, &MainWindow::onSaveFile);

    file_menu->addSeparator();

    auto *prefs_action = file_menu->addAction(LOC("menu.preferences"));
    prefs_action->setShortcut(QKeySequence::Preferences);
    prefs_action->setMenuRole(QAction::PreferencesRole);
    connect(prefs_action, &QAction::triggered, this, [this] {
        PreferencesDialog dlg(this);
        dlg.exec();
    });

    file_menu->addSeparator();

    quit_action_ = file_menu->addAction(LOC("menu.quit"));
    quit_action_->setShortcut(QKeySequence::Quit);
    quit_action_->setMenuRole(QAction::QuitRole);
    connect(quit_action_, &QAction::triggered, this, &QMainWindow::close);

    auto *edit_menu = menuBar()->addMenu(LOC("menu.edit"));
    undo_action_ = edit_menu->addAction(LOC("menu.edit_undo"));
    undo_action_->setShortcut(QKeySequence::Undo);
    undo_action_->setEnabled(false);
    connect(undo_action_, &QAction::triggered, this, [this] {
        if (auto *ed = editor_host_->currentEditor()) ed->undo();
    });
    redo_action_ = edit_menu->addAction(LOC("menu.edit_redo"));
    redo_action_->setShortcut(QKeySequence::Redo);
    redo_action_->setEnabled(false);
    connect(redo_action_, &QAction::triggered, this, [this] {
        if (auto *ed = editor_host_->currentEditor()) ed->redo();
    });
    edit_menu->addSeparator();
    cut_action_ = edit_menu->addAction(LOC("menu.edit_cut"));
    cut_action_->setShortcut(QKeySequence::Cut);
    cut_action_->setEnabled(false);
    connect(cut_action_, &QAction::triggered, this, [this] {
        if (auto *ed = editor_host_->currentEditor()) ed->cut();
    });
    copy_action_ = edit_menu->addAction(LOC("menu.edit_copy"));
    copy_action_->setShortcut(QKeySequence::Copy);
    copy_action_->setEnabled(false);
    connect(copy_action_, &QAction::triggered, this, [this] {
        if (auto *ed = editor_host_->currentEditor()) ed->copy();
    });
    paste_action_ = edit_menu->addAction(LOC("menu.edit_paste"));
    paste_action_->setShortcut(QKeySequence::Paste);
    paste_action_->setEnabled(false);
    connect(paste_action_, &QAction::triggered, this, [this] {
        if (auto *ed = editor_host_->currentEditor()) ed->paste();
    });

    auto *view_menu = menuBar()->addMenu(LOC("menu.view"));
    toggle_explorer_ = view_menu->addAction(LOC("menu.view_explorer"));
    toggle_explorer_->setCheckable(true);
    toggle_explorer_->setChecked(true);
    connect(toggle_explorer_, &QAction::triggered, this, [this](bool checked) {
        if (!checked) return;
        toggle_git_->setChecked(false);
        sidebar_->showExplorer();
    });
    toggle_git_ = view_menu->addAction(LOC("menu.view_git"));
    toggle_git_->setCheckable(true);
    toggle_git_->setChecked(false);
    connect(toggle_git_, &QAction::triggered, this, [this](bool checked) {
        if (!checked) return;
        toggle_explorer_->setChecked(false);
        sidebar_->showGit();
    });
    toggle_http_ = view_menu->addAction(LOC("menu.view_http"));
    toggle_http_->setCheckable(true);
    toggle_http_->setChecked(true);
    connect(toggle_http_, &QAction::triggered, this, [this] {
        editor_host_->openHttpClient();
    });

    toggle_hydra_ = view_menu->addAction(LOC("menu.view_hydra"));
    toggle_hydra_->setCheckable(true);
    toggle_hydra_->setChecked(true);
    connect(toggle_hydra_, &QAction::triggered, this, [this] {
        editor_host_->openHydra();
    });

    toggle_terminal_ = view_menu->addAction(LOC("menu.view_terminal"));
    toggle_terminal_->setCheckable(true);
    toggle_terminal_->setChecked(true);
    toggle_terminal_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_QuoteLeft));
    connect(toggle_terminal_, &QAction::triggered, this, [this](bool checked) {
        bottom_panel_->setVisible(checked);
    });
}

void MainWindow::setupStatusBar()
{
    auto *sb = statusBar();
    sb->setSizeGripEnabled(false);

    auto *branch_label = new QLabel(this);
    branch_label->setObjectName(QStringLiteral("statusBranch"));
    branch_label->setCursor(Qt::PointingHandCursor);
    connect(sidebar_->gitPanel(), &GitPanel::branchChanged, this,
            [branch_label](const QString &branch) {
        branch_label->setText(QStringLiteral("  \\u23C7 ") + branch + QStringLiteral("  "));
    });
    sb->addWidget(branch_label);

    sb->addPermanentWidget(new QLabel(QStringLiteral(""), this));

    cur_pos_label_ = new QLabel(QStringLiteral("  %1  ").arg(LOC("status.position").arg(1).arg(1)), this);
    cur_pos_label_->setObjectName(QStringLiteral("statusCursor"));
    cur_pos_label_->setCursor(Qt::PointingHandCursor);
    sb->addPermanentWidget(cur_pos_label_);

    lang_label_ = new QLabel(QStringLiteral("  %1  ").arg(LOC("status.lang.plain")), this);
    lang_label_->setObjectName(QStringLiteral("statusLang"));
    lang_label_->setCursor(Qt::PointingHandCursor);
    sb->addPermanentWidget(lang_label_);

    encoding_label_ = new QLabel(QStringLiteral("  UTF-8  "), this);
    encoding_label_->setObjectName(QStringLiteral("statusEncoding"));
    encoding_label_->setCursor(Qt::PointingHandCursor);
    sb->addPermanentWidget(encoding_label_);

    auto *spacer = new QLabel(QStringLiteral("  "), this);
    sb->addPermanentWidget(spacer);
}

void MainWindow::onOpenProject()
{
    const auto dir = QFileDialog::getExistingDirectory(
        this,
        LOC("dialog.open_project_title"),
        QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (dir.isEmpty()) return;

    auto &cfg = NezhaIDE::Configuration::instance();
    cfg.set_project_root(dir);
    cfg.save();

    updateProjectRoot(dir);
}

void MainWindow::onSaveFile()
{
    if (auto *ed = editor_host_->currentEditor()) {
        if (ed->save()) {
            updateEditActions();
        }
    }
}

void MainWindow::updateEditActions()
{
    auto *ed = editor_host_->currentEditor();
    const bool has = ed != nullptr;

    save_action_->setEnabled(has);
    undo_action_->setEnabled(has && ed->document()->isUndoAvailable());
    redo_action_->setEnabled(has && ed->document()->isRedoAvailable());
    cut_action_->setEnabled(has);
    copy_action_->setEnabled(has && !ed->textCursor().selectedText().isEmpty());
    paste_action_->setEnabled(has);
}

void MainWindow::updateProjectRoot(const QString &projectPath)
{
    const QDir dir(projectPath);
    if (!dir.exists()) {
        NezhaIDE::Utilities::Logger::instance().log(
            NezhaIDE::Utilities::LogLevel::Warn, __FILE__, __LINE__, __func__,
            "项目目录不存在: {}", projectPath.toStdString());
        QMessageBox::warning(this, LOC("error.title"),
                             LOC("error.project_not_found").arg(projectPath));
        return;
    }

    NezhaIDE::Utilities::Logger::instance().log(
        NezhaIDE::Utilities::LogLevel::Info, __FILE__, __LINE__, __func__,
        "打开项目: {}", projectPath.toStdString());

    QDir::setCurrent(projectPath);

    sidebar_->explorer()->setRootPath(projectPath);
    sidebar_->gitPanel()->setWorkingDirectory(projectPath);

    const auto title = QStringLiteral("%1 — %2")
        .arg(dir.dirName())
        .arg(QString::fromUtf8(NezhaIDE::Constants::ApplicationName.data(),
                               static_cast<int>(NezhaIDE::Constants::ApplicationName.size())));
    setWindowTitle(title);
}

void MainWindow::setupLayout()
{
    splitter_ = new QSplitter(Qt::Horizontal, this);
    splitter_->setHandleWidth(4);

    activity_bar_ = new ActivityBar(this);
    splitter_->addWidget(activity_bar_);

    sidebar_ = new SidebarContainer(this);
    splitter_->addWidget(sidebar_);

    editor_host_ = new NezhaIDE::Editor::EditorTabHost(this);
    splitter_->addWidget(editor_host_);

    splitter_->setStretchFactor(0, 0);
    splitter_->setStretchFactor(1, 0);
    splitter_->setStretchFactor(2, 1);
    splitter_->setSizes({48, 280, 952});

    ui->centralwidget->layout()->addWidget(splitter_);

    bottom_panel_ = new BottomPanel(this);
    ui->centralwidget->layout()->addWidget(bottom_panel_);
    terminal_panel_ = bottom_panel_->terminalPanel();

    applyStyles();

    // Connect BottomPanel visibility toggles
    connect(bottom_panel_, &BottomPanel::panelVisibilityChanged, this, [this](bool visible) {
        toggle_terminal_->setChecked(visible);
    });

    connect(activity_bar_, &ActivityBar::itemSelected, this, [this](ActivityBarItem item) {
        switch (item) {
        case ActivityBarItem::Explorer:
            toggle_explorer_->setChecked(true);
            toggle_git_->setChecked(false);
            sidebar_->showExplorer();
            break;
        case ActivityBarItem::Git:
            toggle_git_->setChecked(true);
            toggle_explorer_->setChecked(false);
            sidebar_->showGit();
            break;
        case ActivityBarItem::HttpClient:
            editor_host_->openHttpClient();
            break;
        case ActivityBarItem::Hydra:
            editor_host_->openHydra();
            break;
        case ActivityBarItem::Terminal:
            bottom_panel_->togglePanel();
            break;
        default: break;
        }
    });

    connect(sidebar_->explorer(), &ExplorerPanel::fileOpened,
            editor_host_, &NezhaIDE::Editor::EditorTabHost::openFile);
    connect(sidebar_->gitPanel(), &GitPanel::fileOpened,
            editor_host_, &NezhaIDE::Editor::EditorTabHost::openFile);

    connect(editor_host_, &QTabWidget::currentChanged,
            this, &MainWindow::updateEditActions);
    connect(editor_host_, &NezhaIDE::Editor::EditorTabHost::editActionsChanged,
            this, &MainWindow::updateEditActions);

    // 连接编辑器光标变化以更新状态栏（连接去重，避免切 tab 累积）
    connect(editor_host_, &QTabWidget::currentChanged, this, [this](int) {
        auto *ed = editor_host_->currentEditor();
        updateStatusFromEditor(ed);
        if (!ed) return;

        if (cursor_conns_.contains(ed)) {
            disconnect(cursor_conns_.take(ed));
        }
        cursor_conns_[ed] = connect(
            ed, &QPlainTextEdit::cursorPositionChanged, this,
            [this, ed] { updateStatusFromEditor(ed); });
    });
}

void MainWindow::updateStatusFromEditor(NezhaIDE::Editor::CodeEditor *editor)
{
    if (!editor) {
        cur_pos_label_->setText(QStringLiteral("  %1  ")
            .arg(LOC("status.position").arg(1).arg(1)));
        lang_label_->setText(QStringLiteral("  %1  ").arg(LOC("status.lang.plain")));
        return;
    }

    const auto cursor = editor->textCursor();
    cur_pos_label_->setText(QStringLiteral("  %1  ")
        .arg(LOC("status.position")
                 .arg(cursor.blockNumber() + 1)
                 .arg(cursor.columnNumber() + 1)));

    // 根据文件后缀更新语言模式
    const auto suffix = QFileInfo(editor->filePath()).suffix().toLower();
    QString lang = suffix.isEmpty() ? LOC("status.lang.plain") : suffix.toUpper();
    // 已知语言映射
    static const QHash<QString, QString> langMap = {
        {QStringLiteral("cpp"), QStringLiteral("C++")},
        {QStringLiteral("h"), QStringLiteral("C++")},
        {QStringLiteral("cc"), QStringLiteral("C++")},
        {QStringLiteral("c"), QStringLiteral("C")},
        {QStringLiteral("py"), QStringLiteral("Python")},
        {QStringLiteral("js"), QStringLiteral("JavaScript")},
        {QStringLiteral("ts"), QStringLiteral("TypeScript")},
        {QStringLiteral("json"), QStringLiteral("JSON")},
        {QStringLiteral("xml"), QStringLiteral("XML")},
        {QStringLiteral("html"), QStringLiteral("HTML")},
        {QStringLiteral("css"), QStringLiteral("CSS")},
        {QStringLiteral("sql"), QStringLiteral("SQL")},
        {QStringLiteral("md"), QStringLiteral("Markdown")},
        {QStringLiteral("yaml"), QStringLiteral("YAML")},
        {QStringLiteral("yml"), QStringLiteral("YAML")},
        {QStringLiteral("sh"), QStringLiteral("Shell")},
        {QStringLiteral("cmake"), QStringLiteral("CMake")},
        {QStringLiteral("txt"), QStringLiteral("Plain Text")},
    };
    if (langMap.contains(suffix)) lang = langMap[suffix];
    lang_label_->setText(QStringLiteral("  %1  ").arg(lang));
}

void MainWindow::applyStyles()
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    setStyleSheet(ts.qss(QStringLiteral("style.main_window")));
    menuBar()->setStyleSheet(ts.qss(QStringLiteral("style.menubar")));
    statusBar()->setStyleSheet(ts.qss(QStringLiteral("style.statusbar")));
    splitter_->setStyleSheet(ts.qss(QStringLiteral("style.splitter")));
    bottom_panel_->setStyleSheet(ts.qss(QStringLiteral("style.panel_container")));
    activity_bar_->applyStyles();
}

} // namespace NezhaIDE::Views
