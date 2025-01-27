include(../common.pri)
TARGET = ut_volumecontrol
INCLUDEPATH += $$VOLUMESRCDIR $$UTILITYSRCDIR $$3RDPARTYSRCDIR
CONFIG += link_pkgconfig
PKGCONFIG += dbus-1 libresourceqt5 glib-2.0
QT += dbus qml quick
QMAKE_CXXFLAGS += `pkg-config --cflags-only-I mlite5`

HEADERS += \
    ut_volumecontrol.h \
    $$3RDPARTYSRCDIR/dbus-gmain/dbus-gmain.h \
    $$VOLUMESRCDIR/volumecontrol.h \
    $$VOLUMESRCDIR/pulseaudiocontrol.h \
    $$UTILITYSRCDIR/closeeventeater.h \
    $$SRCDIR/homewindow.h \
    $$STUBSDIR/nemo-devicelock/devicelock.h \
    /usr/include/mlite5/mdconfitem.h \

SOURCES += \
    ut_volumecontrol.cpp \
    $$VOLUMESRCDIR/volumecontrol.cpp \
    $$STUBSDIR/stubbase.cpp \
    $$STUBSDIR/homewindow.cpp \
