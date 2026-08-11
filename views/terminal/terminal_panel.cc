#include "terminal_panel.h"
#include "terminal_view.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
#include <QVBoxLayout>

namespace NezhaIDE::Views {

TerminalPanel::TerminalPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    applyTheme();
    newTerminal();

    connect(&Services::ThemeService::instance(), &Services::ThemeService::themeChanged,
            this, [this] { applyTheme(); });
}

TerminalPanel::~TerminalPanel() = default;

void TerminalPanel::setupUI() {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    toolbar_ = new QToolBar(this);
    toolbar_->setIconSize({16, 16});
    toolbar_->setMovable(false);
    toolbar_->addAction(QStringLiteral("+"), LOC("terminal.new"), this, &TerminalPanel::newTerminal);
    toolbar_->addAction(QStringLiteral("×"), LOC("terminal.close"),
                        this, &TerminalPanel::closeCurrentTerminal);
    layout->addWidget(toolbar_);

    tab_widget_ = new QTabWidget(this);
    tab_widget_->setTabsClosable(true);
    tab_widget_->setMovable(true);
    tab_widget_->setDocumentMode(true);
    tab_widget_->setObjectName(QStringLiteral("terminalTabWidget"));

    connect(tab_widget_, &QTabWidget::tabCloseRequested, this, [this](int index) {
        if (tab_widget_->count() <= 1) return;
        auto *w = tab_widget_->widget(index);
        tab_widget_->removeTab(index);
        delete w;
    });

    layout->addWidget(tab_widget_, 1);
}

void TerminalPanel::newTerminal() {
    terminal_count_++;
    auto *term = new TerminalView(tab_widget_);
    auto title = QStringLiteral("%1 %2").arg(term->shellName()).arg(terminal_count_);
    int idx = tab_widget_->addTab(term, title);
    tab_widget_->setCurrentIndex(idx);

    if (!term->startShell()) {
        term->setPlainText(QStringLiteral("Failed to start shell"));
    }

    connect(term, &TerminalView::shellExited, this, [this, term](int) {
        int idx = tab_widget_->indexOf(term);
        if (idx >= 0) {
            if (tab_widget_->count() > 1) {
                tab_widget_->removeTab(idx);
                term->deleteLater();
            } else {
                term->setReadOnly(true);
                term->appendPlainText(QStringLiteral("\n[Process exited]"));
            }
        }
    });
}

void TerminalPanel::closeCurrentTerminal() {
    if (tab_widget_->count() <= 1) return;
    int idx = tab_widget_->currentIndex();
    auto *w = tab_widget_->widget(idx);
    tab_widget_->removeTab(idx);
    delete w;
}

void TerminalPanel::applyTheme() {
    auto &ts = Services::ThemeService::instance();
    toolbar_->setStyleSheet(ts.qss(QStringLiteral("style.toolbar")));
    tab_widget_->setStyleSheet(ts.qss(QStringLiteral("style.http_tabs")));
    setStyleSheet(
        QStringLiteral("QWidget { background: %1; }").arg(ts.color(QStringLiteral("bg.primary"))));

    for (int i = 0; i < tab_widget_->count(); ++i) {
        auto *tv = qobject_cast<TerminalView *>(tab_widget_->widget(i));
        if (tv) tv->applyTheme();
    }
}

} // namespace NezhaIDE::Views
