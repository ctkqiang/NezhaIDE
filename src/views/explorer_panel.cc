#include "explorer_panel.h"
#include <QVBoxLayout>
#include <QMenu>
#include <QMessageBox>
#include <QInputDialog>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>

namespace NezhaIDE::Views {

ExplorerPanel::ExplorerPanel(QWidget *parent)
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
    layout->addWidget(toolbar_);

    setupActions();

    fs_model_ = new QFileSystemModel(this);
    fs_model_->setRootPath(QDir::currentPath());
    fs_model_->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);

    tree_view_ = new QTreeView(this);
    tree_view_->setModel(fs_model_);
    tree_view_->setRootIndex(fs_model_->index(QDir::currentPath()));
    tree_view_->setHeaderHidden(true);
    tree_view_->setAnimated(true);
    tree_view_->setIndentation(16);
    tree_view_->setContextMenuPolicy(Qt::CustomContextMenu);
    tree_view_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tree_view_->setStyleSheet(
        "QTreeView { border: none; background: #F5F6F7; }"
        "QTreeView::item { padding: 4px 8px; border-radius: 4px; }"
        "QTreeView::item:hover { background: rgba(0,0,0,0.04); }"
        "QTreeView::item:selected { background: rgba(51,112,255,0.12); color: #3370FF; }"
        "QTreeView::branch:has-siblings:!adjoins-item { border-image: none; }"
        "QTreeView::branch:has-siblings:adjoins-item { border-image: none; }"
        "QTreeView::branch:!has-children:!has-siblings:adjoins-item { border-image: none; }"
    );

    for (int i = 1; i < fs_model_->columnCount(); ++i) {
        tree_view_->hideColumn(i);
    }

    connect(tree_view_, &QTreeView::doubleClicked, this, &ExplorerPanel::onTreeDoubleClicked);
    connect(tree_view_->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this] { onTreeSelectionChanged(); });
    connect(tree_view_, &QTreeView::customContextMenuRequested,
            this, &ExplorerPanel::onCustomContextMenu);

    layout->addWidget(tree_view_);
}

ExplorerPanel::~ExplorerPanel() = default;

void ExplorerPanel::setupActions()
{
    new_file_action_ = toolbar_->addAction(QStringLiteral("➕ 文件"));
    new_folder_action_ = toolbar_->addAction(QStringLiteral("\U0001F4C1 文件夹"));
    toolbar_->addSeparator();
    refresh_action_ = toolbar_->addAction(QStringLiteral("↻ 刷新"));
    collapse_all_action_ = toolbar_->addAction(QStringLiteral("▲ 折叠"));

    connect(new_file_action_, &QAction::triggered, this, &ExplorerPanel::onNewFile);
    connect(new_folder_action_, &QAction::triggered, this, &ExplorerPanel::onNewFolder);
    connect(refresh_action_, &QAction::triggered, this, [this] {
        fs_model_->setRootPath(QDir::currentPath());
    });
    connect(collapse_all_action_, &QAction::triggered, tree_view_, &QTreeView::collapseAll);
}

void ExplorerPanel::setRootPath(const QString &path)
{
    tree_view_->setRootIndex(fs_model_->index(path));
}

void ExplorerPanel::onTreeDoubleClicked(const QModelIndex &index)
{
    const auto file_path = fs_model_->filePath(index);
    const QFileInfo info(file_path);
    if (info.isFile()) {
        emit fileOpened(file_path);
    }
}

void ExplorerPanel::onTreeSelectionChanged()
{
    const auto indexes = tree_view_->selectionModel()->selectedIndexes();
    if (!indexes.isEmpty()) {
        emit fileSelected(fs_model_->filePath(indexes.first()));
    }
}

void ExplorerPanel::onNewFile()
{
    bool ok;
    const auto name = QInputDialog::getText(this,
        QStringLiteral("新建文件"),
        QStringLiteral("文件名:"),
        QLineEdit::Normal, QString(), &ok);
    if (!ok || name.isEmpty()) return;

    auto current_dir = QDir::currentPath();
    const auto indexes = tree_view_->selectionModel()->selectedIndexes();
    if (!indexes.isEmpty()) {
        const auto path = fs_model_->filePath(indexes.first());
        current_dir = QFileInfo(path).isDir() ? path : QFileInfo(path).absolutePath();
    }

    QFile file(current_dir + "/" + name);
    if (file.exists()) {
        QMessageBox::warning(this, QStringLiteral("错误"),
            QStringLiteral("文件已存在"));
        return;
    }
    file.open(QIODevice::WriteOnly);
    file.close();
}

