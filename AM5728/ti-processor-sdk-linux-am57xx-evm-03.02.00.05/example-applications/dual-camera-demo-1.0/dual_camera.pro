#-------------------------------------------------
#
# Project created by QtCreator 2014-01-15T08:40:35
#
#-------------------------------------------------

QT       += core gui widgets

lessThan(QT_MAJOR_VERSION, 4): QT += threads

TARGET = dual_camera
TEMPLATE = app
LIBS += -ljpeg -ldrm_omap -ldrm 
INCLUDEPATH += $(SDK_PATH_TARGET)/usr/include/libdrm $(SDK_PATH_TARGET)/usr/include/omap

SOURCES += main.cpp mainwindow.cpp jpeg.c loopback.c
HEADERS  += mainwindow.h  jpeg.h  loopback.h
FORMS    += mainwindow.ui
