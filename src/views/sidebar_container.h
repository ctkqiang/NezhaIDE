#pragma once

#include <QWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>

namespace NezhaIDE::Views {

class ExplorerPanel;
class GitPanel;

enum class SidebarTab {
    Explorer = 0,
    Git = 1
};

class SidebarContainer : public QWidget {
    Q_OBJECT

public:
    explicit SidebarContainer(QWidget *parent = nullptr);
    ~SidebarContainer() override;

    ExplorerPanel *explorer() const;
    GitPanel *gitPanel() const;

public slots:
    void switchToExplorer();
    void switchToGit();
    void switchToTab(SidebarTab tab);

signals:
    void tabChanged(SidebarTab tab);

private:
    void setupHeader();
    void updateTabStyles();
    void applyStyles();

    QStackedWidget *stack_{};
    ExplorerPanel *explorer_panel_{};
    GitPanel *git_panel_{};

    QWidget *header_{};
    QLabel *section_title_{};
    QPushButton *explorer_tab_{};
    QPushButton *git_tab_{};
    SidebarTab current_tab_ = SidebarTab::Explorer;
};

} // namespace NezhaIDE::Views
