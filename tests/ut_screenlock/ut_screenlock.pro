include(../common.pri)
TARGET = ut_screenlock
QT += dbus qml quick

INCLUDEPATH += $$SCREENLOCKSRCDIR $$UTILITYSRCDIR $$SRCDIR/xtools ../../src/qmsystem2

SOURCES += ut_screenlock.cpp \
    $$SCREENLOCKSRCDIR/screenlock.cpp \
    $$STUBSDIR/homeapplication.cpp \
    $$STUBSDIR/stubbase.cpp

HEADERS += ut_screenlock.h \
    $$SCREENLOCKSRCDIR/screenlock.h \
    $$SRCDIR/homeapplication.h \
    $$UTILITYSRCDIR/closeeventeater.h
