#include "git_panel.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
#include <QFontDatabase>
#include "src/utilities/logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <algorithm>
#include <QApplication>
#include <QDir>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QShortcut>
#include <QStyle>
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
    toolbar_->addAction(
        QApplication::style()->standardIcon(QStyle::SP_BrowserReload),
        LOC("git.refresh"), this, &GitPanel::onRefresh);
    toolbar_->addSeparator();
    toolbar_->addAction(
        QApplication::style()->standardIcon(QStyle::SP_ArrowUp),
        LOC("git.stage"), this, &GitPanel::onStageFile);
    toolbar_->addAction(
        QApplication::style()->standardIcon(QStyle::SP_ArrowDown),
        LOC("git.unstage"), this, &GitPanel::onUnstageFile);
    toolbar_->addAction(
        QIcon(QStringLiteral(":/vectors/stage_all.svg")), LOC("git.stage_all"), this, &GitPanel::onStageAll);
    toolbar_->addAction(
        QIcon(QStringLiteral(":/vectors/unstage_all.svg")), LOC("git.unstage_all"), this, &GitPanel::onUnstageAll);
    layout->addWidget(toolbar_);

    branch_label_ = new QLabel(this);
    branch_label_->setStyleSheet(
        QStringLiteral("QLabel { padding: 6px 12px; font-size: 12px; font-weight: bold; }"));
    layout->addWidget(branch_label_);

    splitter_ = new QSplitter(Qt::Vertical, this);
    splitter_->setHandleWidth(1);
    splitter_->setChildrenCollapsible(false);

    file_list_ = new QListWidget(splitter_);
    file_list_->setContextMenuPolicy(Qt::CustomContextMenu);
    file_list_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(file_list_, &QListWidget::itemClicked, this, &GitPanel::onListItemClicked);
    connect(file_list_, &QListWidget::itemDoubleClicked, this, &GitPanel::onListItemDoubleClicked);
    connect(file_list_, &QListWidget::customContextMenuRequested,
            this, &GitPanel::onCustomContextMenu);

    file_tabs_ = new QTabWidget(splitter_);
    file_tabs_->setObjectName(QStringLiteral("gitFileTabs"));
    file_tabs_->setDocumentMode(true);
    file_tabs_->addTab(file_list_, LOC("git.changes"));

    graph_view_ = new GitGraphView(file_tabs_);
    graph_view_->setObjectName(QStringLiteral("gitGraphView"));
    graph_view_->setEmptyText(LOC("git.graph_empty"));
    connect(graph_view_, &GitGraphView::commitSelected, this, &GitPanel::onCommitSelected);
    file_tabs_->addTab(graph_view_, LOC("git.history"));

    diff_view_ = new QPlainTextEdit(splitter_);
    diff_view_->setReadOnly(true);
    diff_view_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    diff_view_->setPlaceholderText(LOC("git.diff_hint"));

    splitter_->addWidget(file_tabs_);
    splitter_->addWidget(diff_view_);
    splitter_->setStretchFactor(0, 3);
    splitter_->setStretchFactor(1, 2);
    splitter_->setSizes({200, 120});
    layout->addWidget(splitter_, 1);

    auto *commit_frame = new QWidget(this);
    auto *commit_layout = new QVBoxLayout(commit_frame);
    commit_layout->setContentsMargins(8, 8, 8, 8);
    commit_layout->setSpacing(6);

    commit_message_ = new QTextEdit(this);
    commit_message_->setPlaceholderText(LOC("git.commit_placeholder"));
    commit_message_->setMaximumHeight(80);
    commit_layout->addWidget(commit_message_);

    auto *ctrlEnter = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return), commit_message_);
    connect(ctrlEnter, &QShortcut::activated, this, &GitPanel::onCommit);

    commit_button_ = new QPushButton(LOC("git.commit_button"), this);
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
}

GitPanel::~GitPanel()
{
    if (git_process_->state() != QProcess::NotRunning) {
        git_process_->kill();
        git_process_->waitForFinished(1000);
    }
}

