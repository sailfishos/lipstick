/***************************************************************************
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPL included in the packaging
** of this file.
**
****************************************************************************/

#include <QSocketNotifier>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QScreen>
#include <QTimer>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <QIcon>
#include <QTranslator>
#include <QDebug>
#include <QEvent>
#include <systemd/sd-daemon.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <deviceinfo.h>

#include "notifications/notificationmanager.h"
#include "notifications/notificationpreviewpresenter.h"
#include "notifications/batterynotifier.h"
#include "notifications/thermalnotifier.h"
#include "screenlock/screenlock.h"
#include "screenlock/screenlockadaptor.h"
#include "touchscreen/touchscreen.h"
#include "lipsticksettings.h"
#include "homeapplication.h"
#include "homewindow.h"
#include "compositor/lipstickcompositor.h"
#include "compositor/lipstickcompositorwindow.h"
#include "lipstickdbus.h"

#include "volume/volumecontrol.h"
#include "usbmodeselector.h"
#include "shutdownscreen.h"
#include "shutdownscreenadaptor.h"
#include "screenshotservice.h"
#include "vpnagent.h"
#include "connmanvpnagent.h"
#include "connmanvpnproxy.h"

#include <nemo-devicelock/devicelock.h>

void HomeApplication::quitSignalHandler(int)
{
    uint64_t a = 1;
    ssize_t unused = ::write(s_quitSignalFd, &a, sizeof(a));
    Q_UNUSED(unused);
}

int HomeApplication::s_quitSignalFd = -1;

static void registerDBusObject(QDBusConnection &bus, const char *path, QObject *object)
{
    if (!bus.registerObject(path, object)) {
        qWarning("Unable to register object at path %s: %s", path, bus.lastError().message().toUtf8().constData());
    }
}

HomeApplication::HomeApplication(int &argc, char **argv, const QString &qmlPath)
    : QGuiApplication(argc, argv)
    , m_quitSignalNotifier(0)
    , m_mainWindowInstance(0)
    , m_qmlPath(qmlPath)
    , m_homeReadySent(false)
    , m_connmanVpn(0)
    , m_online(false)
{
    setUpSignalHandlers();

    QTranslator *engineeringEnglish = new QTranslator(this);
    engineeringEnglish->load("lipstick_eng_en", "/usr/share/translations");
    installTranslator(engineeringEnglish);
    QTranslator *translator = new QTranslator(this);
    translator->load(QLocale(), "lipstick", "-", "/usr/share/translations");
    installTranslator(translator);

    setApplicationName("Lipstick");
    setApplicationVersion(VERSION);

    // Initialize the QML engine
    m_qmlEngine = new QQmlEngine(this);

    // Export screen size / geometry as dconf keys
    LipstickSettings::instance()->exportScreenProperties();

    // Create touch screen and register for the screen lock.
    m_touchScreen = new TouchScreen(this);
    connect(m_touchScreen, &TouchScreen::displayStateChanged, this, &HomeApplication::displayStateChanged);

    m_screenLock = new ScreenLock(m_touchScreen, this);
    LipstickSettings::instance()->setScreenLock(m_screenLock);
    new ScreenLockAdaptor(m_screenLock);

    NemoDeviceLock::DeviceLock *deviceLock = new NemoDeviceLock::DeviceLock(this);

    // Initialize the notification manager
    NotificationManager::instance();
    new NotificationPreviewPresenter(m_screenLock, deviceLock, this);

    m_volumeControl = new VolumeControl(true, this);

    DeviceInfo deviceInfo;
    if (deviceInfo.hasFeature(DeviceInfo::FeatureBattery)) {
        new BatteryNotifier(this);
    }

    new ThermalNotifier(this);
    m_usbModeSelector = new USBModeSelector(deviceLock, this);
    m_shutdownScreen = new ShutdownScreen(this);
    new ShutdownScreenAdaptor(m_shutdownScreen);

    // MCE and usb-moded expect services to be registered on the system bus
    QDBusConnection systemBus = QDBusConnection::systemBus();
    registerDBusObject(systemBus, LIPSTICK_DBUS_SCREENLOCK_PATH, m_screenLock);
    registerDBusObject(systemBus, LIPSTICK_DBUS_SHUTDOWN_PATH, m_shutdownScreen);
    if (!systemBus.registerService(LIPSTICK_DBUS_SERVICE_NAME)) {
        qWarning("Unable to register D-Bus service %s: %s", LIPSTICK_DBUS_SERVICE_NAME,
                 systemBus.lastError().message().toUtf8().constData());
    }

    // Respond to requests for VPN user input
    m_vpnAgent = new VpnAgent(this);
    new ConnmanVpnAgentAdaptor(m_vpnAgent);

    registerDBusObject(systemBus, LIPSTICK_DBUS_VPNAGENT_PATH, m_vpnAgent);

    auto registerVpnAgent = [this]() {
        if (!m_connmanVpn) {
            if (QDBusConnection::systemBus().interface()->isServiceRegistered(LIPSTICK_DBUS_CONNMAN_VPN_SERVICE)) {
                m_connmanVpn = new ConnmanVpnProxy(LIPSTICK_DBUS_CONNMAN_VPN_SERVICE,
                                                   "/", QDBusConnection::systemBus());
                m_connmanVpn->RegisterAgent(QDBusObjectPath(LIPSTICK_DBUS_VPNAGENT_PATH));
            }
        }
    };
    auto unregisterVpnAgent = [this]() {
        delete m_connmanVpn;
        m_connmanVpn = 0;
    };

    QDBusServiceWatcher *connmanVpnWatcher
            = new QDBusServiceWatcher(LIPSTICK_DBUS_CONNMAN_VPN_SERVICE, systemBus,
                                      QDBusServiceWatcher::WatchForRegistration
                                      | QDBusServiceWatcher::WatchForUnregistration,
                                      this);
    connect(connmanVpnWatcher, &QDBusServiceWatcher::serviceRegistered,
            this, [registerVpnAgent](const QString &){ registerVpnAgent(); });
    connect(connmanVpnWatcher, &QDBusServiceWatcher::serviceUnregistered,
            this, [unregisterVpnAgent](const QString &){ unregisterVpnAgent(); });

    registerVpnAgent();

    // Setting up the context and engine things
    m_qmlEngine->rootContext()->setContextProperty("initialSize",
                                                   QGuiApplication::primaryScreen()
                                                   ? QGuiApplication::primaryScreen()->size()
                                                   : QSize());
    m_qmlEngine->rootContext()->setContextProperty("lipstickSettings", LipstickSettings::instance());
    m_qmlEngine->rootContext()->setContextProperty("LipstickSettings", LipstickSettings::instance());
    m_qmlEngine->rootContext()->setContextProperty("volumeControl", m_volumeControl);
    m_qmlEngine->rootContext()->setContextProperty("usbModeSelector", m_usbModeSelector);

    connect(this, SIGNAL(homeReady()), this, SLOT(sendStartupNotifications()));
}

