include(../../global.pri)
TEMPLATE = app
CONFIG -= qt app_bundle
CONFIG += console c++11 thread

SOURCES += main.cpp \
    core/context.cpp \
    core/app.cpp \
    handlers/client.cpp

# libconfig
LIBS += -lconfig++

# our own libraries
INCLUDEPATH += ../../common/include ../common/include
DEPENDPATH += ../../common/include ../common/include
LIBS += -L../../bin -lyaic -lyaicserver

DESTDIR = ../../bin
TARGET = yaic-master

HEADERS += \
    core/app.h \
    core/context.h \
    handlers/slave.h \
    handlers/client.h
