#pragma once

#include <QWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QProcess>
#include <QToolBar>
#include <QPlainTextEdit>
#include <QSplitter>

namespace NezhaIDE::Views {

/**
 * Git 文件状态条目。
 */
struct GitFileEntry {
    QString path;
    QString fullPath;
    QChar status_x;
    QChar status_y;
};

/**
 * Git 版本控制面板，通过进程调用 git 命令。
 *
 * 显示工作区文件状态（modified/added/deleted/untracked），
 * 支持 stage/unstage/discard/commit 操作，内嵌 diff 预览。
 * 使用异步 QProcess 避免阻塞 UI。
 */
class GitPanel : public QWidget {
    Q_OBJECT

public:
    explicit GitPanel(QWidget *parent = nullptr);
    ~GitPanel() override;

    void refresh();
    void setWorkingDirectory(const QString &path);

signals:
    void fileStaged(const QString &path);
    void fileUnstaged(const QString &path);
    void commitRequested(const QString &message);
    void branchChanged(const QString &branch);
    void fileOpened(const QString &path);

private slots:
    void onRefresh();
    void onStageFile();
    void onStageAll();
    void onUnstageFile();
    void onUnstageAll();
    void onDiscardFile();
    void onShowDiff();
    void onOpenFile();
    void onCommit();
    void onStatusFinished(int exit_code, QProcess::ExitStatus status);
    void onBranchFinished(int exit_code, QProcess::ExitStatus status);
    void onListItemClicked(QListWidgetItem *item);
    void onListItemDoubleClicked(QListWidgetItem *item);
    void onCustomContextMenu(const QPoint &pos);

private:
    void parseStatusOutput(const QString &output);
    void updateBranchDisplay();
    void showDiffForFile(const QString &path);
    void runGitCommand(const QStringList &args, std::function<void(int, const QString &)> callback);
    void applyStyles();
    void applyGitColors();
    QString statusCharToText(QChar x, QChar y) const;
    QColor statusColor(QChar x, QChar y) const;

    QString unquoteGitPath(const QString &raw) const;

    QSplitter *splitter_{};
    QListWidget *file_list_{};
    QTextEdit *commit_message_{};
    QPushButton *commit_button_{};
    QLabel *branch_label_{};
    QToolBar *toolbar_{};
    QLabel *status_label_{};
    QProcess *git_process_{};
    QPlainTextEdit *diff_view_{};
    QList<GitFileEntry> entries_;
    bool has_working_dir_{false};
};

} // namespace NezhaIDE::Views
