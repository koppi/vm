CONFIG += qt
QT     += core gui widgets

TARGET = vm
TEMPLATE = app

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    vm.cpp

HEADERS += \
        mainwindow.h \
    vm.h

FORMS += \
        mainwindow.ui

DISTFILES += \
    README.md
