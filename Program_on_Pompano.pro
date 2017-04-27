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

HEADERS += \
    canctrl.h \
    kellerctrl.h \
    xsensctrl.h


target.path = /test
INSTALLS += target