void ExplorerPanel::onNewFolder()
{
    bool ok;
    const auto name = QInputDialog::getText(this,
        QStringLiteral("新建文件夹"),
        QStringLiteral("文件夹名:"),
        QLineEdit::Normal, QString(), &ok);
    if (!ok || name.isEmpty()) return;

    auto current_dir = QDir::currentPath();
    const auto indexes = tree_view_->selectionModel()->selectedIndexes();
    if (!indexes.isEmpty()) {
        const auto path = fs_model_->filePath(indexes.first());
        current_dir = QFileInfo(path).isDir() ? path : QFileInfo(path).absolutePath();
    }

    QDir dir(current_dir);
    if (!dir.mkdir(name)) {
        QMessageBox::warning(this, QStringLiteral("错误"),
            QStringLiteral("无法创建文件夹"));
    }
}

void ExplorerPanel::onDeleteFile()
{
    const auto indexes = tree_view_->selectionModel()->selectedIndexes();
    if (indexes.isEmpty()) return;

    const auto path = fs_model_->filePath(indexes.first());
    if (!confirmDelete(path)) return;

    const QFileInfo info(path);
    if (info.isDir()) {
        QDir(path).removeRecursively();
    } else {
        QFile::remove(path);
    }
}

void ExplorerPanel::onRenameFile()
{
    const auto indexes = tree_view_->selectionModel()->selectedIndexes();
    if (indexes.isEmpty()) return;

    const auto old_path = fs_model_->filePath(indexes.first());
    const QFileInfo info(old_path);
    bool ok;
    const auto new_name = QInputDialog::getText(this,
        QStringLiteral("重命名"),
        QStringLiteral("新名称:"),
        QLineEdit::Normal, info.fileName(), &ok);
    if (!ok || new_name.isEmpty() || new_name == info.fileName()) return;

    const auto new_path = info.absolutePath() + "/" + new_name;
    if (!QFile::rename(old_path, new_path)) {
        QMessageBox::warning(this, QStringLiteral("错误"),
            QStringLiteral("重命名失败"));
    }
}

void ExplorerPanel::onOpenInFinder()
{
    const auto indexes = tree_view_->selectionModel()->selectedIndexes();
    if (indexes.isEmpty()) return;

    auto path = fs_model_->filePath(indexes.first());
    const QFileInfo info(path);
    if (info.isFile()) path = info.absolutePath();

    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void ExplorerPanel::onCustomContextMenu(const QPoint &pos)
{
    const auto index = tree_view_->indexAt(pos);
    if (!index.isValid()) return;

    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { border: 1px solid #E0E0E0; border-radius: 6px; padding: 4px; }"
        "QMenu::item { padding: 6px 24px; border-radius: 4px; }"
        "QMenu::item:hover { background: rgba(0,0,0,0.04); }"
    );

    menu.addAction(QStringLiteral("☁️ 新建文件"), this, &ExplorerPanel::onNewFile);
    menu.addAction(QStringLiteral("\U0001F4C1 新建文件夹"), this, &ExplorerPanel::onNewFolder);
    menu.addSeparator();
    menu.addAction(QStringLiteral("✏️ 重命名"), this, &ExplorerPanel::onRenameFile);
    menu.addAction(QStringLiteral("\U0001F5D1️ 删除"), this, &ExplorerPanel::onDeleteFile);
    menu.addSeparator();
    menu.addAction(QStringLiteral("\U0001F4C2 在 Finder 中打开"), this, &ExplorerPanel::onOpenInFinder);

    menu.exec(tree_view_->viewport()->mapToGlobal(pos));
}

bool ExplorerPanel::confirmDelete(const QString &path)
{
    const QFileInfo info(path);
    const auto msg = info.isDir()
        ? QStringLiteral("确定删除文件夹「%1」及其所有内容？").arg(info.fileName())
        : QStringLiteral("确定删除文件「%1」？").arg(info.fileName());
    return QMessageBox::question(this, QStringLiteral("确认删除"), msg)
           == QMessageBox::Yes;
}

} // namespace NezhaIDE::Views
