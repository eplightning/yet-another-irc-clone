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
    include/server/timer.h

SOURCES += \
    selector.cpp \
    tcp_server.cpp \
    event.cpp \
    socket_utils.cpp \
    dispatcher.cpp \
    misc_utils.cpp \
    log.cpp \
    log/stdout.cpp \
    timer.cpp

macx {
    SOURCES += \
        selector/selector_kqueue.cpp \
        timer/timer_kqueue.cpp

    HEADERS += \
        selector/selector_kqueue.h \
        timer/timer_kqueue.h
}

unix:!macx {
    SOURCES += \
        selector/selector_epoll.cpp \
        timer/timer_linux.cpp

    HEADERS += \
        selector/selector_epoll.h \
        timer/timer_linux.h
}
