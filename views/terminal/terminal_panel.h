#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QToolBar>
#include <QLabel>
#include <QList>

namespace NezhaIDE::Views {

class TerminalView;

/**
 * 多 tab 终端面板，管理最多 100 个独立 TerminalView 实例。
 *
 * 每个 tab 运行独立的 PTY shell 进程。工具栏提供新建、关闭终端功能。
 */
class TerminalPanel : public QWidget {
    Q_OBJECT

public:
    explicit TerminalPanel(QWidget *parent = nullptr);
    ~TerminalPanel() override;

    /**
     * 创建新终端 tab 并启动 shell。
     */
    void newTerminal();

    /**
     * 关闭当前活动终端 tab。
     */
    void closeCurrentTerminal();

    /**
     * 对所有终端应用主题。
     */
    void applyTheme();

private:
    void setupUI();
    TerminalView *currentTerminal() const;

    QToolBar *toolbar_{};
    QTabWidget *tab_widget_{};
    int terminal_count_{0};
};

} // namespace NezhaIDE::Views
