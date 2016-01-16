QT += core network
QT -= gui

CONFIG += c++11

TARGET = Server
CONFIG += console
CONFIG -= app_bundle
CONFIG += debug_and_release

TEMPLATE = app

SOURCES += main.cpp \
    qserver.cpp \
    qclientthread.cpp

HEADERS += \
    qserver.h \
    qclientthread.h
