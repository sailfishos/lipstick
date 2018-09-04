include(../common.pri)
TARGET = ut_usbmodeselector
USBMODEDQTINCLUDEDIR = $$system(pkg-config --cflags-only-I usb-moded-qt5 | sed s/^-I//g)
INCLUDEPATH += $$SRCDIR $$NOTIFICATIONSRCDIR $$UTILITYSRCDIR $$USBMODEDQTINCLUDEDIR $$QMSYSTEM2
PKGCONFIG += usb_moded
QT += qml quick dbus

# unit test and unit
SOURCES += \
    $$SRCDIR/lipstickwindow.cpp \
    $$SRCDIR/usbmodeselector.cpp \
    $$NOTIFICATIONSRCDIR/lipsticknotification.cpp \
    $$STUBSDIR/stubbase.cpp \
    ut_usbmodeselector.cpp \
    qusbmoded_stub.cpp

HEADERS += \
    $$SRCDIR/lipstickwindow.h \
    $$SRCDIR/usbmodeselector.h \
    $$NOTIFICATIONSRCDIR/notificationmanager.h \
    $$NOTIFICATIONSRCDIR/lipsticknotification.h \
    $$UTILITYSRCDIR/closeeventeater.h \
    $$USBMODEDQTINCLUDEDIR/qusbmoded.h \
    $$USBMODEDQTINCLUDEDIR/qusbmode.h \
    $$STUBSDIR/nemo-devicelock/devicelock.h \
    ut_usbmodeselector.h \
    $$SRCDIR/homewindow.h \
