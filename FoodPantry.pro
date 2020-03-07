#-------------------------------------------------
#
# Project created by QtCreator 2020-02-20T08:51:40
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = FoodPantry
TEMPLATE = app


SOURCES += main.cpp\
        MainWindow.cpp \
        HX711.cpp

HEADERS  += MainWindow.h \
    HX711.h

FORMS    += MainWindow.ui

LIBS += -lwiringPi
