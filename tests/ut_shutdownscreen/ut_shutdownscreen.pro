include(../common.pri)
TARGET = ut_shutdownscreen
INCLUDEPATH += $$SRCDIR $$NOTIFICATIONSRCDIR $$TOUCHSCREENSRCDIR $$UTILITYSRCDIR $$XTOOLSRCDIR $$QMSYSTEM2
QT += qml quick dbus

CONFIG += link_pkgconfig
PKGCONFIG += dsme_dbus_if thermalmanager_dbus_if usb_moded

# unit test and unit
SOURCES += \
    $$SRCDIR/shutdownscreen.cpp \
    $$NOTIFICATIONSRCDIR/lipsticknotification.cpp \
    $$STUBSDIR/stubbase.cpp \
    $$STUBSDIR/homewindow.cpp \
    $$STUBSDIR/homeapplication.cpp \
    $$QMSYSTEM2/qmsystemstate.cpp \
    $$QMSYSTEM2/qmipcinterface.cpp \
    ut_shutdownscreen.cpp

HEADERS += \
    $$SRCDIR/shutdownscreen.h \
    $$NOTIFICATIONSRCDIR/notificationmanager.h \
    $$NOTIFICATIONSRCDIR/lipsticknotification.h \
    $$UTILITYSRCDIR/closeeventeater.h \
    $$SRCDIR/homeapplication.h \
    $$SRCDIR/homewindow.h \
    $$QMSYSTEM2/qmsystemstate.h \
    $$QMSYSTEM2/qmsystemstate_p.h \
    $$QMSYSTEM2/qmipcinterface_p.h \
    ut_shutdownscreen.h
