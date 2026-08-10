//
// Created by 钟智强 on 2026/8/10.
//

#ifndef NEZHAIDE_MAIN_WINDOW_H
#define NEZHAIDE_MAIN_WINDOW_H

#if __has_include(<QMainWindow>)
    #include <QMainWindow>
    #define HAS_QMAINWINDOW 1
#else
    #define HAS_QMAINWINDOW 0
    #error "缺少QT，请先安装 QT。"
#endif

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
        Ui::MainWindow *ui{};
    };
}

#endif //NEZHAIDE_MAIN_WINDOW_H