QString GitPanel::unquoteGitPath(const QString &raw) const
{
    auto s = raw.trimmed();
    if (s.size() >= 2 && s.startsWith(QChar('"')) && s.endsWith(QChar('"'))) {
        s = s.mid(1, s.size() - 2);
        QString unescaped;
        unescaped.reserve(s.size());
        for (int i = 0; i < s.size(); ++i) {
            if (s[i] == QChar('\\') && i + 3 < s.size() && s[i + 1].isDigit()) {
                auto oct = s.mid(i + 1, 3).toInt(nullptr, 8);
                unescaped += QChar(oct);
                i += 3;
            } else if (s[i] == QChar('\\') && i + 1 < s.size()) {
                unescaped += s[i + 1];
                ++i;
            } else {
                unescaped += s[i];
            }
        }
        return unescaped;
    }
    return s;
}

void GitPanel::refresh()
{
    status_label_->setText(LOC("git.status_refreshing"));
    git_process_->terminate();
    git_process_->waitForFinished(500);
    git_process_->start("git", {"status", "--porcelain", "-u"});
    loadGraph();
}

void GitPanel::setWorkingDirectory(const QString &path)
{
    has_working_dir_ = true;
    git_process_->setWorkingDirectory(path);
    diff_view_->clear();
    updateBranchDisplay();
    refresh();
}

void GitPanel::onRefresh() { refresh(); }

void GitPanel::onStageFile()
{
    const auto selected = file_list_->selectedItems();
    const auto wd = git_process_->workingDirectory();
    for (auto *item : selected) {
        const int idx = file_list_->row(item);
        if (idx >= entries_.size()) continue;
        QProcess proc;
        proc.setWorkingDirectory(wd);
        proc.start("git", {"add", entries_[idx].path});
        proc.waitForFinished(3000);
        emit fileStaged(entries_[idx].path);
    }
    refresh();
}

void GitPanel::onStageAll()
{
    const auto wd = git_process_->workingDirectory();
    QProcess proc;
    proc.setWorkingDirectory(wd);
    proc.start("git", {"add", "-A"});
    proc.waitForFinished(10000);
    refresh();
}

void GitPanel::onUnstageFile()
{
    const auto selected = file_list_->selectedItems();
    const auto wd = git_process_->workingDirectory();
    for (auto *item : selected) {
        const int idx = file_list_->row(item);
        if (idx >= entries_.size()) continue;
        if (entries_[idx].status_x == ' ' || entries_[idx].status_x == '?') continue;
        QProcess proc;
        proc.setWorkingDirectory(wd);
        proc.start("git", {"reset", "HEAD", "--", entries_[idx].path});
        proc.waitForFinished(3000);
        emit fileUnstaged(entries_[idx].path);
    }
    refresh();
}

void GitPanel::onUnstageAll()
{
    const auto wd = git_process_->workingDirectory();
    QProcess proc;
    proc.setWorkingDirectory(wd);
    proc.start("git", {"reset", "HEAD"});
    proc.waitForFinished(10000);
    refresh();
}

void GitPanel::onDiscardFile()
{
    const auto selected = file_list_->selectedItems();
    if (selected.isEmpty()) return;
    const auto msg = LOC("confirm.discard_changes").arg(selected.size());
    if (QMessageBox::question(this, LOC("confirm.discard_title"), msg) != QMessageBox::Yes)
        return;

    const auto wd = git_process_->workingDirectory();
    for (auto *item : selected) {
        const int idx = file_list_->row(item);
        if (idx >= entries_.size()) continue;
        QProcess proc;
        proc.setWorkingDirectory(wd);
        proc.start("git", {"checkout", "--", entries_[idx].path});
        proc.waitForFinished(3000);
    }
    refresh();
}

void GitPanel::onShowDiff()
{
    const auto selected = file_list_->selectedItems();
    if (selected.isEmpty()) return;
    const int idx = file_list_->row(selected.first());
    if (idx >= entries_.size()) return;
    showDiffForFile(entries_[idx].path);
}

