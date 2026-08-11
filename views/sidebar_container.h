#pragma once

#include <QWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QVBoxLayout>

namespace NezhaIDE::Views {

class ExplorerPanel;
class GitPanel;

class SidebarContainer : public QWidget {
    Q_OBJECT

public:
    explicit SidebarContainer(QWidget *parent = nullptr);
    ~SidebarContainer() override;

    ExplorerPanel *explorer() const;
    GitPanel *gitPanel() const;

    void showExplorer();
    void showGit();

private:
    void setupHeader();
    void setHeaderTitle(const QString &title);
    void applyStyles();

    QStackedWidget *stack_{};
    ExplorerPanel *explorer_panel_{};
    GitPanel *git_panel_{};

    QWidget *header_{};
    QLabel *project_name_{};
};

} // namespace NezhaIDE::Views
