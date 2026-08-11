#pragma once

#include <QPlainTextEdit>
#include <QSocketNotifier>
#include <QTextCharFormat>
#include <QProcess>
#include <memory>

namespace NezhaIDE::Views {

/**
 * PTY 终端显示组件，基于 QPlainTextEdit + forkpty + ANSI SGR 解析。
 *
 * 通过伪终端连接交互式 shell（bash/zsh），支持 ANSI 颜色、
 * 基本光标控制和键盘输入转发。每个实例拥有独立的 shell 进程。
 */
class TerminalView : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit TerminalView(QWidget *parent = nullptr);
    ~TerminalView() override;

    /**
     * 启动 shell 进程并建立 PTY 连接。
     *
     * @return true 表示 shell 启动成功。
     */
    bool startShell();

    /**
     * 终止 shell 进程并关闭 PTY。
     */
    void killShell();

    /**
     * 向 shell 发送指定文本（用于粘贴等操作）。
     *
     * @param text 要发送的文本。
     */
    void sendText(const QString &text);

    bool isRunning() const noexcept;

signals:
    void shellExited(int exitCode);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onPtyReadyRead();

private:
    void parseAnsiAndAppend(const QByteArray &data);
    void processSgr(const QStringList &params);
    void writeToPty(const QByteArray &data);

    int master_fd_{-1};
    pid_t shell_pid_{-1};
    std::unique_ptr<QSocketNotifier> notifier_;

    QString pending_ansi_;
    QTextCharFormat current_fmt_;
    bool bold_{false};
    bool italic_{false};
    bool underline_{false};
    bool inverse_{false};

    QColor fg_color_{Qt::white};
    QColor bg_color_{Qt::black};

    static constexpr int kDefaultFg = 37;
    static constexpr int kDefaultBg = 40;
    static constexpr int kMaxScrollback = 5000;
};

} // namespace NezhaIDE::Views
