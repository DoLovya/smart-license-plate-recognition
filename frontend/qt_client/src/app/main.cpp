#include <QApplication>

#include "app/MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("SmartLicensePlateQtClient");
    app.setOrganizationName("smart-license-plate-recognition");

    MainWindow window;
    window.show();
    return app.exec();
}
