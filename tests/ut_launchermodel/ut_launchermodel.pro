include(../common.pri)
TARGET = ut_launchermodel

INCLUDEPATH += $$COMPONENTSSRCDIR
INCLUDEPATH += $$UTILITYSRCDIR
INCLUDEPATH += $$3RDPARTYSRCDIR

QMAKE_CXXFLAGS += `pkg-config --cflags-only-I mlite5`

QT += dbus qml

packagesExist(contentaction5) {
    PKGCONFIG += contentaction5
    DEFINES += HAVE_CONTENTACTION
} else {
    PKGCONFIG += \
        gio-2.0
}

SOURCES += \
    ut_launchermodel.cpp \
    $$COMPONENTSSRCDIR/launchermodel.cpp \
    $$COMPONENTSSRCDIR/launchermonitor.cpp \
    $$COMPONENTSSRCDIR/launcheritem.cpp \
    $$COMPONENTSSRCDIR/launcherdbus.cpp \
    $$STUBSDIR/stubbase.cpp \
    $$UTILITYSRCDIR/qobjectlistmodel.cpp \
    $$SRCDIR/logging.cpp \

HEADERS += \
    ut_launchermodel.h \
    $$COMPONENTSSRCDIR/launchermodel.h \
    $$COMPONENTSSRCDIR/launchermonitor.h \
    $$COMPONENTSSRCDIR/launcheritem.h \
    $$COMPONENTSSRCDIR/launcherdbus.h \
    $$UTILITYSRCDIR/qobjectlistmodel.h \
    $$3RDPARTYSRCDIR/synchronizelists.h \
    $$SRCDIR/logging.h \
    /usr/include/mlite5/mdesktopentry.h \

