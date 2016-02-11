include(../global.pri)
TEMPLATE = app
QT += core gui widgets
CONFIG += c++11

SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

DESTDIR = ../bin
TARGET = yaic-client

# our own libraries
INCLUDEPATH += ../common/include
DEPENDPATH += ../common/include
LIBS += -L../bin -lyaic

# hack
unix {
    TARGETDEPS += ../bin/libyaic.a
}
