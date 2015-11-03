include(../../global.pri)
TEMPLATE = lib
CONFIG -= qt app_bundle
CONFIG += shared c++11 thread

INCLUDEPATH += ../../common/include
DEPENDPATH += ../../common/include

DESTDIR = ../../bin
TARGET = yaicserver