HomeApplication::~HomeApplication()
{
    if (m_connmanVpn) {
        m_connmanVpn->UnregisterAgent(QDBusObjectPath(LIPSTICK_DBUS_VPNAGENT_PATH));
        delete m_connmanVpn;
    }

    emit aboutToDestroy();

    delete m_mainWindowInstance;
    m_mainWindowInstance = nullptr;

    delete m_qmlEngine;
    m_qmlEngine = nullptr;
}

HomeApplication *HomeApplication::instance()
{
    return qobject_cast<HomeApplication *>(qApp);
}

void HomeApplication::setUpSignalHandlers()
{
    s_quitSignalFd = ::eventfd(0, 0);
    if (s_quitSignalFd == -1)
        qFatal("Failed to create eventfd object for signal handling");

    m_quitSignalNotifier = new QSocketNotifier(s_quitSignalFd, QSocketNotifier::Read, this);
    connect(m_quitSignalNotifier, &QSocketNotifier::activated, this, [this]() {
        uint64_t tmp;
        ssize_t unused = ::read(s_quitSignalFd, &tmp, sizeof(tmp));
        Q_UNUSED(unused);

        quit();
    });

    struct sigaction action;
    action.sa_handler = quitSignalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_flags |= SA_RESTART;

    if (sigaction(SIGINT, &action, nullptr))
        qFatal("Failed to set up SIGINT handling");
    if (sigaction(SIGTERM, &action, nullptr))
        qFatal("Failed to set up SIGTERM handling");
}

void HomeApplication::sendHomeReadySignalIfNotAlreadySent()
{
    if (!m_homeReadySent) {
        m_homeReadySent = true;
        disconnect(LipstickCompositor::instance(), SIGNAL(frameSwapped()),
                   this, SLOT(sendHomeReadySignalIfNotAlreadySent()));

        emit homeReady();
    }
}

void HomeApplication::sendStartupNotifications()
{
    /* Let systemd know that we are initialized */
    if (arguments().indexOf("--systemd") >= 0) {
        sd_notify(0, "READY=1");
    }

    /* Let timed know that the UI is up */
    QDBusConnection::systemBus().call(QDBusMessage::createSignal("/com/nokia/startup/signal",
                                                                 "com.nokia.startup.signal",
                                                                 "desktop_visible"), QDBus::NoBlock);
}

