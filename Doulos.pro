#-------------------------------------------------
#
# Project created by QtCreator 2018-08-08T16:07:08
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets serialport

TARGET = Doulos
TEMPLATE = app

CONFIG += console
RC_FILE += Doulos.rc

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0



win32 {
    INCLUDEPATH += $$PWD/include

    LIBS += $$PWD/lib/PX14_64.lib
    LIBS += $$PWD/lib/NIDAQmx.lib
    LIBS += $$PWD/lib/intel64_win/ippcore.lib \
            $$PWD/lib/intel64_win/ippi.lib \
            $$PWD/lib/intel64_win/ipps.lib
    debug {
        LIBS += $$PWD/lib/intel64_win/vc14/tbb_debug.lib
    }
    release {
        LIBS += $$PWD/lib/intel64_win/vc14/tbb.lib
    }
    LIBS += $$PWD/lib/intel64_win/mkl_core.lib \
            $$PWD/lib/intel64_win/mkl_tbb_thread.lib \
            $$PWD/lib/intel64_win/mkl_intel_lp64.lib
}


SOURCES += Doulos/Doulos.cpp \
    Doulos/MainWindow.cpp \
    Doulos/QStreamTab.cpp \
    Doulos/QOperationTab.cpp \
    Doulos/QDeviceControlTab.cpp \
    Doulos/QVisualizationTab.cpp \
    Doulos/Viewer/QScope.cpp \
    Doulos/Viewer/QImageView.cpp \
    Doulos/Dialog/PulseCalibDlg.cpp

SOURCES += DataAcquisition/SignatecDAQ/SignatecDAQ.cpp \
    DataAcquisition/DataProcess/DataProcess.cpp \
    DataAcquisition/ThreadManager.cpp \
    DataAcquisition/DataAcquisition.cpp

SOURCES += MemoryBuffer/MemoryBuffer.cpp

SOURCES += DeviceControl/PmtGain/PmtGainControl.cpp \
    DeviceControl/PmtGain/PmtGainMonitor.cpp \
    DeviceControl/ResonantScan/ResonantScan.cpp \
    DeviceControl/GalvoScan/GalvoScan.cpp \
    DeviceControl/ZaberStage/ZaberStage.cpp


HEADERS += Doulos/Configuration.h \
    Doulos/MainWindow.h \
    Doulos/QStreamTab.h \
    Doulos/QOperationTab.h \
    Doulos/QDeviceControlTab.h \
    Doulos/QVisualizationTab.h \
    Doulos/Viewer/QScope.h \
    Doulos/Viewer/QImageView.h \
    Doulos/Dialog/PulseCalibDlg.h

HEADERS += DataAcquisition/SignatecDAQ/SignatecDAQ.h \
    DataAcquisition/DataProcess/DataProcess.h \
    DataAcquisition/ThreadManager.h \
    DataAcquisition/DataAcquisition.h

HEADERS += MemoryBuffer/MemoryBuffer.h

HEADERS += DeviceControl/PmtGain/PmtGainControl.h \
    DeviceControl/PmtGain/PmtGainMonitor.h \
    DeviceControl/ResonantScan/ResonantScan.h \
    DeviceControl/GalvoScan/GalvoScan.h \
    DeviceControl/ZaberStage/ZaberStage.h \
    DeviceControl/QSerialComm.h


FORMS   += Doulos/MainWindow.ui
