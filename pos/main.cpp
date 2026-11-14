#include <QApplication>
#include <QStyleFactory>
#include <QPalette>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Enforce consistent light retail palette across all operating systems and themes
    app.setStyle(QStyleFactory::create("Fusion"));
    QPalette lightPalette;
    lightPalette.setColor(QPalette::Window, QColor("#F4F5F7"));
    lightPalette.setColor(QPalette::WindowText, QColor("#1F2937"));
    lightPalette.setColor(QPalette::Base, QColor("#FFFFFF"));
    lightPalette.setColor(QPalette::AlternateBase, QColor("#F9FAFB"));
    lightPalette.setColor(QPalette::ToolTipBase, QColor("#1F2937"));
    lightPalette.setColor(QPalette::ToolTipText, QColor("#FFFFFF"));
    lightPalette.setColor(QPalette::Text, QColor("#1F2937"));
    lightPalette.setColor(QPalette::Button, QColor("#FFFFFF"));
    lightPalette.setColor(QPalette::ButtonText, QColor("#1F2937"));
    lightPalette.setColor(QPalette::BrightText, QColor("#DC2626"));
    lightPalette.setColor(QPalette::Highlight, QColor("#1E3A5F"));
    lightPalette.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
    lightPalette.setColor(QPalette::PlaceholderText, QColor("#9CA3AF"));
    app.setPalette(lightPalette);

    // Retail POS application metadata
    QApplication::setApplicationName("Retail POS System");
    QApplication::setApplicationVersion("2.0");
    QApplication::setOrganizationName("Retail POS");

    // Launch main POS application window
    MainWindow window;
    window.show();

    return app.exec();
}
