include(../common.pri)
TARGET = ut_shutdownscreen
INCLUDEPATH += $$SRCDIR $$NOTIFICATIONSRCDIR $$TOUCHSCREENSRCDIR $$UTILITYSRCDIR $$XTOOLSRCDIR $$DEVICESTATE
QT += qml quick dbus

CONFIG += link_pkgconfig
PKGCONFIG += dsme_dbus_if thermalmanager_dbus_if usb_moded sailfishusermanager

# unit test and unit
SOURCES += \
    $$SRCDIR/shutdownscreen.cpp \
    $$NOTIFICATIONSRCDIR/lipsticknotification.cpp \
    $$NOTIFICATIONSRCDIR/thermalnotifier.cpp \
    $$STUBSDIR/stubbase.cpp \
    $$STUBSDIR/homewindow.cpp \
    $$STUBSDIR/homeapplication.cpp \
    $$DEVICESTATE/devicestate.cpp \
    $$DEVICESTATE/displaystate.cpp \
    ut_shutdownscreen.cpp

HEADERS += \
    $$SRCDIR/shutdownscreen.h \
    $$NOTIFICATIONSRCDIR/notificationmanager.h \
    $$NOTIFICATIONSRCDIR/lipsticknotification.h \
    $$NOTIFICATIONSRCDIR/thermalnotifier.h \
    $$UTILITYSRCDIR/closeeventeater.h \
    $$SRCDIR/homeapplication.h \
    $$SRCDIR/homewindow.h \
    $$DEVICESTATE/devicestate.h \
    $$DEVICESTATE/devicestate_p.h \
    $$DEVICESTATE/displaystate.h \
    $$DEVICESTATE/displaystate_p.h \
    $$STUBSDIR/nemo-devicelock/devicelock.h \
    ut_shutdownscreen.h
