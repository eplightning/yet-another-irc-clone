include(../global.pri)
TEMPLATE = app
QT += core gui widgets network
CONFIG += c++11

SOURCES += main.cpp\
        mainwindow.cpp \
    dialog.cpp \
    channel.cpp \
    tcpsocket.cpp

HEADERS  += mainwindow.h \
    dialog.h \
    channel.h \
    tcpsocket.h

FORMS    += mainwindow.ui \
    dialog.ui

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
