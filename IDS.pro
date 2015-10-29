#-------------------------------------------------
#
# Project created by QtCreator 2015-03-04T15:52:36
#
#-------------------------------------------------

QT       += core gui network webkit
win32:LIBS += -lws2_32

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = IDS
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    fancyprogressbar.cpp \
    serverlist.cpp \
    serverscroller.cpp \
    serverupdater.cpp \
    huffman.cpp \
    settings.cpp \
    filtersettingswindow.cpp \
    filtersettingsmaster.cpp \
    filterlist.cpp \
    playerspopup.cpp

HEADERS  += mainwindow.h \
    fancyprogressbar.h \
    serverlist.h \
    serverscroller.h \
    serverupdater.h \
    huffman.h \
    settings.h \
    filtersettingswindow.h \
    filtersettingsmaster.h \
    filterlist.h \
    playerspopup.h

FORMS    += mainwindow.ui \
    filtersettingswindow.ui \
    filtersettingsmaster.ui \
    filterlist.ui \
    playerspopup.ui

RESOURCES += \
    resources.qrc

OTHER_FILES += \
    IDS.rc \
    IDS.exe.manifest

win32:RC_FILE = IDS.rc
