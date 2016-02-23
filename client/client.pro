include(../global.pri)
TEMPLATE = app
QT += core gui widgets network
CONFIG += c++11

SOURCES += main.cpp\
        mainwindow.cpp \
    dialog.cpp \
    tcpsocket.cpp \
    channeljoiningdialog.cpp \
    conversation.cpp \
    privateconversation.cpp \
    channelconversation.cpp \
    servermessagesconversation.cpp \
    mytextedit.cpp

HEADERS  += mainwindow.h \
    dialog.h \
    tcpsocket.h \
    channeljoiningdialog.h \
    conversation.h \
    privateconversation.h \
    channelconversation.h \
    servermessagesconversation.h \
    mytextedit.h

FORMS    += mainwindow.ui \
    dialog.ui \
    channeljoiningdialog.ui

DESTDIR = ../bin
TARGET = yaic-client

# our own libraries
INCLUDEPATH += ../common/include
DEPENDPATH += ../common/include
LIBS += -L../bin -lyaic

# hack
unix {
    TARGETDEPS += ../bin/libyaic.a
}

RESOURCES += \
    res.qrc
