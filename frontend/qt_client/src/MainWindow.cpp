#include "../include/MainWindow.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), statusLabel_(nullptr), importButton_(nullptr)
{
    setupUi();
}

void MainWindow::setupUi()
{
    auto* centralWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(centralWidget);

    statusLabel_ = new QLabel("等待接入识别任务", centralWidget);
    importButton_ = new QPushButton("导入图片", centralWidget);

    layout->addWidget(statusLabel_);
    layout->addWidget(importButton_);

    setCentralWidget(centralWidget);
    setWindowTitle("智能车牌检测识别系统");
    resize(960, 640);
}
