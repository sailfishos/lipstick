include(../common.pri)
TARGET = ut_touchscreen
QT += dbus qml quick

INCLUDEPATH += $$TOUCHSCREENSRCDIR $$UTILITYSRCDIR $$SRCDIR/xtools $$QMSYSTEM2

SOURCES += ut_touchscreen.cpp \
    $$TOUCHSCREENSRCDIR/touchscreen.cpp \
    $$STUBSDIR/homeapplication.cpp \
    $$STUBSDIR/stubbase.cpp

HEADERS += ut_touchscreen.h \
    $$TOUCHSCREENSRCDIR/touchscreen.h \
    $$QMSYSTEM2/qmdisplaystate.h \
    $$SRCDIR/homeapplication.h
