//
// Created by 钟智强 on 2026/8/10.
//

#include "main_window.h"
#include "ui_main_window.h"

namespace NezhaIDE::Views {

    MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent), ui(new Ui::MainWindow) {
        ui->setupUi(this);
    }

    MainWindow::~MainWindow() {
        delete ui;
    }
}
