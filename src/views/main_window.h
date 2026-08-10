//
// Created by 钟智强 on 2026/8/10.
//

#ifndef NEZHAIDE_MAIN_WINDOW_H
#define NEZHAIDE_MAIN_WINDOW_H

#include <QMainWindow>

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
