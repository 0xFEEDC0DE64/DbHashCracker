QT += core sql
QT -= gui widgets

DBLIBS +=

TARGET = hashcracker

PROJECT_ROOT = ..

SOURCES += main.cpp \
    workerthread.cpp

HEADERS += \
    workerthread.h

FORMS +=

RESOURCES +=

TRANSLATIONS +=

include($${PROJECT_ROOT}/app.pri)
