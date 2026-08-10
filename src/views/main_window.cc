#include "main_window.h"
#include "sidebar_container.h"
#include "explorer_panel.h"
#include "git_panel.h"
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
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupLayout()
{
    splitter_ = new QSplitter(Qt::Horizontal, this);
    splitter_->setHandleWidth(1);
    splitter_->setStyleSheet(
        "QSplitter::handle { background: #E0E0E0; }"
        "QSplitter::handle:hover { background: #3370FF; }"
    );

    sidebar_ = new SidebarContainer(this);
    splitter_->addWidget(sidebar_);

    editor_host_ = new QTabWidget(this);
    editor_host_->setTabsClosable(true);
    editor_host_->setMovable(true);
    editor_host_->setStyleSheet(
        "QTabWidget::pane { border: none; background: #FFFFFF; }"
        "QTabBar::tab { padding: 8px 16px; border: none; border-right: 1px solid #E0E0E0;"
        "min-width: 120px; font-size: 12px; color: #646A73; }"
        "QTabBar::tab:selected { color: #1F2329; font-weight: bold; background: #FFFFFF; }"
        "QTabBar::tab:hover { background: rgba(0,0,0,0.02); }"
    );
    editor_host_->addTab(new QLabel(tr("打开文件以开始编辑")), tr("欢迎"));
    splitter_->addWidget(editor_host_);

    splitter_->setStretchFactor(0, 0);
    splitter_->setStretchFactor(1, 1);
    splitter_->setSizes({320, 960});

    ui->centralwidget->layout()->addWidget(splitter_);

    connect(sidebar_->explorer(), &ExplorerPanel::fileOpened, this, [this](const QString &path) {
        editor_host_->setTabText(0, path.section('/', -1));
    });
}

} // namespace NezhaIDE::Views
