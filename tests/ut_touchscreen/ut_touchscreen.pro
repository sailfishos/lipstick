include(../common.pri)
TARGET = ut_touchscreen
QT += dbus qml quick
CONFIG += link_pkgconfig
PKGCONFIG += mce

INCLUDEPATH += $$TOUCHSCREENSRCDIR $$UTILITYSRCDIR $$SRCDIR/xtools $$DEVICESTATE

SOURCES += ut_touchscreen.cpp \
    $$TOUCHSCREENSRCDIR/touchscreen.cpp \
    $$STUBSDIR/homeapplication.cpp \
    $$STUBSDIR/stubbase.cpp

HEADERS += ut_touchscreen.h \
    $$TOUCHSCREENSRCDIR/touchscreen.h \
    $$DEVICESTATE/displaystate.h \
    $$SRCDIR/homeapplication.h