bool HomeApplication::homeActive() const
{
    LipstickCompositor *c = LipstickCompositor::instance();
    return c ? c->homeActive() : (QGuiApplication::focusWindow() != 0);
}

TouchScreen *HomeApplication::touchScreen() const
{
    return m_touchScreen;
}

TouchScreen::DisplayState HomeApplication::displayState()
{
    Q_ASSERT(m_touchScreen);
    return m_touchScreen->currentDisplayState();
}

void HomeApplication::setDisplayOff()
{
    Q_ASSERT(m_touchScreen);
    m_touchScreen->setDisplayOff();
}

bool HomeApplication::event(QEvent *e)
{
    bool rv = QGuiApplication::event(e);
    if (LipstickCompositor::instance() == 0
            && (e->type() == QEvent::ApplicationActivate
                || e->type() == QEvent::ApplicationDeactivate)) {
        emit homeActiveChanged();
    }
    return rv;
}

const QString &HomeApplication::qmlPath() const
{
    return m_qmlPath;
}

void HomeApplication::setQmlPath(const QString &path)
{
    m_qmlPath = path;

    if (m_mainWindowInstance) {
        m_mainWindowInstance->setSource(path);
        if (m_mainWindowInstance->hasErrors()) {
            qWarning() << "HomeApplication: Errors while loading" << path;
            qWarning() << m_mainWindowInstance->errors();
        }
    }
}

const QString &HomeApplication::compositorPath() const
{
    return m_compositorPath;
}

void HomeApplication::setCompositorPath(const QString &path)
{
    if (path.isEmpty()) {
        qWarning() << "HomeApplication: Invalid empty compositor path";
        return;
    }

    if (!m_compositorPath.isEmpty()) {
        qWarning() << "HomeApplication: Compositor already set";
        return;
    }

    m_compositorPath = path;
    QQmlComponent component(m_qmlEngine, QUrl(path));
    if (component.isError()) {
        qWarning() << "HomeApplication: Errors while loading compositor from" << path;
        qWarning() << component.errors();
        return;
    } 

    QObject *compositor = component.beginCreate(m_qmlEngine->rootContext());
    if (compositor) {
        compositor->setParent(this);
        if (LipstickCompositor::instance()) {
            LipstickCompositor::instance()->setGeometry(QRect(QPoint(0, 0),
                                                              QGuiApplication::primaryScreen()->size()));
            connect(m_usbModeSelector, SIGNAL(showUnlockScreen()),
                    LipstickCompositor::instance(), SIGNAL(showUnlockScreen()));
        }

        component.completeCreate();

        if (!m_qmlEngine->incubationController() && LipstickCompositor::instance()) {
            // install default incubation controller
            m_qmlEngine->setIncubationController(LipstickCompositor::instance()->incubationController());
        }
    } else {
        qWarning() << "HomeApplication: Error creating compositor from" << path;
        qWarning() << component.errors();
    }
}

HomeWindow *HomeApplication::mainWindowInstance()
{
    if (m_mainWindowInstance)
        return m_mainWindowInstance;

    m_mainWindowInstance = new HomeWindow();
    m_mainWindowInstance->setGeometry(QRect(QPoint(), QGuiApplication::primaryScreen()->size()));
    m_mainWindowInstance->setWindowTitle("Home");
    QObject::connect(m_mainWindowInstance->engine(), SIGNAL(quit()),
                     qApp, SLOT(quit()));
    QObject::connect(m_mainWindowInstance, SIGNAL(visibleChanged(bool)),
                     this, SLOT(connectFrameSwappedSignal(bool)));

    // Setting the source, if present
    if (!m_qmlPath.isEmpty())
        m_mainWindowInstance->setSource(m_qmlPath);

    return m_mainWindowInstance;
}

QQmlEngine *HomeApplication::engine() const
{
    return m_qmlEngine;
}

void HomeApplication::connectFrameSwappedSignal(bool mainWindowVisible)
{
    if (!m_homeReadySent && mainWindowVisible) {
        connect(LipstickCompositor::instance(), SIGNAL(frameSwapped()),
                this, SLOT(sendHomeReadySignalIfNotAlreadySent()));
    }
}

bool HomeApplication::takeScreenshot(const QString &path)
{
    if (ScreenshotResult *result = ScreenshotService::saveScreenshot(path)) {
        result->waitForFinished();

        return result->status() == ScreenshotResult::Finished;
    } else {
        return false;
    }
}
