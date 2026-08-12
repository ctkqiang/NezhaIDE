#pragma once

#include <QWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>

namespace NezhaIDE::Views {

class TerminalPanel;

class BottomPanel : public QWidget {
    Q_OBJECT

public:
    explicit BottomPanel(QWidget *parent = nullptr);
    ~BottomPanel() override;

    TerminalPanel *terminalPanel() const;

    void showPanel();
    void hidePanel();
    void togglePanel();

    void switchToTab(int index);

signals:
    void panelVisibilityChanged(bool visible);

private:
    void setupUI();
    void applyStyles();
    QPushButton *makeTabButton(const QString &text, int index);

    QWidget *header_{};
    QWidget *tab_bar_{};
    QStackedWidget *stack_{};
    TerminalPanel *terminal_panel_{};
    QWidget *output_panel_{};
    QWidget *problems_panel_{};

    QList<QPushButton *> tab_buttons_;
    int active_tab_{0};
    bool panel_visible_{true};
};

} // namespace NezhaIDE::Views
