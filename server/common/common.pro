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
    include/server/event.h \
    include/server/dispatcher.h \
    include/server/misc_utils.h \
    include/server/logger/stdout.h \
    include/server/syseventloop.h \
    include/server/logger.h \
    include/server/tcp.h \
    include/server/tcp_manager.h

SOURCES += \
    selector.cpp \
    event.cpp \
    dispatcher.cpp \
    misc_utils.cpp \
    logger/stdout.cpp \
    syseventloop.cpp \
    logger.cpp \
    tcp.cpp \
    tcp_manager.cpp

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
