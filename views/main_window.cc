#include "main_window.h"
#include "activity_bar.h"
#include "preferences_dialog.h"
#include "sidebar_container.h"
#include "explorer_panel.h"
#include "git_panel.h"
#include "src/configuration.h"
#include "views/editor/editor_tab_host.h"
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
    auto *undo_action = edit_menu->addAction(LOC("menu.edit_undo"));
    undo_action->setShortcut(QKeySequence::Undo);
    auto *redo_action = edit_menu->addAction(LOC("menu.edit_redo"));
    redo_action->setShortcut(QKeySequence::Redo);
    edit_menu->addSeparator();
    auto *cut_action = edit_menu->addAction(LOC("menu.edit_cut"));
    cut_action->setShortcut(QKeySequence::Cut);
    auto *copy_action = edit_menu->addAction(LOC("menu.edit_copy"));
    copy_action->setShortcut(QKeySequence::Copy);
    auto *paste_action = edit_menu->addAction(LOC("menu.edit_paste"));
    paste_action->setShortcut(QKeySequence::Paste);

    auto *view_menu = menuBar()->addMenu(LOC("menu.view"));
    auto *toggle_explorer = view_menu->addAction(LOC("menu.view_explorer"));
    toggle_explorer->setCheckable(true);
    toggle_explorer->setChecked(true);
    auto *toggle_git = view_menu->addAction(LOC("menu.view_git"));
    toggle_git->setCheckable(true);
    toggle_git->setChecked(true);
    auto *toggle_http = view_menu->addAction(LOC("menu.view_http"));
    toggle_http->setCheckable(true);
    toggle_http->setChecked(true);
}

void MainWindow::setupStatusBar()
{
    auto *sb = statusBar();
    sb->setSizeGripEnabled(false);

    auto *branch_label = new QLabel(this);
    branch_label->setObjectName(QStringLiteral("statusBranch"));
    connect(sidebar_->gitPanel(), &GitPanel::branchChanged, this,
            [branch_label](const QString &branch) {
        branch_label->setText(QStringLiteral("  branch: %1  ").arg(branch));
    });

    auto *info_label = new QLabel(this);
    info_label->setObjectName(QStringLiteral("statusInfo"));
    info_label->setText(QStringLiteral("  Ready  "));

    sb->addWidget(branch_label);
    sb->addPermanentWidget(info_label);
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

void MainWindow::updateProjectRoot(const QString &projectPath)
{
    const QDir dir(projectPath);
    if (!dir.exists()) {
        QMessageBox::warning(this, LOC("error.title"),
                             LOC("error.project_not_found").arg(projectPath));
        return;
    }

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
    splitter_->setHandleWidth(1);

    activity_bar_ = new ActivityBar(this);
    splitter_->addWidget(activity_bar_);

    sidebar_ = new SidebarContainer(this);
    splitter_->addWidget(sidebar_);

    editor_host_ = new NezhaIDE::Editor::EditorTabHost(this);
    splitter_->addWidget(editor_host_);

    splitter_->setStretchFactor(0, 0);
    splitter_->setStretchFactor(1, 0);
    splitter_->setStretchFactor(2, 1);
    splitter_->setSizes({36, 280, 964});

    ui->centralwidget->layout()->addWidget(splitter_);

    applyStyles();

    connect(activity_bar_, &ActivityBar::itemSelected, this, [this](ActivityBarItem item) {
        switch (item) {
        case ActivityBarItem::Explorer: sidebar_->showExplorer(); break;
        case ActivityBarItem::Git: sidebar_->showGit(); break;
        case ActivityBarItem::HttpClient: editor_host_->openHttpClient(); break;
        default: break;
        }
    });

    connect(sidebar_->explorer(), &ExplorerPanel::fileOpened,
            editor_host_, &NezhaIDE::Editor::EditorTabHost::openFile);
}

void MainWindow::applyStyles()
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    setStyleSheet(ts.qss(QStringLiteral("style.main_window")));
    menuBar()->setStyleSheet(ts.qss(QStringLiteral("style.menubar")));
    statusBar()->setStyleSheet(ts.qss(QStringLiteral("style.statusbar")));
    splitter_->setStyleSheet(ts.qss(QStringLiteral("style.splitter")));
    activity_bar_->applyStyles();
}

} // namespace NezhaIDE::Views
