SRCDIR = ../../src
NOTIFICATIONSRCDIR = $$SRCDIR/notifications
SCREENLOCKSRCDIR = $$SRCDIR/screenlock
TOUCHSCREENSRCDIR = $$SRCDIR/touchscreen
DEVICELOCKSRCDIR = $$SRCDIR/devicelock
UTILITYSRCDIR = $$SRCDIR/utilities
3RDPARTYSRCDIR = $$SRCDIR/3rdparty
VOLUMESRCDIR = $$SRCDIR/volume
COMPOSITORSRCDIR = $$SRCDIR/compositor
COMPONENTSSRCDIR = $$SRCDIR/components
QMSYSTEM2 = $$SRCDIR/qmsystem2
STUBSDIR = ../stubs
COMMONDIR = ../common
INCLUDEPATH += $$SRCDIR $$STUBSDIR $$PWD
DEPENDPATH = $$INCLUDEPATH
QT += testlib
TEMPLATE = app
DEFINES += UNIT_TEST
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x000000
CONFIG -= link_prl
CONFIG += link_pkgconfig
PKGCONFIG += mlite5

QMAKE_CXXFLAGS += \
    -Werror \
    -g \
    -std=c++0x \
    -fPIC \
    -fvisibility=hidden \
    -fvisibility-inlines-hidden

target.path = /opt/tests/lipstick-tests
INSTALLS += target
