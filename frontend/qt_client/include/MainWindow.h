#pragma once

#include <QMainWindow>

class QLabel;
class QPushButton;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void setupUi();

    QLabel* statusLabel_;
    QPushButton* importButton_;
};
