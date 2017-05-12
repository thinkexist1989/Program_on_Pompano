#-------------------------------------------------
#
# Project created by QtCreator 2017-04-27T15:24:53
#
#-------------------------------------------------

QT       += core
QT       += serialport

QT       -= gui

TARGET = Program_on_Pompano
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    canctrl.cpp \
    kellerctrl.cpp \
    xsensctrl.cpp \
    altctrl.cpp \
    tcpctrl.cpp \
    lightctrl.cpp \
    platformctrl.cpp

HEADERS += \
    canctrl.h \
    kellerctrl.h \
    xsensctrl.h \
    altctrl.h \
    tcpctrl.h \
    lightctrl.h \
    platformctrl.h

#remote debugging, copy program to pompanoo /test directory
target.path = /test
INSTALLS += target

#Add Support to TCP
QT       += network
