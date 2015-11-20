include(../../global.pri)
TEMPLATE = lib
CONFIG -= qt app_bundle
CONFIG += static c++11 thread

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
    include/server/misc_utils.h \
    include/server/log.h \
    include/server/log/stdout.h \
    include/server/syseventloop.h

SOURCES += \
    selector.cpp \
    tcp_server.cpp \
    event.cpp \
    socket_utils.cpp \
    dispatcher.cpp \
    misc_utils.cpp \
    log.cpp \
    log/stdout.cpp \
    syseventloop.cpp

macx {
    SOURCES += \
        selector/selector_kqueue.cpp \
        syseventloop/syseventloop_kqueue.cpp

    HEADERS += \
        selector/selector_kqueue.h \
        syseventloop/syseventloop_kqueue.h
}

unix:!macx {
    SOURCES += \
        selector/selector_epoll.cpp \
        syseventloop/syseventloop_linux.cpp


    HEADERS += \
        selector/selector_epoll.h \
        syseventloop/syseventloop_linux.h
}
