#include "main_window.h"
#include "preferences_dialog.h"
#include "sidebar_container.h"
#include "explorer_panel.h"
#include "git_panel.h"
#include "src/configuration.h"
#include "src/editor/editor_tab_host.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
#include "ui_main_window.h"
#include <QAction>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>

namespace NezhaIDE::Views {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupMenuBar();
    setupLayout();

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

    sidebar_ = new SidebarContainer(this);
    splitter_->addWidget(sidebar_);

    editor_host_ = new NezhaIDE::Editor::EditorTabHost(this);
    splitter_->addWidget(editor_host_);

    splitter_->setStretchFactor(0, 0);
    splitter_->setStretchFactor(1, 1);
    splitter_->setSizes({320, 960});

    ui->centralwidget->layout()->addWidget(splitter_);

    applyStyles();

    connect(sidebar_->explorer(), &ExplorerPanel::fileOpened,
            editor_host_, &NezhaIDE::Editor::EditorTabHost::openFile);
}

void MainWindow::applyStyles()
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    setStyleSheet(ts.qss(QStringLiteral("style.main_window")));
    splitter_->setStyleSheet(ts.qss(QStringLiteral("style.splitter")));
}

} // namespace NezhaIDE::Views