void GitPanel::onOpenFile()
{
    const auto selected = file_list_->selectedItems();
    if (selected.isEmpty()) return;
    const int idx = file_list_->row(selected.first());
    if (idx >= entries_.size()) return;
    const auto &entry = entries_[idx];
    if (!entry.fullPath.isEmpty() && QFileInfo::exists(entry.fullPath)) {
        emit fileOpened(entry.fullPath);
    }
}

void GitPanel::onCommit()
{
    const auto msg = commit_message_->toPlainText().trimmed();
    if (msg.isEmpty()) {
        QMessageBox::warning(this, LOC("error.title"),
            LOC("git.error_commit_message"));
        return;
    }

    NezhaIDE::Utilities::Logger::instance().log(
        NezhaIDE::Utilities::LogLevel::Info, __FILE__, __LINE__, __func__,
        "Git 提交: {}", msg.left(60).toStdString());

    auto *proc = new QProcess(this);
    proc->setWorkingDirectory(git_process_->workingDirectory());
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, msg, proc](int exitCode, QProcess::ExitStatus) {
        if (exitCode != 0) {
            QMessageBox::warning(this, LOC("git.error_commit_failed"),
                QString::fromUtf8(proc->readAllStandardError()));
        } else {
            emit commitRequested(msg);
            commit_message_->clear();
            diff_view_->clear();
            refresh();
        }
        proc->deleteLater();
    });
    proc->start("git", {"commit", "-m", msg});
}

void GitPanel::onStatusFinished(int exit_code, QProcess::ExitStatus status)
{
    if (status == QProcess::NormalExit && exit_code == 0) {
        const auto output = QString::fromUtf8(git_process_->readAllStandardOutput());
        parseStatusOutput(output);
        status_label_->setText(
            entries_.isEmpty()
                ? LOC("git.working_clean")
                : LOC("git.files_changed").arg(entries_.size())
        );
    } else {
        const auto err = QString::fromUtf8(git_process_->readAllStandardError()).trimmed();
        status_label_->setText(
            err.isEmpty() ? LOC("git.not_repo") : LOC("git.error_prefix").arg(err));
    }
}

void GitPanel::onBranchFinished(int exit_code, QProcess::ExitStatus status)
{
    if (status == QProcess::NormalExit && exit_code == 0) {
        const auto branch = QString::fromUtf8(git_process_->readAllStandardOutput()).trimmed();
        branch_label_->setText(LOC("git.branch").arg(branch));
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
        showDiffForFile(entry.path);
    }
}

void GitPanel::onListItemDoubleClicked(QListWidgetItem *item)
{
    const int idx = file_list_->row(item);
    if (idx < entries_.size() && !entries_[idx].fullPath.isEmpty()) {
        if (QFileInfo::exists(entries_[idx].fullPath)) {
            emit fileOpened(entries_[idx].fullPath);
        }
    }
}

void GitPanel::onCommitSelected(const QString &hash, const QString &subject)
{
    status_label_->setText(
        QStringLiteral("%1 · %2").arg(hash.left(7), subject));
    auto *proc = new QProcess(this);
    proc->setWorkingDirectory(git_process_->workingDirectory());
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc](int exitCode, QProcess::ExitStatus) {
        if (exitCode == 0) {
            diff_view_->setPlainText(QString::fromUtf8(proc->readAllStandardOutput()));
        } else {
            diff_view_->setPlainText(
                QString::fromUtf8(proc->readAllStandardError()));
        }
        proc->deleteLater();
    });
    proc->start("git", {"show", "--stat", "--format=medium", "--no-color", hash});
}

/**
 * 异步加载提交图：git log 输出 commit 列表，for-each-ref 输出 refs 映射，
 * 两者分别到达后交给 GitGraphView 合并渲染。
 */
