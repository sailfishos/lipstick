TEMPLATE = lib
TARGET = lipstickplugin
VERSION = 0.1

CONFIG += qt plugin link_pkgconfig
QT += core gui qml quick compositor dbus
PKGCONFIG += mlite5 dsme_dbus_if thermalmanager_dbus_if usb-moded-qt5 systemsettings

INSTALLS = target qmldirfile
qmldirfile.files = qmldir
qmldirfile.path = $$[QT_INSTALL_QML]/org/nemomobile/lipstick
target.path = $$[QT_INSTALL_QML]/org/nemomobile/lipstick

DEPENDPATH += ../src
INCLUDEPATH += ../src \
    ../src/utilities \
    ../src/xtools \
    ../src/compositor \
    ../src/devicestate \
    ../src/touchscreen

LIBS += -L../src -llipstick-qt5

HEADERS += \
    lipstickplugin.h

SOURCES += \
    lipstickplugin.cpp

OTHER_FILES += \
    qmldir
