#pragma once

#if __has_include(<QMainWindow>)
    #include <QMainWindow>
    #define HAS_QMAINWINDOW 1
#else
    #define HAS_QMAINWINDOW 0
    #error "缺少QT，请先安装 QT。"
#endif

#include <QHash>
#include <QMetaObject>

class QSplitter;
class QAction;
class QLabel;
class QPlainTextEdit;

namespace NezhaIDE::Editor {
    class EditorTabHost;
    class CodeEditor;
}

namespace NezhaIDE::Views {
    class ActivityBar;
    class SidebarContainer;
    class TerminalPanel;
    class BottomPanel;
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
    void updateStatusFromEditor(NezhaIDE::Editor::CodeEditor *editor);

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
    BottomPanel *bottom_panel_{};

    // 状态栏组件
    QLabel *cur_pos_label_{};
    QLabel *lang_label_{};
    QLabel *encoding_label_{};

    // 光标位置连接去重：每个编辑器至多一条 cursorPositionChanged 连接
    QHash<QPlainTextEdit *, QMetaObject::Connection> cursor_conns_{};
};

} // namespace NezhaIDE::Views
