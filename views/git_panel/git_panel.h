#pragma once

#include <QWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QProcess>
#include <QToolBar>

namespace NezhaIDE::Views {

struct GitFileEntry {
    QString path;
    QString fullPath;
    QChar status_x; // index status
    QChar status_y; // working tree status
};

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
    void onUnstageFile();
    void onCommit();
    void onStatusFinished(int exit_code, QProcess::ExitStatus status);
    void onBranchFinished(int exit_code, QProcess::ExitStatus status);
    void onListItemClicked(QListWidgetItem *item);
    void onListItemDoubleClicked(QListWidgetItem *item);

private:
    void parseStatusOutput(const QString &output);
    void updateBranchDisplay();
    void applyStyles();
    void applyGitColors();
    QString statusCharToText(QChar x, QChar y) const;

    QListWidget *file_list_{};
    QTextEdit *commit_message_{};
    QPushButton *commit_button_{};
    QLabel *branch_label_{};
    QToolBar *toolbar_{};
    QLabel *status_label_{};
    QProcess *git_process_{};
    QList<GitFileEntry> entries_;
};

} // namespace NezhaIDE::Views
