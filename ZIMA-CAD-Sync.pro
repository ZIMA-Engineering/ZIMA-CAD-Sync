#-------------------------------------------------
#
# Project created by QtCreator 2013-03-03T16:57:02
#
#-------------------------------------------------

QT       += core gui network

TARGET = ZIMA-CAD-Sync
TEMPLATE = app

win32:INCLUDEPATH += ../
VPATH += ./src

SOURCES += ZIMA-CAD-Sync.cpp\
        MainWindow.cpp \
    src/BaseSynchronizer.cpp \
    src/FtpSynchronizer.cpp \
    src/Item.cpp

HEADERS  += MainWindow.h \
    src/BaseSynchronizer.h \
    src/FtpSynchronizer.h \
    src/Item.h

FORMS    += MainWindow.ui

CODECFORTR = UTF-8
#TRANSLATIONS += locale/ZIMA-CAD-Sync_cs_CZ.ts

RESOURCES += \
    ZIMA-CAD-Sync.qrc

