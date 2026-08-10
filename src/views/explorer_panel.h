#pragma once

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QToolBar>
#include <QAction>

namespace NezhaIDE::Views {

class ExplorerPanel : public QWidget {
    Q_OBJECT

public:
    explicit ExplorerPanel(QWidget *parent = nullptr);
    ~ExplorerPanel() override;

    void setRootPath(const QString &path);

signals:
    void fileOpened(const QString &path);
    void fileSelected(const QString &path);

private slots:
    void onNewFile();
    void onNewFolder();
    void onDeleteFile();
    void onRenameFile();
    void onOpenInFinder();
    void onTreeDoubleClicked(const QModelIndex &index);
    void onTreeSelectionChanged();
    void onCustomContextMenu(const QPoint &pos);

private:
    void setupActions();
    void setupContextMenu();
    bool confirmDelete(const QString &path);
    void applyStyles();

    QTreeView *tree_view_{};
    QFileSystemModel *fs_model_{};
    QToolBar *toolbar_{};

    QAction *new_file_action_{};
    QAction *new_folder_action_{};
    QAction *refresh_action_{};
    QAction *collapse_all_action_{};
};

} // namespace NezhaIDE::Views
