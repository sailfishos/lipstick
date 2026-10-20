include(../common.pri)
TARGET = ut_intenturl
QT -= gui
PKGCONFIG -= mlite5 systemsettings
CONFIG += c++11 testcase
INCLUDEPATH += $$PWD/../../src
SOURCES += ut_intenturl.cpp
HEADERS += $$PWD/../../src/intenturl.h
