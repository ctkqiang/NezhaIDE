#include "git_panel.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <algorithm>
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
    toolbar_->addAction(QStringLiteral("↻ ") + LOC("git.refresh"), this, &GitPanel::onRefresh);
    toolbar_->addSeparator();
    toolbar_->addAction(QStringLiteral("＋ ") + LOC("git.stage"), this, &GitPanel::onStageFile);
    toolbar_->addAction(QStringLiteral("－ ") + LOC("git.unstage"), this, &GitPanel::onUnstageFile);
    layout->addWidget(toolbar_);

    branch_label_ = new QLabel(this);
    layout->addWidget(branch_label_);

    file_list_ = new QListWidget(this);
    file_list_->setContextMenuPolicy(Qt::CustomContextMenu);
    file_list_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(file_list_, &QListWidget::itemClicked, this, &GitPanel::onListItemClicked);
    layout->addWidget(file_list_, 1);

    auto *commit_frame = new QWidget(this);
    auto *commit_layout = new QVBoxLayout(commit_frame);
    commit_layout->setContentsMargins(8, 8, 8, 8);
    commit_layout->setSpacing(6);

    commit_message_ = new QTextEdit(this);
    commit_message_->setPlaceholderText(LOC("git.commit_placeholder"));
    commit_message_->setMaximumHeight(80);
    commit_layout->addWidget(commit_message_);

    commit_button_ = new QPushButton(QStringLiteral("✓ ") + LOC("git.commit_button"), this);
    connect(commit_button_, &QPushButton::clicked, this, &GitPanel::onCommit);
    commit_layout->addWidget(commit_button_, 0, Qt::AlignRight);

    layout->addWidget(commit_frame);

    status_label_ = new QLabel(this);
    layout->addWidget(status_label_);

    git_process_ = new QProcess(this);
    git_process_->setWorkingDirectory(QDir::currentPath());
    connect(git_process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &GitPanel::onStatusFinished);

    applyStyles();

    connect(&NezhaIDE::Services::ThemeService::instance(), &NezhaIDE::Services::ThemeService::themeChanged,
            this, [this] {
        applyStyles();
        applyGitColors();
    });

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

void GitPanel::setWorkingDirectory(const QString &path)
{
    git_process_->setWorkingDirectory(path);
    updateBranchDisplay();
    refresh();
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
            item->setForeground(NezhaIDE::Services::ThemeService::instance().qcolor(QStringLiteral("git.modified")));
        } else if (entry.status_x == 'A') {
            item->setForeground(NezhaIDE::Services::ThemeService::instance().qcolor(QStringLiteral("git.added")));
        } else if (entry.status_x == 'D' || entry.status_y == 'D') {
            item->setForeground(NezhaIDE::Services::ThemeService::instance().qcolor(QStringLiteral("git.deleted")));
        } else if (entry.status_x == '?') {
            item->setForeground(NezhaIDE::Services::ThemeService::instance().qcolor(QStringLiteral("git.untracked")));
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

void GitPanel::applyStyles()
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    toolbar_->setStyleSheet(ts.qss(QStringLiteral("style.toolbar")));
    branch_label_->setStyleSheet(ts.qss(QStringLiteral("style.branch_label")));
    file_list_->setStyleSheet(ts.qss(QStringLiteral("style.list_widget")));
    commit_message_->setStyleSheet(ts.qss(QStringLiteral("style.commit_input")));
    commit_button_->setStyleSheet(ts.qss(QStringLiteral("style.primary_button")));
    status_label_->setStyleSheet(ts.qss(QStringLiteral("style.status_label")));
    if (auto *cf = commit_message_->parentWidget()) {
        cf->setStyleSheet(ts.qss(QStringLiteral("style.commit_frame")));
    }
}

void GitPanel::applyGitColors()
{
    for (int i = 0; i < std::min(static_cast<int>(entries_.size()), file_list_->count()); ++i) {
        auto *item = file_list_->item(i);
        if (!item) continue;
        const auto &entry = entries_[i];
        if (entry.status_x == 'M' || entry.status_y == 'M') {
            item->setForeground(NezhaIDE::Services::ThemeService::instance().qcolor(QStringLiteral("git.modified")));
        } else if (entry.status_x == 'A') {
            item->setForeground(NezhaIDE::Services::ThemeService::instance().qcolor(QStringLiteral("git.added")));
        } else if (entry.status_x == 'D' || entry.status_y == 'D') {
            item->setForeground(NezhaIDE::Services::ThemeService::instance().qcolor(QStringLiteral("git.deleted")));
        } else if (entry.status_x == '?') {
            item->setForeground(NezhaIDE::Services::ThemeService::instance().qcolor(QStringLiteral("git.untracked")));
        }
    }
}

} // namespace NezhaIDE::Views
