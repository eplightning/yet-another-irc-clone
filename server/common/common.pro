include(../../global.pri)
TEMPLATE = lib
CONFIG -= qt app_bundle
CONFIG += shared c++11 thread

INCLUDEPATH += ../../common/include include
DEPENDPATH += ../../common/include include

DESTDIR = ../../bin
TARGET = yaicserver

HEADERS += \
    include/server/selector.h \
    selector/selector_epoll.h

SOURCES += \
    selector.cpp \
    selector/selector_epoll.cpp

macx {
    SOURCES += \
        selector/selector_kqueue.cpp

    HEADERS += \
        selector/selector_kqueue.h
}

unix:!macx {
    SOURCES += \
        selector/selector_epoll.cpp

    HEADERS += \
        selector/selector_epoll.h
}
