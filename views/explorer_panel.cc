#include "explorer_panel.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
#include <QApplication>
#include <QDir>
#include <QDesktopServices>
#include <QFile>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QScroller>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>

namespace NezhaIDE::Views {

ExplorerPanel::ExplorerPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    fs_model_ = new QFileSystemModel(this);
    fs_model_->setRootPath(QDir::currentPath());
    fs_model_->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);

    tree_view_ = new QTreeView(this);
    tree_view_->setModel(fs_model_);
    tree_view_->setRootIndex(fs_model_->index(QDir::currentPath()));
    tree_view_->setHeaderHidden(true);
    tree_view_->setAnimated(true);
    tree_view_->setIndentation(20);
    tree_view_->setIconSize({18, 18});
    tree_view_->setContextMenuPolicy(Qt::CustomContextMenu);
    tree_view_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tree_view_->setRootIsDecorated(true);

    for (int i = 1; i < fs_model_->columnCount(); ++i) {
        tree_view_->hideColumn(i);
    }

    connect(tree_view_, &QTreeView::doubleClicked, this, &ExplorerPanel::onTreeDoubleClicked);
    connect(tree_view_->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this] { onTreeSelectionChanged(); });
    connect(tree_view_, &QTreeView::customContextMenuRequested,
            this, &ExplorerPanel::onCustomContextMenu);

    layout->addWidget(tree_view_);

    QScroller::grabGesture(tree_view_, QScroller::LeftMouseButtonGesture);

    applyStyles();

    connect(&NezhaIDE::Services::ThemeService::instance(), &NezhaIDE::Services::ThemeService::themeChanged,
            this, [this] { applyStyles(); });
}

ExplorerPanel::~ExplorerPanel() = default;

void ExplorerPanel::setRootPath(const QString &path)
{
    fs_model_->setRootPath(path);
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
        LOC("explorer.new_file"),
        LOC("explorer.file_name"),
        QLineEdit::Normal, QString(), &ok);
    if (!ok || name.isEmpty()) return;

    auto current_dir = QDir::currentPath();
    const auto indexes = tree_view_->selectionModel()->selectedIndexes();
    if (!indexes.isEmpty()) {
        const auto path = fs_model_->filePath(indexes.first());
        current_dir = QFileInfo(path).isDir() ? path : QFileInfo(path).absolutePath();
    }

    const auto new_path = current_dir + "/" + name;
    QFile file(new_path);
    if (file.exists()) {
        QMessageBox::warning(this, LOC("error.title"),
            LOC("error.file_exists"));
        return;
    }
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, LOC("error.title"),
            LOC("error.cannot_create_file"));
        return;
    }
    file.close();

    emit fileOpened(new_path);
}

void ExplorerPanel::onNewFolder()
{
    bool ok;
    const auto name = QInputDialog::getText(this,
        LOC("explorer.new_folder"),
        LOC("explorer.folder_name"),
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
        QMessageBox::warning(this, LOC("error.title"),
            LOC("error.cannot_create_folder"));
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
        LOC("explorer.rename"),
        LOC("explorer.new_name"),
        QLineEdit::Normal, info.fileName(), &ok);
    if (!ok || new_name.isEmpty() || new_name == info.fileName()) return;

    const auto new_path = info.absolutePath() + "/" + new_name;
    if (!QFile::rename(old_path, new_path)) {
        QMessageBox::warning(this, LOC("error.title"),
            LOC("error.rename_failed"));
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
    menu.setStyleSheet(NezhaIDE::Services::ThemeService::instance().qss(QStringLiteral("style.menu")));

    menu.addAction(LOC("explorer.new_file"), this, &ExplorerPanel::onNewFile);
    menu.addAction(LOC("explorer.new_folder"), this, &ExplorerPanel::onNewFolder);
    menu.addSeparator();
    menu.addAction(LOC("explorer.rename"), this, &ExplorerPanel::onRenameFile);
    menu.addAction(LOC("explorer.delete"), this, &ExplorerPanel::onDeleteFile);
    menu.addSeparator();
    menu.addAction(LOC("explorer.open_in_finder"), this, &ExplorerPanel::onOpenInFinder);

    menu.exec(tree_view_->viewport()->mapToGlobal(pos));
}

bool ExplorerPanel::confirmDelete(const QString &path)
{
    const QFileInfo info(path);
    const auto msg = info.isDir()
        ? LOC("confirm.delete_folder").arg(info.fileName())
        : LOC("confirm.delete_file").arg(info.fileName());
    return QMessageBox::question(this, LOC("confirm.delete_title"), msg)
           == QMessageBox::Yes;
}

void ExplorerPanel::applyStyles()
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    setStyleSheet(ts.qss(QStringLiteral("style.explorer_root")));
    tree_view_->setStyleSheet(ts.qss(QStringLiteral("style.tree_view")));
}

} // namespace NezhaIDE::Views
