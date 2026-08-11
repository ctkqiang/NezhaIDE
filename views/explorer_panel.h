#pragma once

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>

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

public slots:
    void onNewFile();
    void onNewFolder();
    void onDeleteFile();
    void onRenameFile();
    void onOpenInFinder();
    void onTreeDoubleClicked(const QModelIndex &index);
    void onTreeSelectionChanged();
    void onCustomContextMenu(const QPoint &pos);

private:
    bool confirmDelete(const QString &path);
    void applyStyles();

    QTreeView *tree_view_{};
    QFileSystemModel *fs_model_{};
};

} // namespace NezhaIDE::Views
