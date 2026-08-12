#pragma once

#if __has_include(<QMainWindow>)
    #include <QMainWindow>
    #define HAS_QMAINWINDOW 1
#else
    #define HAS_QMAINWINDOW 0
    #error "缺少QT，请先安装 QT。"
#endif

class QSplitter;
class QAction;

namespace NezhaIDE::Editor {
    class EditorTabHost;
}

namespace NezhaIDE::Views {
    class ActivityBar;
    class SidebarContainer;
    class TerminalPanel;
}

namespace Ui {
    class MainWindow;
}

namespace NezhaIDE::Views {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void setupLayout();
    void setupMenuBar();
    void setupStatusBar();
    void applyStyles();
    void updateProjectRoot(const QString &projectPath);
    void updateEditActions();

    void onOpenProject();
    void onSaveFile();

    Ui::MainWindow *ui{};
    ActivityBar *activity_bar_{};
    SidebarContainer *sidebar_{};
    QSplitter *splitter_{};
    NezhaIDE::Editor::EditorTabHost *editor_host_{};

    QAction *open_project_action_{};
    QAction *save_action_{};
    QAction *quit_action_{};
    QAction *undo_action_{};
    QAction *redo_action_{};
    QAction *cut_action_{};
    QAction *copy_action_{};
    QAction *paste_action_{};
    QAction *toggle_explorer_{};
    QAction *toggle_git_{};
    QAction *toggle_http_{};
    QAction *toggle_hydra_{};
    QAction *toggle_terminal_{};
    TerminalPanel *terminal_panel_{};
};

} // namespace NezhaIDE::Views
