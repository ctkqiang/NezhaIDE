#pragma once

#if __has_include(<QMainWindow>)
    #include <QMainWindow>
    #define HAS_QMAINWINDOW 1
#else
    #define HAS_QMAINWINDOW 0
    #error "缺少QT，请先安装 QT。"
#endif

class QSplitter;
class QTabWidget;

namespace Ui {
    class MainWindow;
}

namespace NezhaIDE::Views {

class SidebarContainer;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void setupLayout();
    void applyStyles();

    Ui::MainWindow *ui{};
    SidebarContainer *sidebar_{};
    QSplitter *splitter_{};
    QTabWidget *editor_host_{};
};

} // namespace NezhaIDE::Views