void GitPanel::loadGraph()
{
    if (!has_working_dir_) return;
    auto *proc = new QProcess(this);
    proc->setWorkingDirectory(git_process_->workingDirectory());
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc](int exitCode, QProcess::ExitStatus) {
        if (exitCode == 0) {
            QList<GitGraphCommit> commits;
            const auto records = QString::fromUtf8(proc->readAllStandardOutput())
                                     .split(QChar(0x1e), Qt::SkipEmptyParts);
            commits.reserve(records.size());
            for (const auto &rec : records) {
                const auto f = rec.split(QChar(0x1f));
                if (f.size() < 5) continue;
                GitGraphCommit c;
                c.hash = f[0];
                c.parents = f[1].isEmpty()
                                ? QStringList{}
                                : f[1].split(' ', Qt::SkipEmptyParts);
                c.author = f[2];
                c.date = f[3];
                c.subject = f[4];
                commits.append(c);
            }
            graph_view_->setCommits(commits);

            auto *refProc = new QProcess(this);
            refProc->setWorkingDirectory(git_process_->workingDirectory());
            connect(refProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                    this, [this, refProc](int refCode, QProcess::ExitStatus) {
                if (refCode == 0) {
                    QHash<QString, QStringList> refs;
                    const auto lines = QString::fromUtf8(refProc->readAllStandardOutput())
                                           .split('\n', Qt::SkipEmptyParts);
                    for (const auto &line : lines) {
                        const auto parts = line.split(QChar(0x1f));
                        if (parts.size() < 2) continue;
                        refs[parts[1]].append(parts[0]);
                    }
                    graph_view_->setRefs(refs);
                }
                refProc->deleteLater();
            });
            refProc->start("git",
                           {"for-each-ref", "--format=%(refname:short)%x1f%(objectname)"});
        }
        proc->deleteLater();
    });
    proc->start("git", {"log", "--all", "--date-order",
                        "--pretty=format:%H%x1f%P%x1f%an%x1f%ai%x1f%s%x1e",
                        "--max-count=500"});
}

void GitPanel::onCustomContextMenu(const QPoint &pos)
{
    const auto selected = file_list_->selectedItems();
    if (selected.isEmpty()) return;

    QMenu menu(this);
    menu.setStyleSheet(NezhaIDE::Services::ThemeService::instance().qss(QStringLiteral("style.menu")));

    const int idx = file_list_->row(selected.first());
    bool hasStaged = false;
    bool hasUnstaged = false;
    for (auto *item : selected) {
        const int i = file_list_->row(item);
        if (i >= entries_.size()) continue;
        if (entries_[i].status_x != ' ' && entries_[i].status_x != '?') hasStaged = true;
        if (entries_[i].status_y != ' ') hasUnstaged = true;
    }

    menu.addAction(LOC("git.stage"), this, &GitPanel::onStageFile);
    menu.addAction(LOC("git.unstage"), this, &GitPanel::onUnstageFile)->setEnabled(hasStaged);
    menu.addSeparator();
    menu.addAction(LOC("git.discard"), this, &GitPanel::onDiscardFile)->setEnabled(hasUnstaged);
    menu.addSeparator();
    menu.addAction(LOC("git.show_diff"), this, &GitPanel::onShowDiff);
    menu.addAction(LOC("git.open_file"), this, &GitPanel::onOpenFile);

    menu.exec(file_list_->viewport()->mapToGlobal(pos));
}

void GitPanel::showDiffForFile(const QString &path)
{
    if (path.isEmpty()) return;
    auto *proc = new QProcess(this);
    proc->setWorkingDirectory(git_process_->workingDirectory());
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc](int exitCode, QProcess::ExitStatus) {
        if (exitCode == 0) {
            auto diff = QString::fromUtf8(proc->readAllStandardOutput());
            diff_view_->setPlainText(diff);
        } else {
            diff_view_->setPlainText(
                QString::fromUtf8(proc->readAllStandardError()));
        }
        proc->deleteLater();
    });
    proc->start("git", {"diff", "--", path});
}

