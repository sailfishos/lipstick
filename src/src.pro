system(qdbusxml2cpp notifications/notificationmanager.xml -a notifications/notificationmanageradaptor -c NotificationManagerAdaptor -l NotificationManager -i notificationmanager.h)
system(qdbusxml2cpp screenlock/screenlock.xml -a screenlock/screenlockadaptor -c ScreenLockAdaptor -l ScreenLock -i screenlock.h)
system(qdbusxml2cpp shutdownscreen.xml -a shutdownscreenadaptor -c ShutdownScreenAdaptor -l ShutdownScreen -i shutdownscreen.h)
system(qdbusxml2cpp net.connman.vpn.Agent.xml -a connmanvpnagent -c ConnmanVpnAgentAdaptor -l VpnAgent -i vpnagent.h)
system(qdbusxml2cpp -c ConnmanVpnProxy -p connmanvpnproxy net.connman.vpn.xml -i qdbusxml2cpp_dbus_types.h)
system(qdbusxml2cpp -c ConnmanManagerProxy -p connmanmanagerproxy net.connman.manager.xml -i qdbusxml2cpp_dbus_types.h)
system(qdbusxml2cpp -c ConnmanServiceProxy -p connmanserviceproxy net.connman.service.xml -i qdbusxml2cpp_dbus_types.h)

TEMPLATE = lib
TARGET = lipstick-qt5

DEFINES += LIPSTICK_BUILD_LIBRARY
DEFINES += VERSION=\\\"$${VERSION}\\\"
DEFINES += MESA_EGL_NO_X11_HEADERS

CONFIG += qt wayland-scanner
INSTALLS = target ts_install engineering_english_install
target.path = $$[QT_INSTALL_LIBS]

QMAKE_STRIP = echo
OBJECTS_DIR = .obj
MOC_DIR = .moc

INCLUDEPATH += utilities touchscreen components xtools 3rdparty qmsystem2

include(compositor/compositor.pri)
include(compositor/alienmanager/alienmanager.pri)

PUBLICHEADERS += \
    utilities/qobjectlistmodel.h \
    utilities/closeeventeater.h \
    touchscreen/touchscreen.h \
    homeapplication.h \
    homewindow.h \
    lipstickglobal.h \
    lipsticksettings.h \
    lipstickdbus.h \
    lipstickqmlpath.h \
    components/launcheritem.h \
    components/launchermodel.h \
    components/launcherwatchermodel.h \
    components/launchermonitor.h \
    components/launcherdbus.h \
    components/launcherfoldermodel.h \
    notifications/notificationmanager.h \
    notifications/lipsticknotification.h \
    notifications/notificationlistmodel.h \
    notifications/notificationpreviewpresenter.h \
    usbmodeselector.h \
    shutdownscreen.h \
    qmsystem2/qmdisplaystate.h \
    qmsystem2/qmsystemstate.h \
    qmsystem2/qmthermal.h \
    qmsystem2/system_global.h \
    vpnagent.h \
    connectivitymonitor.h \
    connectionselector.h

INSTALLS += publicheaderfiles dbus_policy
publicheaderfiles.files = $$PUBLICHEADERS
publicheaderfiles.path = /usr/include/lipstick-qt5
dbus_policy.files += lipstick.conf
dbus_policy.path = /etc/dbus-1/system.d

HEADERS += \
    $$PUBLICHEADERS \
    3rdparty/synchronizelists.h \
    notifications/notificationmanageradaptor.h \
    notifications/categorydefinitionstore.h \
    notifications/batterynotifier.h \
    notifications/lowbatterynotifier.h \
    notifications/notificationfeedbackplayer.h \
    notifications/androidprioritystore.h \
    screenlock/screenlock.h \
    screenlock/screenlockadaptor.h \
    touchscreen/touchscreen_p.h \
    volume/volumecontrol.h \
    volume/pulseaudiocontrol.h \
    lipstickapi.h \
    lipstickqmlpath.h \
    shutdownscreenadaptor.h \
    screenshotservice.h \
    qdbusxml2cpp_dbus_types.h \
    connmanvpnagent.h \
    connmanvpnproxy.h \
    connmanmanagerproxy.h \
    connmanserviceproxy.h \
    notifications/thermalnotifier.h \
    qmsystem2/qmsystemstate_p.h \
    qmsystem2/qmdisplaystate_p.h \
    qmsystem2/qmipcinterface_p.h \
    qmsystem2/qmthermal_p.h \


