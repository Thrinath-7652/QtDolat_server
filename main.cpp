#include "mainwindow.h"   // Include the header file for MainWindow class
#include <QApplication>    // Include QApplication class for GUI application handling

int main(int argc, char *argv[])   // Main function - entry point of the application
{
    QApplication a(argc, argv);    // Create an application object to manage app control flow and resources
    MainWindow w;                  // Create an instance of MainWindow
    w.show();                      // Display the main window on the screen
    return a.exec();               // Enter the main event loop and wait for user actions
}
