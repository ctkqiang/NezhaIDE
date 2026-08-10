#include "sidebar_container.h"
#include "explorer_panel.h"
#include "git_panel.h"
#include <QVBoxLayout>

namespace NezhaIDE::Views {

static const char *kTabActiveStyle =
    "QPushButton { background: rgba(51,112,255,0.12); color: #3370FF;"
    "border: none; border-bottom: 2px solid #3370FF;"
    "padding: 6px 16px; font-size: 12px; font-weight: bold; border-radius: 0; }";

static const char *kTabInactiveStyle =
    "QPushButton { background: transparent; color: #646A73;"
    "border: none; border-bottom: 2px solid transparent;"
    "padding: 6px 16px; font-size: 12px; border-radius: 0; }"
    "QPushButton:hover { background: rgba(0,0,0,0.04); color: #1F2329; }";

SidebarContainer::SidebarContainer(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    setStyleSheet("QWidget { background: #F5F6F7; }");

    setupHeader();
    layout->addWidget(header_);

    stack_ = new QStackedWidget(this);
    stack_->setStyleSheet("QStackedWidget { background: #F5F6F7; }");

    explorer_panel_ = new ExplorerPanel(this);
    git_panel_ = new GitPanel(this);

    stack_->addWidget(explorer_panel_);
    stack_->addWidget(git_panel_);

    layout->addWidget(stack_, 1);

    switchToTab(SidebarTab::Explorer);
}

SidebarContainer::~SidebarContainer() = default;

void SidebarContainer::setupHeader()
{
    header_ = new QWidget(this);
    header_->setFixedHeight(36);
    header_->setStyleSheet("QWidget { background: #FFFFFF; border-bottom: 1px solid #E0E0E0; }");

    auto *header_layout = new QHBoxLayout(header_);
    header_layout->setContentsMargins(8, 0, 8, 0);
    header_layout->setSpacing(0);

    explorer_tab_ = new QPushButton(QStringLiteral("资源管理器"), header_);
    git_tab_ = new QPushButton(QStringLiteral("Git"), header_);
    git_tab_->setFixedWidth(60);

    header_layout->addWidget(explorer_tab_);
    header_layout->addWidget(git_tab_);
    header_layout->addStretch();

    connect(explorer_tab_, &QPushButton::clicked, this, &SidebarContainer::switchToExplorer);
    connect(git_tab_, &QPushButton::clicked, this, &SidebarContainer::switchToGit);
}

ExplorerPanel *SidebarContainer::explorer() const { return explorer_panel_; }

GitPanel *SidebarContainer::gitPanel() const { return git_panel_; }

void SidebarContainer::switchToExplorer()
{
    switchToTab(SidebarTab::Explorer);
}

void SidebarContainer::switchToGit()
{
    switchToTab(SidebarTab::Git);
}

void SidebarContainer::switchToTab(SidebarTab tab)
{
    current_tab_ = tab;
    stack_->setCurrentIndex(static_cast<int>(tab));
    updateTabStyles();
    emit tabChanged(tab);
}

void SidebarContainer::updateTabStyles()
{
    const auto is_explorer = (current_tab_ == SidebarTab::Explorer);
    explorer_tab_->setStyleSheet(is_explorer ? kTabActiveStyle : kTabInactiveStyle);
    git_tab_->setStyleSheet(is_explorer ? kTabInactiveStyle : kTabActiveStyle);
}

} // namespace NezhaIDE::Views
