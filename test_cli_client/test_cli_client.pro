include(../global.pri)
TEMPLATE = app
CONFIG -= qt app_bundle
CONFIG += console c++11 thread

SOURCES += main.cpp

# our own libraries
INCLUDEPATH += ../common/include
DEPENDPATH += ../common/include
LIBS += -L../bin -lyaic

DESTDIR = ../bin
TARGET = yaic-test-cli-client
