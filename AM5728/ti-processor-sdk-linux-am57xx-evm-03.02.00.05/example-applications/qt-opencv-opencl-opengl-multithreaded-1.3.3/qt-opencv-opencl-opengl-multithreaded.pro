QT += core gui

QT_CONFIG -= no-pkg-config
CONFIG += link_pkgconfig
PKGCONFIG += opencv

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qt-opencv-opencl-opengl-multithreaded
TEMPLATE = app

DEFINES += APP_VERSION=\\\"1.3.3\\\"

LIBS += -lOpenCL -locl_util

SOURCES += main.cpp \
    MainWindow.cpp \
    MatToQImage.cpp \
    FrameLabel.cpp \
    CameraView.cpp \
    ProcessingThread.cpp \
    CaptureThread.cpp \
    CameraGrab.cpp \
    CameraConnectDialog.cpp \
    ImageProcessingSettingsDialog.cpp \
    SharedImageBuffer.cpp \
    Gesture.cpp \
    WaveSimulationThread.cpp \
    WaveDisplayWidget.cpp \
    GeometryEngine.cpp

HEADERS += \
    MainWindow.h \
    Config.h \
    MatToQImage.h \
    FrameLabel.h \
    Structures.h \
    CameraView.h \
    ProcessingThread.h \
    CaptureThread.h \
    CameraGrab.h \
    CameraConnectDialog.h \
    ImageProcessingSettingsDialog.h \
    SharedImageBuffer.h \
    Buffer.h \
    Gesture.h \
    WaveSimulationThread.h \
    phillips.dsp_h \
    WaveDisplayWidget.h \
    GeometryEngine.h

FORMS += \
    MainWindow.ui \
    CameraView.ui \
    CameraConnectDialog.ui \
    ImageProcessingSettingsDialog.ui

RESOURCES += \
    shaders.qrc \
    textures.qrc

QMAKE_CXXFLAGS += -Wall
