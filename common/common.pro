include(../global.pri)
TEMPLATE = lib
CONFIG -= qt app_bundle
CONFIG += shared c++11 thread

TARGET = yaic
DESTDIR = ../bin

HEADERS += \
    include/common/types.h \
    include/common/packet.h \
    include/common/packets/master_client.h

INCLUDEPATH += include

SOURCES += \
    packet.cpp \
    packets/master_client.cpp
