#include "sidebar_container.h"
#include "explorer_panel.h"
#include "views/git_panel/git_panel.h"
#include "src/services/design_tokens.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
#include "src/utilities/logger.h"
#include <QDir>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

namespace NezhaIDE::Views {

SidebarContainer::SidebarContainer(QWidget *parent)
    : QWidget(parent)
{
    explorer_panel_ = new ExplorerPanel(this);
    git_panel_ = new GitPanel(this);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    setupHeader();
    layout->addWidget(header_);

    stack_ = new QStackedWidget(this);
    stack_->addWidget(explorer_panel_);
    stack_->addWidget(git_panel_);
    layout->addWidget(stack_, 1);

    applyStyles();

    connect(&NezhaIDE::Services::ThemeService::instance(), &NezhaIDE::Services::ThemeService::themeChanged,
            this, [this] { applyStyles(); });

    showExplorer();
}

SidebarContainer::~SidebarContainer() = default;

void SidebarContainer::setupHeader()
{
    header_ = new QWidget(this);
    header_->setFixedHeight(NezhaIDE::Services::Tokens::kSidebarHeaderH);
    header_->setObjectName(QStringLiteral("sidebarHeader"));

    auto *layout = new QHBoxLayout(header_);
    layout->setContentsMargins(12, 0, 4, 0);
    layout->setSpacing(2);

    project_name_ = new QLabel(LOC("sidebar.explorer"), header_);
    project_name_->setObjectName(QStringLiteral("sidebarTitle"));
    layout->addWidget(project_name_);
    layout->addStretch();

    auto makeHeaderBtn = [](const QString &iconName, const QString &tooltip, QWidget *parent) -> QPushButton * {
        auto *btn = new QPushButton(parent);
        btn->setObjectName(QStringLiteral("sidebarHeaderBtn"));
        const auto s = NezhaIDE::Services::Tokens::kIconBtnSize;
        btn->setFixedSize(s, s);
        btn->setIconSize({14, 14});
        btn->setIcon(QIcon(QStringLiteral(":/vectors/%1.svg").arg(iconName)));
        btn->setFlat(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setToolTip(tooltip);
        return btn;
    };

    auto *btn_new = makeHeaderBtn(QStringLiteral("plus"), LOC("explorer.new_file"), header_);
    connect(btn_new, &QPushButton::clicked, explorer_panel_, &ExplorerPanel::onNewFile);
    layout->addWidget(btn_new);

    auto *btn_folder = makeHeaderBtn(QStringLiteral("folder_new"), LOC("explorer.new_folder"), header_);
    connect(btn_folder, &QPushButton::clicked, explorer_panel_, &ExplorerPanel::onNewFolder);
    layout->addWidget(btn_folder);

    auto *btn_refresh = makeHeaderBtn(QStringLiteral("refresh"), LOC("explorer.refresh"), header_);
    connect(btn_refresh, &QPushButton::clicked, this, [this] {
        explorer()->setRootPath(QDir::currentPath());
    });
    layout->addWidget(btn_refresh);
}

ExplorerPanel *SidebarContainer::explorer() const { return explorer_panel_; }
GitPanel *SidebarContainer::gitPanel() const { return git_panel_; }

void SidebarContainer::showExplorer()
{
    stack_->setCurrentIndex(0);
    setHeaderTitle(LOC("sidebar.explorer"));
}

void SidebarContainer::showGit()
{
    stack_->setCurrentIndex(1);
    setHeaderTitle(LOC("sidebar.git"));
}

void SidebarContainer::setHeaderTitle(const QString &title)
{
    project_name_->setText(title.toUpper());
}

void SidebarContainer::applyStyles()
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    setStyleSheet(ts.qss(QStringLiteral("style.panel")));
    stack_->setStyleSheet(ts.qss(QStringLiteral("style.stack")));
    header_->setStyleSheet(ts.qss(QStringLiteral("style.header")));
}

} // namespace NezhaIDE::Views