SOURCES += \
    homeapplication.cpp \
    homewindow.cpp \
    lipsticksettings.cpp \
    lipstickqmlpath.cpp \
    utilities/qobjectlistmodel.cpp \
    utilities/closeeventeater.cpp \
    components/launcheritem.cpp \
    components/launchermodel.cpp \
    components/launcherwatchermodel.cpp \
    components/launchermonitor.cpp \
    components/launcherdbus.cpp \
    components/launcherfoldermodel.cpp \
    notifications/notificationmanager.cpp \
    notifications/notificationmanageradaptor.cpp \
    notifications/lipsticknotification.cpp \
    notifications/categorydefinitionstore.cpp \
    notifications/notificationlistmodel.cpp \
    notifications/notificationpreviewpresenter.cpp \
    notifications/batterynotifier.cpp \
    notifications/lowbatterynotifier.cpp \
    notifications/androidprioritystore.cpp \
    screenlock/screenlock.cpp \
    screenlock/screenlockadaptor.cpp \
    touchscreen/touchscreen.cpp \
    volume/volumecontrol.cpp \
    volume/pulseaudiocontrol.cpp \
    notifications/notificationfeedbackplayer.cpp \
    usbmodeselector.cpp \
    shutdownscreen.cpp \
    shutdownscreenadaptor.cpp \
    vpnagent.cpp \
    connectivitymonitor.cpp \
    connmanvpnagent.cpp \
    connmanvpnproxy.cpp \
    connmanmanagerproxy.cpp \
    connmanserviceproxy.cpp \
    connectionselector.cpp \
    lipstickapi.cpp \
    screenshotservice.cpp \
    notifications/thermalnotifier.cpp \
    qmsystem2/qmdisplaystate.cpp \
    qmsystem2/qmsystemstate.cpp \
    qmsystem2/qmthermal.cpp \
    qmsystem2/qmipcinterface.cpp \

CONFIG += link_pkgconfig mobility qt warn_on depend_includepath qmake_cache target_qt
CONFIG -= link_prl
PKGCONFIG += \
    contextkit-statefs \
    dbus-1 \
    dbus-glib-1 \
    dsme_dbus_if \
    keepalive \
    libresourceqt5 \
    libsystemd-daemon \
    mlite5 \
    mce \
    nemodevicelock \
    ngf-qt5 \
    Qt5SystemInfo \
    systemsettings \
    thermalmanager_dbus_if \
    usb-moded-qt5

LIBS += -lrt -lEGL

packagesExist(contentaction5) {
    message("Using contentaction to launch applications")
    PKGCONFIG += contentaction5
    DEFINES += HAVE_CONTENTACTION
} else {
    warning("contentaction doesn't exist; falling back to exec - this may not work so great")
}

packagesExist(contextkit-statefs) {
    PKGCONFIG += contextkit-statefs
    DEFINES += HAVE_CONTEXTSUBSCRIBER
} else {
    warning("Contextsubscriber not found")
}

QT += dbus xml qml quick sql gui gui-private sensors

QMAKE_CXXFLAGS += \
    -Werror \
    -g \
    -std=c++0x \
    -fPIC \
    -fvisibility=hidden \
    -fvisibility-inlines-hidden

QMAKE_LFLAGS += \
    -pie \
    -rdynamic

QMAKE_CLEAN += \
    *.gcov \
    ./.obj/*.gcno

CONFIG += create_pc create_prl
QMAKE_PKGCONFIG_NAME = lib$$TARGET
QMAKE_PKGCONFIG_VERSION = $$VERSION
QMAKE_PKGCONFIG_DESCRIPTION = Library for creating QML desktops
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_INCDIR = $$publicheaderfiles.path
QMAKE_PKGCONFIG_DESTDIR = pkgconfig

pkgconfig.files = $$TARGET.pc
pkgconfig.path = $$target.path/pkgconfig
INSTALLS += pkgconfig

# translations
TS_FILE = $$OUT_PWD/lipstick.ts
EE_QM = $$OUT_PWD/lipstick_eng_en.qm
ts.commands += lupdate $$PWD -ts $$TS_FILE
ts.CONFIG += no_check_exist
ts.output = $$TS_FILE
ts.input = .
ts_install.files = $$TS_FILE
ts_install.path = /usr/share/translations/source
ts_install.CONFIG += no_check_exist

# should add -markuntranslated "-" when proper translations are in place (or for testing)
engineering_english.commands += lrelease -idbased $$TS_FILE -qm $$EE_QM
engineering_english.CONFIG += no_check_exist
engineering_english.depends = ts
engineering_english.input = $$TS_FILE
engineering_english.output = $$EE_QM
engineering_english_install.path = /usr/share/translations
engineering_english_install.files = $$EE_QM
engineering_english_install.CONFIG += no_check_exist

QMAKE_EXTRA_TARGETS += ts engineering_english
PRE_TARGETDEPS += ts engineering_english

androidpriorities.files = androidnotificationpriorities
androidpriorities.path = /usr/share/lipstick/

INSTALLS += androidpriorities
OTHER_FILES += androidnotificationpriorities *.xml screenlock/screenlock.xml notifications/notificationmanager.xml

include(notificationcategories/notificationcategories.pri)
