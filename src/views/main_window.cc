#include "main_window.h"
#include "sidebar_container.h"
#include "explorer_panel.h"
#include "git_panel.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
#include "ui_main_window.h"
#include <QSplitter>
#include <QTabWidget>
#include <QLabel>

namespace NezhaIDE::Views {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupLayout();

    connect(&NezhaIDE::Services::ThemeService::instance(), &NezhaIDE::Services::ThemeService::themeChanged,
            this, [this] { applyStyles(); });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupLayout()
{
    splitter_ = new QSplitter(Qt::Horizontal, this);
    splitter_->setHandleWidth(1);

    sidebar_ = new SidebarContainer(this);
    splitter_->addWidget(sidebar_);

    editor_host_ = new QTabWidget(this);
    editor_host_->setTabsClosable(true);
    editor_host_->setMovable(true);
    editor_host_->addTab(new QLabel(LOC("editor.open_to_edit")), LOC("editor.welcome"));
    splitter_->addWidget(editor_host_);

    splitter_->setStretchFactor(0, 0);
    splitter_->setStretchFactor(1, 1);
    splitter_->setSizes({320, 960});

    ui->centralwidget->layout()->addWidget(splitter_);

    applyStyles();

    connect(sidebar_->explorer(), &ExplorerPanel::fileOpened, this, [this](const QString &path) {
        editor_host_->setTabText(0, path.section('/', -1));
    });
}

void MainWindow::applyStyles()
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    setStyleSheet(ts.qss(QStringLiteral("style.main_window")));
    splitter_->setStyleSheet(ts.qss(QStringLiteral("style.splitter")));
    editor_host_->setStyleSheet(ts.qss(QStringLiteral("style.tab_widget")));
}

} // namespace NezhaIDE::Views
