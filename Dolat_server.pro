# Add required Qt modules
QT       += core gui network

# Add widgets module for Qt5 and above
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# Enable C++11 features
CONFIG += c++11

# Disable deprecated Qt APIs before version 6 (optional)
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

# Source files used in the project
SOURCES += \
    main.cpp \           # main function file
    mainwindow.cpp       # main window implementation file

# Header files used in the project
HEADERS += \
    mainwindow.h         # main window header file

# UI form files used in the project
FORMS += \
    mainwindow.ui        # main window design file

# Deployment rules for different platforms
qnx: target.path = /tmp/$${TARGET}/bin          # install path for QNX
else: unix:!android: target.path = /opt/$${TARGET}/bin  # install path for Linux
!isEmpty(target.path): INSTALLS += target       # include in install process
