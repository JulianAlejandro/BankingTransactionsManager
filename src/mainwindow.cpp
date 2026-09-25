#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "appinfo.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(AppInfo::DisplayName);
}

MainWindow::~MainWindow()
{
    delete ui;
}
