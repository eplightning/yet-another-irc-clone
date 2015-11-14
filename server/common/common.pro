include(../../global.pri)
TEMPLATE = lib
CONFIG -= qt app_bundle
CONFIG += shared c++11 thread

INCLUDEPATH += ../../common/include include
DEPENDPATH += ../../common/include include
LIBS += -L../../bin -lyaic

DESTDIR = ../../bin
TARGET = yaicserver

HEADERS += \
    include/server/selector.h \
    include/server/tcp_server.h \
    include/server/event.h \
    include/server/socket_utils.h \
    include/server/dispatcher.h \
    include/server/misc_utils.h

SOURCES += \
    selector.cpp \
    tcp_server.cpp \
    event.cpp \
    socket_utils.cpp \
    dispatcher.cpp \
    misc_utils.cpp

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
