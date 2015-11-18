include(../global.pri)
TEMPLATE = lib
CONFIG -= qt app_bundle
CONFIG += static c++11 thread

TARGET = yaic
DESTDIR = ../bin

HEADERS += \
    include/common/types.h \
    include/common/packet.h \
    include/common/packets/master_user.h

INCLUDEPATH += include

SOURCES += \
    packet.cpp \
    types.cpp \
    packets/master_user.cpp
