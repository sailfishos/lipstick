include(../common.pri)
TARGET = ut_volumecontrol
INCLUDEPATH += $$VOLUMESRCDIR $$UTILITYSRCDIR
CONFIG += link_pkgconfig
PKGCONFIG += libresourceqt5
QT += dbus qml quick
QMAKE_CXXFLAGS += `pkg-config --cflags-only-I mlite5`

HEADERS += \
    ut_volumecontrol.h \
    $$VOLUMESRCDIR/volumecontrol.h \
    $$VOLUMESRCDIR/pulseaudiocontrol.h \
    $$UTILITYSRCDIR/closeeventeater.h \
    $$SRCDIR/homewindow.h \
    $$STUBSDIR/nemo-devicelock/devicelock.h \
    /usr/include/mlite5/mgconfitem.h \

SOURCES += \
    ut_volumecontrol.cpp \
    $$VOLUMESRCDIR/volumecontrol.cpp \
    $$STUBSDIR/stubbase.cpp \
    $$STUBSDIR/homewindow.cpp \
