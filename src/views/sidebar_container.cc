#include "sidebar_container.h"
#include "explorer_panel.h"
#include "git_panel.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
#include <QVBoxLayout>

namespace NezhaIDE::Views {

SidebarContainer::SidebarContainer(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    setupHeader();
    layout->addWidget(header_);

    stack_ = new QStackedWidget(this);

    explorer_panel_ = new ExplorerPanel(this);
    git_panel_ = new GitPanel(this);

    stack_->addWidget(explorer_panel_);
    stack_->addWidget(git_panel_);

    layout->addWidget(stack_, 1);

    applyStyles();

    connect(&NezhaIDE::Services::ThemeService::instance(), &NezhaIDE::Services::ThemeService::themeChanged,
            this, [this] { applyStyles(); });

    switchToTab(SidebarTab::Explorer);
}

SidebarContainer::~SidebarContainer() = default;

void SidebarContainer::setupHeader()
{
    header_ = new QWidget(this);
    header_->setFixedHeight(56);
    header_->setObjectName(QStringLiteral("sidebarHeader"));

    auto *outer = new QVBoxLayout(header_);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto *title_bar = new QHBoxLayout();
    title_bar->setContentsMargins(16, 8, 8, 0);
    title_bar->setSpacing(0);

    section_title_ = new QLabel(LOC("sidebar.explorer").toUpper(), header_);
    section_title_->setObjectName(QStringLiteral("sidebarSectionTitle"));
    section_title_->setStyleSheet(
        QStringLiteral("QLabel { font-size: 11px; font-weight: bold; color: ") +
        NezhaIDE::Services::ThemeService::instance().color(QStringLiteral("text.secondary")) +
        QStringLiteral("; text-transform: uppercase; letter-spacing: 1px; }"));
    title_bar->addWidget(section_title_);
    title_bar->addStretch();
    outer->addLayout(title_bar);

    auto *tab_bar = new QHBoxLayout();
    tab_bar->setContentsMargins(12, 4, 8, 0);
    tab_bar->setSpacing(0);

    explorer_tab_ = new QPushButton(LOC("sidebar.explorer").toUpper(), header_);
    git_tab_ = new QPushButton(LOC("sidebar.git").toUpper(), header_);

    tab_bar->addWidget(explorer_tab_);
    tab_bar->addWidget(git_tab_);
    tab_bar->addStretch();
    outer->addLayout(tab_bar);

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
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    const auto is_explorer = (current_tab_ == SidebarTab::Explorer);
    explorer_tab_->setStyleSheet(ts.qss(is_explorer ? QStringLiteral("style.tab_active") : QStringLiteral("style.tab_inactive")));
    git_tab_->setStyleSheet(ts.qss(is_explorer ? QStringLiteral("style.tab_inactive") : QStringLiteral("style.tab_active")));
    section_title_->setText(is_explorer ? LOC("sidebar.explorer").toUpper() : LOC("sidebar.git").toUpper());
}

void SidebarContainer::applyStyles()
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    setStyleSheet(ts.qss(QStringLiteral("style.panel")));
    stack_->setStyleSheet(ts.qss(QStringLiteral("style.stack")));
    header_->setStyleSheet(ts.qss(QStringLiteral("style.header")));
    updateTabStyles();
}

} // namespace NezhaIDE::Views
