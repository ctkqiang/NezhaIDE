#include "git_panel.h"
#include "src/services/localization_service.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QDir>
#include <QProcess>
#include <QTimer>

namespace NezhaIDE::Views {

GitPanel::GitPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    toolbar_ = new QToolBar(this);
    toolbar_->setIconSize({16, 16});
    toolbar_->setMovable(false);
    toolbar_->setStyleSheet(
        "QToolBar { border: none; border-bottom: 1px solid #E0E0E0; padding: 4px 8px; spacing: 4px; }"
    );
    toolbar_->addAction(QStringLiteral("↻ ") + LOC("git.refresh"), this, &GitPanel::onRefresh);
    toolbar_->addSeparator();
    toolbar_->addAction(QStringLiteral("＋ ") + LOC("git.stage"), this, &GitPanel::onStageFile);
    toolbar_->addAction(QStringLiteral("－ ") + LOC("git.unstage"), this, &GitPanel::onUnstageFile);
    layout->addWidget(toolbar_);

    branch_label_ = new QLabel(this);
    branch_label_->setStyleSheet(
        "QLabel { padding: 8px 12px; font-size: 12px; color: #646A73;"
        "border-bottom: 1px solid #E0E0E0; background: #FFFFFF; }"
    );
    layout->addWidget(branch_label_);

    file_list_ = new QListWidget(this);
    file_list_->setContextMenuPolicy(Qt::CustomContextMenu);
    file_list_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    file_list_->setStyleSheet(
        "QListWidget { border: none; background: #F5F6F7; }"
        "QListWidget::item { padding: 4px 12px; border-radius: 2px; }"
        "QListWidget::item:hover { background: rgba(0,0,0,0.04); }"
        "QListWidget::item:selected { background: rgba(51,112,255,0.12); color: #3370FF; }"
    );
    connect(file_list_, &QListWidget::itemClicked, this, &GitPanel::onListItemClicked);
    layout->addWidget(file_list_, 1);

    auto *commit_frame = new QWidget(this);
    commit_frame->setStyleSheet("QWidget { border-top: 1px solid #E0E0E0; background: #FFFFFF; }");
    auto *commit_layout = new QVBoxLayout(commit_frame);
    commit_layout->setContentsMargins(8, 8, 8, 8);
    commit_layout->setSpacing(6);

    commit_message_ = new QTextEdit(this);
    commit_message_->setPlaceholderText(LOC("git.commit_placeholder"));
    commit_message_->setMaximumHeight(80);
    commit_message_->setStyleSheet(
        "QTextEdit { border: 1px solid #E0E0E0; border-radius: 6px; padding: 6px;"
        "font-size: 12px; background: #FAFAFA; }"
        "QTextEdit:focus { border-color: #3370FF; background: #FFFFFF; }"
    );
    commit_layout->addWidget(commit_message_);

    commit_button_ = new QPushButton(QStringLiteral("✓ ") + LOC("git.commit_button"), this);
    commit_button_->setStyleSheet(
        "QPushButton { background: #3370FF; color: white; border: none; border-radius: 6px;"
        "padding: 6px 16px; font-size: 12px; font-weight: bold; }"
        "QPushButton:hover { background: #2860DF; }"
        "QPushButton:pressed { background: #1E50C8; }"
    );
    connect(commit_button_, &QPushButton::clicked, this, &GitPanel::onCommit);
    commit_layout->addWidget(commit_button_, 0, Qt::AlignRight);

    layout->addWidget(commit_frame);

    status_label_ = new QLabel(this);
    status_label_->setStyleSheet(
        "QLabel { padding: 4px 12px; font-size: 11px; color: #999; background: #F5F6F7; }"
    );
    layout->addWidget(status_label_);

    git_process_ = new QProcess(this);
    git_process_->setWorkingDirectory(QDir::currentPath());
    connect(git_process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &GitPanel::onStatusFinished);

    updateBranchDisplay();
    QTimer::singleShot(100, this, &GitPanel::refresh);
}

GitPanel::~GitPanel()
{
    if (git_process_->state() != QProcess::NotRunning) {
        git_process_->kill();
        git_process_->waitForFinished(1000);
    }
}

void GitPanel::refresh()
{
    status_label_->setText(LOC("git.status_refreshing"));
    git_process_->terminate();
    git_process_->waitForFinished(500);

    git_process_->start("git", {"status", "--porcelain", "-u"});
}

void GitPanel::onRefresh()
{
    refresh();
}

void GitPanel::onStageFile()
{
    const auto selected = file_list_->selectedItems();
    for (auto *item : selected) {
        const int idx = file_list_->row(item);
        if (idx < entries_.size()) {
            QProcess::execute("git", {"add", entries_[idx].path});
            emit fileStaged(entries_[idx].path);
        }
    }
    refresh();
}