void GitPanel::parseStatusOutput(const QString &output)
{
    entries_.clear();
    file_list_->clear();

    const auto lines = output.split('\n', Qt::SkipEmptyParts);
    auto &ts = NezhaIDE::Services::ThemeService::instance();

    for (const auto &line : lines) {
        if (line.length() < 3) continue;

        GitFileEntry entry;
        entry.status_x = line[0];
        entry.status_y = line[1];

        auto rawPath = line.mid(3);
        if (entry.status_x == 'R') {
            auto arrowIdx = rawPath.indexOf(QStringLiteral(" -> "));
            if (arrowIdx > 0) {
                rawPath = rawPath.mid(arrowIdx + 4);
            }
        }
        entry.path = unquoteGitPath(rawPath);
        entry.fullPath = QDir(git_process_->workingDirectory()).filePath(entry.path);
        entries_.append(entry);

        auto label = QStringLiteral("[%1%2] %3")
            .arg(entry.status_x != ' ' ? entry.status_x : QChar('_'))
            .arg(entry.status_y != ' ' ? entry.status_y : QChar('_'))
            .arg(entry.path);

        auto *item = new QListWidgetItem(label);
        item->setForeground(statusColor(entry.status_x, entry.status_y));
        file_list_->addItem(item);
    }
}

QColor GitPanel::statusColor(QChar x, QChar y) const
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    if (x == 'D' || y == 'D') return ts.qcolor(QStringLiteral("git.deleted"));
    if (x == 'A') return ts.qcolor(QStringLiteral("git.added"));
    if (x == 'M' || y == 'M') return ts.qcolor(QStringLiteral("git.modified"));
    if (x == '?' || y == '?') return ts.qcolor(QStringLiteral("git.untracked"));
    if (x == 'R') return ts.qcolor(QStringLiteral("git.modified"));
    return ts.qcolor(QStringLiteral("text.primary"));
}

/**
 * 更新分支显示。
 *
 * 未打开项目时直接返回，避免启动阶段在启动目录产生无意义的 git 进程。
 */
void GitPanel::updateBranchDisplay()
{
    if (!has_working_dir_) return;
    auto *proc = new QProcess(this);
    proc->setWorkingDirectory(git_process_->workingDirectory());
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &GitPanel::onBranchFinished);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            proc, &QObject::deleteLater);
    proc->start("git", {"branch", "--show-current"});
}

QString GitPanel::statusCharToText(QChar x, QChar y) const
{
    if (x == '?' && y == '?') return LOC("git.status.untracked");
    if (x == 'M' && y == 'M') return LOC("git.status.both_modified");
    if (x == 'M') return LOC("git.status.staged_modified");
    if (x == 'A') return LOC("git.status.staged_added");
    if (x == 'D') return LOC("git.status.staged_deleted");
    if (x == 'R') return LOC("git.status.staged_renamed");
    if (x == 'C') return LOC("git.status.staged_copied");
    if (x == 'U' || y == 'U') return LOC("git.status.conflict");
    if (y == 'M') return LOC("git.status.unstaged_modified");
    if (y == 'D') return LOC("git.status.unstaged_deleted");
    if (y == 'T') return LOC("git.status.typechange");
    return LOC("git.status.changed");
}

void GitPanel::applyStyles()
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    toolbar_->setStyleSheet(ts.qss(QStringLiteral("style.toolbar")));
    branch_label_->setStyleSheet(ts.qss(QStringLiteral("style.branch_label")));
    file_list_->setStyleSheet(ts.qss(QStringLiteral("style.list_widget")));
    file_tabs_->setStyleSheet(ts.qss(QStringLiteral("style.tab_widget")));
    commit_message_->setStyleSheet(ts.qss(QStringLiteral("style.commit_input")));
    commit_button_->setStyleSheet(ts.qss(QStringLiteral("style.primary_button")));
    status_label_->setStyleSheet(ts.qss(QStringLiteral("style.status_label")));
    if (auto *cf = commit_message_->parentWidget()) {
        cf->setStyleSheet(ts.qss(QStringLiteral("style.commit_frame")));
    }
    diff_view_->setStyleSheet(ts.qss(QStringLiteral("style.http_response_body")));
    graph_view_->refresh();
}

void GitPanel::applyGitColors()
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    for (int i = 0; i < std::min(static_cast<int>(entries_.size()), file_list_->count()); ++i) {
        auto *item = file_list_->item(i);
        if (!item) continue;
        const auto &entry = entries_[i];
        item->setForeground(statusColor(entry.status_x, entry.status_y));
    }
}

} // namespace NezhaIDE::Views
