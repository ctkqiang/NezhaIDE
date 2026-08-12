#pragma once

#include <QPlainTextEdit>
#include <QSocketNotifier>
#include <QTextCharFormat>
#include <memory>

#ifdef Q_OS_WIN
#include <QProcess>
#else
#include <unistd.h>
#endif

namespace NezhaIDE::Views {

/**
 * PTY 终端显示组件，基于 QPlainTextEdit + forkpty + ANSI SGR 解析。
 *
 * POSIX 平台使用 forkpty 提供真实交互式 shell；Windows 上无 forkpty，
 * 降级为 QProcess 运行 shell（仅支持非交互输出，保证可编译可运行）。
 * 颜色完全通过 ThemeService 获取，支持暗色/浅色主题切换。
 * 每个实例拥有独立的 shell 进程。
 */
class TerminalView : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit TerminalView(QWidget *parent = nullptr);
    ~TerminalView() override;

    bool startShell();
    void killShell();
    void sendText(const QString &text);
    void applyTheme();
    QString shellName() const;
    bool isRunning() const noexcept;

signals:
    void shellExited(int exitCode);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onPtyReadyRead();

private:
    void parseAnsiAndAppend(const QByteArray &data);
    void processSgr(const QStringList &params);
    void writeToPty(const QByteArray &data);
    void updatePtySize();
    void resetFormat();

#ifdef Q_OS_WIN
    QProcess *process_{};
#else
    int master_fd_{-1};
    pid_t shell_pid_{-1};
    std::unique_ptr<QSocketNotifier> notifier_;
#endif
    QString shell_path_;

    QString pending_ansi_;
    QTextCharFormat current_fmt_;
    bool bold_{false};
    bool italic_{false};
    bool underline_{false};
    bool inverse_{false};

    QColor theme_bg_;
    QColor theme_fg_;
    QColor fg_color_;
    QColor bg_color_;

    static constexpr int kMaxScrollback = 5000;
};

} // namespace NezhaIDE::Views