void GitPanel::onUnstageFile()
{
    const auto selected = file_list_->selectedItems();
    for (auto *item : selected) {
        const int idx = file_list_->row(item);
        if (idx < entries_.size() && entries_[idx].status_x != ' ' && entries_[idx].status_x != '?') {
            QProcess::execute("git", {"reset", "HEAD", "--", entries_[idx].path});
            emit fileUnstaged(entries_[idx].path);
        }
    }
    refresh();
}

void GitPanel::onCommit()
{
    const auto msg = commit_message_->toPlainText().trimmed();
    if (msg.isEmpty()) {
        QMessageBox::warning(this, LOC("error.title"),
            LOC("git.error_commit_message"));
        return;
    }

    QProcess proc;
    proc.start("git", {"commit", "-m", msg});
    proc.waitForFinished(5000);

    if (proc.exitCode() != 0) {
        QMessageBox::warning(this, LOC("git.error_commit_failed"),
            QString::fromUtf8(proc.readAllStandardError()));
        return;
    }

    emit commitRequested(msg);
    commit_message_->clear();
    refresh();
}

void GitPanel::onStatusFinished(int exit_code, QProcess::ExitStatus status)
{
    if (status == QProcess::NormalExit && exit_code == 0) {
        const auto output = QString::fromUtf8(git_process_->readAllStandardOutput());
        parseStatusOutput(output);
        status_label_->setText(
            entries_.isEmpty()
                ? QStringLiteral("✓ ") + LOC("git.working_clean")
                : LOC("git.files_changed").arg(entries_.size())
        );
    } else {
        const auto err = QString::fromUtf8(git_process_->readAllStandardError());
        status_label_->setText(LOC("git.error_prefix").arg(err.trimmed()));
    }
}

void GitPanel::onBranchFinished(int exit_code, QProcess::ExitStatus status)
{
    if (status == QProcess::NormalExit && exit_code == 0) {
        const auto branch = QString::fromUtf8(git_process_->readAllStandardOutput()).trimmed();
        branch_label_->setText(QStringLiteral("⎇ %1").arg(branch));
        emit branchChanged(branch);
    }
}

void GitPanel::onListItemClicked(QListWidgetItem *item)
{
    const int idx = file_list_->row(item);
    if (idx < entries_.size()) {
        const auto &entry = entries_[idx];
        status_label_->setText(
            QStringLiteral("%1 — %2")
                .arg(entry.path, statusCharToText(entry.status_x, entry.status_y)));
    }
}

void GitPanel::parseStatusOutput(const QString &output)
{
    entries_.clear();
    file_list_->clear();

    const auto lines = output.split('\n', Qt::SkipEmptyParts);
    for (const auto &line : lines) {
        if (line.length() < 3) continue;

        GitFileEntry entry;
        entry.status_x = line[0];
        entry.status_y = line[1];
        entry.path = line.mid(3).trimmed();

        if (entry.status_x == 'R') {
            const auto arrow_pos = entry.path.indexOf(" -> ");
            if (arrow_pos > 0) entry.path = entry.path.mid(arrow_pos + 4);
        }

        entries_.append(entry);

        const auto prefix = QString("%1%2 ").arg(entry.status_x == '?' ? '?' : entry.status_x)
                                             .arg(entry.status_y == '?' ? '?' : entry.status_y);
        auto *item = new QListWidgetItem(prefix + entry.path);

        if (entry.status_x == 'M' || entry.status_y == 'M') {
            item->setForeground(QColor("#E67E22"));
        } else if (entry.status_x == 'A') {
            item->setForeground(QColor("#27AE60"));
        } else if (entry.status_x == 'D' || entry.status_y == 'D') {
            item->setForeground(QColor("#E74C3C"));
        } else if (entry.status_x == '?') {
            item->setForeground(QColor("#999999"));
        }

        file_list_->addItem(item);
    }
}

void GitPanel::updateBranchDisplay()
{
    auto *proc = new QProcess(this);
    proc->setWorkingDirectory(QDir::currentPath());
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &GitPanel::onBranchFinished);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            proc, &QObject::deleteLater);
    proc->start("git", {"branch", "--show-current"});
}

QString GitPanel::statusCharToText(QChar x, QChar y) const
{
    if (x == '?' && y == '?') return LOC("git.status.untracked");
    if (x == 'M') return LOC("git.status.staged_modified");
    if (x == 'A') return LOC("git.status.staged_added");
    if (x == 'D') return LOC("git.status.staged_deleted");
    if (x == 'R') return LOC("git.status.staged_renamed");
    if (y == 'M') return LOC("git.status.unstaged_modified");
    if (y == 'D') return LOC("git.status.unstaged_deleted");
    return LOC("git.status.changed");
}

} // namespace NezhaIDE::Views
