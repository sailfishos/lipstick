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
DEVICESTATE = $$SRCDIR/devicestate
STUBSDIR = ../stubs
COMMONDIR = ../common
INCLUDEPATH += $$SRCDIR $$STUBSDIR $$PWD
DEPENDPATH = $$INCLUDEPATH
QT += testlib
TEMPLATE = app
DEFINES += UNIT_TEST

CONFIG -= link_prl
CONFIG += link_pkgconfig
PKGCONFIG += mlite5 systemsettings

target.path = /opt/tests/lipstick-tests
INSTALLS += target
