#include "bottom_panel.h"
#include "views/terminal/terminal_panel.h"
#include "src/services/design_tokens.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QStyle>

namespace NezhaIDE::Views {

BottomPanel::BottomPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("bottomPanelRoot"));
    setupUI();
    applyStyles();

    connect(&NezhaIDE::Services::ThemeService::instance(),
            &NezhaIDE::Services::ThemeService::themeChanged,
            this, [this] { applyStyles(); });
}

BottomPanel::~BottomPanel() = default;

void BottomPanel::setupUI()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    header_ = new QWidget(this);
    header_->setFixedHeight(NezhaIDE::Services::Tokens::kBottomPanelHeaderH);
    header_->setObjectName(QStringLiteral("bottomPanelHeader"));

    auto *header_layout = new QHBoxLayout(header_);
    header_layout->setContentsMargins(8, 0, 4, 0);
    header_layout->setSpacing(0);

    tab_bar_ = new QWidget(header_);
    tab_bar_->setObjectName(QStringLiteral("bottomPanelTabBar"));
    auto *tab_layout = new QHBoxLayout(tab_bar_);
    tab_layout->setContentsMargins(0, 0, 0, 0);
    tab_layout->setSpacing(0);

    auto *terminal_btn = makeTabButton(LOC("bottom.tab.terminal"), 0);
    auto *output_btn = makeTabButton(LOC("bottom.tab.output"), 1);
    auto *problems_btn = makeTabButton(LOC("bottom.tab.problems"), 2);

    header_layout->addWidget(tab_bar_);
    header_layout->addStretch();

    auto *max_btn = new QPushButton(QStringLiteral("\u25A1"), header_);
    max_btn->setObjectName(QStringLiteral("panelToolBtn"));
    max_btn->setFixedSize(NezhaIDE::Services::Tokens::kToolBtnSize,
                          NezhaIDE::Services::Tokens::kToolBtnSize);
    max_btn->setCursor(Qt::PointingHandCursor);
    max_btn->setToolTip(LOC("bottom.panel.maximize"));
    connect(max_btn, &QPushButton::clicked, this, &BottomPanel::togglePanel);
    header_layout->addWidget(max_btn);

    auto *close_btn = new QPushButton(QStringLiteral("\u2715"), header_);
    close_btn->setObjectName(QStringLiteral("panelToolBtn"));
    close_btn->setFixedSize(NezhaIDE::Services::Tokens::kToolBtnSize,
                            NezhaIDE::Services::Tokens::kToolBtnSize);
    close_btn->setCursor(Qt::PointingHandCursor);
    close_btn->setToolTip(LOC("bottom.panel.close"));
    connect(close_btn, &QPushButton::clicked, this, &BottomPanel::hidePanel);
    header_layout->addWidget(close_btn);

    layout->addWidget(header_);

    stack_ = new QStackedWidget(this);
    stack_->setObjectName(QStringLiteral("bottomPanelStack"));
    stack_->setMinimumHeight(140);

    terminal_panel_ = new TerminalPanel(this);
    stack_->addWidget(terminal_panel_);

    output_panel_ = new QWidget(this);
    auto *output_layout = new QVBoxLayout(output_panel_);
    output_layout->setContentsMargins(8, 8, 8, 8);
    auto *output_text = new QPlainTextEdit(output_panel_);
    output_text->setReadOnly(true);
    output_text->setPlaceholderText(LOC("bottom.output.placeholder"));
    output_text->setObjectName(QStringLiteral("outputLog"));
    output_layout->addWidget(output_text);
    stack_->addWidget(output_panel_);

    problems_panel_ = new QWidget(this);
    auto *problems_layout = new QVBoxLayout(problems_panel_);
    problems_layout->setContentsMargins(8, 8, 8, 8);
    auto *problems_label = new QLabel(LOC("bottom.problems.empty"), problems_panel_);
    problems_label->setAlignment(Qt::AlignCenter);
    problems_label->setObjectName(QStringLiteral("problemsPlaceholder"));
    problems_layout->addWidget(problems_label);
    stack_->addWidget(problems_panel_);

    layout->addWidget(stack_, 1);

    switchToTab(0);
}

QPushButton *BottomPanel::makeTabButton(const QString &text, int index)
{
    auto *btn = new QPushButton(text, tab_bar_);
    btn->setObjectName(QStringLiteral("panelTabBtn"));
    btn->setCheckable(true);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFixedHeight(28);
    btn->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    connect(btn, &QPushButton::clicked, this, [this, index] {
        switchToTab(index);
    });
    tab_buttons_.append(btn);
    return btn;
}

void BottomPanel::switchToTab(int index)
{
    if (index < 0 || index >= stack_->count()) return;
    active_tab_ = index;
    stack_->setCurrentIndex(index);
    for (int i = 0; i < tab_buttons_.size(); ++i) {
        tab_buttons_[i]->setChecked(i == index);
    }
}

void BottomPanel::showPanel()
{
    if (panel_visible_) return;
    panel_visible_ = true;
    setVisible(true);
    emit panelVisibilityChanged(true);
}

void BottomPanel::hidePanel()
{
    if (!panel_visible_) return;
    panel_visible_ = false;
    setVisible(false);
    emit panelVisibilityChanged(false);
}

void BottomPanel::togglePanel()
{
    if (panel_visible_) hidePanel();
    else showPanel();
}

TerminalPanel *BottomPanel::terminalPanel() const
{
    return terminal_panel_;
}

void BottomPanel::applyStyles()
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    setStyleSheet(ts.qss(QStringLiteral("style.panel_container")));
    stack_->findChild<QPlainTextEdit *>(QStringLiteral("outputLog"))->setStyleSheet(
        ts.qss(QStringLiteral("style.output_log")));
}

} // namespace NezhaIDE::Views
