include(../../global.pri)
TEMPLATE = app
CONFIG -= qt app_bundle
CONFIG += console c++11 thread

SOURCES += main.cpp \
    core/app.cpp \
    modules/master.cpp \
    modules/slave.cpp

# libconfig
LIBS += -lconfig++
macx {
    LIBS += -L/usr/local/lib
    INCLUDEPATH += /usr/local/include
}

# our own libraries
INCLUDEPATH += ../../common/include ../common/include
DEPENDPATH += ../../common/include ../common/include
LIBS += -L../../bin -lyaic -lyaicserver
# hack
unix {
    TARGETDEPS += ../../bin/libyaic.a ../../bin/libyaicserver.a
}

DESTDIR = ../../bin
TARGET = yaic-slave

HEADERS += \
    core/app.h \
    modules/master.h \
    core/global.h \
    core/context.h \
    modules/slave.h
