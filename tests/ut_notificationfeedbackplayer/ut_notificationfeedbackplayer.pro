include(../common.pri)
TARGET = ut_notificationfeedbackplayer
INCLUDEPATH += $$NOTIFICATIONSRCDIR $$COMPOSITORSRCDIR $$TOUCHSCREENSRCDIR /usr/include/ngf-qt5 $$DEVICESTATE
CONFIG += link_pkgconfig
QT += dbus quick compositor

DEFINES += \
    LIPSTICK_UNIT_TEST_STUB

HEADERS += \
    ut_notificationfeedbackplayer.h \
    $$NOTIFICATIONSRCDIR/notificationfeedbackplayer.h \
    $$NOTIFICATIONSRCDIR/notificationmanager.h \
    $$NOTIFICATIONSRCDIR/lipsticknotification.h \
    $$COMPOSITORSRCDIR/lipstickcompositor.h \
    /usr/include/ngf-qt5/ngfclient.h

SOURCES += \
    ut_notificationfeedbackplayer.cpp \
    $$NOTIFICATIONSRCDIR/notificationfeedbackplayer.cpp \
    $$NOTIFICATIONSRCDIR/lipsticknotification.cpp \
    $$STUBSDIR/stubbase.cpp

