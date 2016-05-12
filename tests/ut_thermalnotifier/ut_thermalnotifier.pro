include(../common.pri)
TARGET = ut_thermalnotifier
CONFIG += link_pkgconfig
INCLUDEPATH += $$SRCDIR $$NOTIFICATIONSRCDIR $$TOUCHSCREENSRCDIR $$UTILITYSRCDIR $$XTOOLSRCDIR $$QMSYSTEM2
QT += qml quick dbus

# unit test and unit
SOURCES += \
    $$NOTIFICATIONSRCDIR/thermalnotifier.cpp \
    $$NOTIFICATIONSRCDIR/lipsticknotification.cpp \
    $$STUBSDIR/stubbase.cpp \
    $$STUBSDIR/homeapplication.cpp \
    ut_thermalnotifier.cpp

HEADERS += \
    $$NOTIFICATIONSRCDIR/thermalnotifier.h \
    $$NOTIFICATIONSRCDIR/notificationmanager.h \
    $$NOTIFICATIONSRCDIR/lipsticknotification.h \
    $$SRCDIR/homeapplication.h \
    $$QMSYSTEM2/qmthermal.h \
    $$QMSYSTEM2/qmdisplaystate.h \
    ut_thermalnotifier.h
