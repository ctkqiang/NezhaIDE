#include "sidebar_container.h"
#include "explorer_panel.h"
#include "git_panel.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
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
    header_->setFixedHeight(32);
    header_->setObjectName(QStringLiteral("sidebarHeader"));

    auto *layout = new QHBoxLayout(header_);
    layout->setContentsMargins(12, 0, 4, 0);
    layout->setSpacing(4);

    project_name_ = new QLabel(QStringLiteral("PROJECT"), header_);
    project_name_->setStyleSheet(
        QStringLiteral("QLabel { font-size: 11px; font-weight: bold;"
        "text-transform: uppercase; letter-spacing: 0.5px; background: transparent; }"));
    layout->addWidget(project_name_);
    layout->addStretch();

    auto makeHeaderBtn = [](const QString &text, const QString &tooltip, QWidget *parent) -> QPushButton * {
        auto *btn = new QPushButton(text, parent);
        btn->setFixedSize(20, 20);
        btn->setFlat(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setToolTip(tooltip);
        btn->setStyleSheet(
            QStringLiteral("QPushButton { border: none; font-size: 13px;"
            "background: transparent; border-radius: 4px; }"
            "QPushButton:hover { background: $hover; }")
            .replace(QStringLiteral("$hover"),
                NezhaIDE::Services::ThemeService::instance().color(QStringLiteral("overlay.hover"))));
        return btn;
    };

    auto *btn_new = makeHeaderBtn(QStringLiteral("+"), LOC("explorer.new_file"), header_);
    connect(btn_new, &QPushButton::clicked, explorer_panel_, &ExplorerPanel::onNewFile);
    layout->addWidget(btn_new);

    auto *btn_folder = makeHeaderBtn(QStringLiteral("▸▾"), LOC("explorer.new_folder"), header_);
    connect(btn_folder, &QPushButton::clicked, explorer_panel_, &ExplorerPanel::onNewFolder);
    layout->addWidget(btn_folder);

    auto *btn_refresh = makeHeaderBtn(QStringLiteral("⟳"), LOC("explorer.refresh"), header_);
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
    project_name_->setStyleSheet(
        project_name_->styleSheet() +
        QStringLiteral(" color: %1;").arg(
            NezhaIDE::Services::ThemeService::instance().color(QStringLiteral("text.secondary"))));
}

void SidebarContainer::applyStyles()
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    setStyleSheet(ts.qss(QStringLiteral("style.panel")));
    stack_->setStyleSheet(ts.qss(QStringLiteral("style.stack")));
    header_->setStyleSheet(ts.qss(QStringLiteral("style.header")));
}

} // namespace NezhaIDE::Views
