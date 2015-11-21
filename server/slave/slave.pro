include(../../global.pri)
TEMPLATE = app
CONFIG -= qt app_bundle
CONFIG += console c++11 thread

SOURCES += main.cpp

# our own libraries
INCLUDEPATH += ../../common/include ../common/include
DEPENDPATH += ../../common/include ../common/include
LIBS += -L../../bin -lyaic -lyaicserver
# hack
unix {
    TARGETDEPS += ../../bin/libyaic.a ../../bin/libyaicserver.a
}

DESTDIR = ../../bin
TARGET = yaic-slave
