#include <QApplication>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Cross-platform application metadata
    QApplication::setApplicationName("Retail POS System");
    QApplication::setApplicationVersion("2.0.0");
    QApplication::setOrganizationName("FAST-NUCES ISE Team");
    QApplication::setOrganizationDomain("fast.nu.edu.pk");

    // Launch main POS application window
    MainWindow window;
    window.show();

    return app.exec();
}
