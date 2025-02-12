/***************************************************************************
**
** Copyright (c) 2013 - 2023 Jolla Ltd.
** Copyright (c) 2019 - 2020 Open Mobile Platform LLC.
**
** This file is part of lipstick.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPL included in the packaging
** of this file.
**
****************************************************************************/


#include <QWaylandInputDevice>
#include <QDesktopServices>
#include <QtSensors/QOrientationSensor>
#include <QClipboard>
#include <QMimeData>
#include <QtGui/qpa/qplatformnativeinterface.h>
#include <qpa/qwindowsysteminterface.h>
#include <private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>

#include <qmcenameowner.h>
#include <dbus/dbus-protocol.h>
#include <sys/types.h>
#include <systemd/sd-bus.h>
#include <systemd/sd-login.h>
#include <unistd.h>

#include "homeapplication.h"
#include "touchscreen/touchscreen.h"
#include "windowmodel.h"
#include "lipstickcompositorprocwindow.h"
#include "lipstickcompositor.h"
#include "lipstickcompositoradaptor.h"
#include "fileserviceadaptor.h"
#include "lipstickkeymap.h"
#include "lipsticksettings.h"
#include "lipstickrecorder.h"
#include "alienmanager/alienmanager.h"
#include "logging.h"

namespace {
bool debuggingCompositorHandover()
{
    static int debugging = -1;
    if (debugging < 0) {
        const QByteArray raw(qgetenv("DEBUG_COMPOSITOR_HANDOVER"));
        const QString env(QString::fromLocal8Bit(raw));
        debugging = env.startsWith('y');
    }
    return debugging > 0;
}
}

LipstickCompositor *LipstickCompositor::m_instance = 0;

LipstickCompositor::LipstickCompositor()    
    : QWaylandQuickCompositor(nullptr,
                              static_cast<QWaylandCompositor::ExtensionFlags>(
                                  QWaylandCompositor::DefaultExtensions & ~QWaylandCompositor::QtKeyExtension))
    , m_output(this, this, QString(), QString())
    , m_totalWindowCount(0)
    , m_nextWindowId(1)
    , m_homeActive(true)
    , m_topmostWindowId(0)
    , m_topmostWindowProcessId(0)
    , m_topmostWindowOrientation(Qt::PrimaryOrientation)
    , m_screenOrientation(Qt::PrimaryOrientation)
    , m_sensorOrientation(Qt::PrimaryOrientation)
    , m_retainedSelection(0)
    , m_updatesEnabled(true)
    , m_completed(false)
    , m_synthesizeBackEvent(true)
    , m_onUpdatesDisabledUnfocusedWindowId(0)
    , m_keymap(0)
    , m_fakeRepaintTimerId(0)
    , m_queuedSetUpdatesEnabledCalls()
    , m_mceNameOwner(new QMceNameOwner(this))
    , m_sessionActivationTries(0)
{
    QGuiApplication::platformNativeInterface()->nativeResourceForIntegration("DisplayOn");
    if (debuggingCompositorHandover()) {
        setColor(Qt::magenta);
        showFullScreen();
    } else {
        setColor(Qt::black);
    }

    setRetainedSelectionEnabled(true);
    addDefaultShell();

    if (m_instance)
        qFatal("LipstickCompositor: Only one compositor instance per process is supported");
    m_instance = this;

    m_orientationLock = new MDConfItem("/lipstick/orientationLock", this);
    connect(m_orientationLock, SIGNAL(valueChanged()), SIGNAL(orientationLockChanged()));

    connect(this, SIGNAL(visibleChanged(bool)), this, SLOT(onVisibleChanged(bool)));
    QObject::connect(this, SIGNAL(afterRendering()), this, SLOT(windowSwapped()));
    QObject::connect(HomeApplication::instance(), SIGNAL(aboutToDestroy()), this, SLOT(homeApplicationAboutToDestroy()));
    connect(this, &QQuickWindow::afterRendering, this, &LipstickCompositor::readContent, Qt::DirectConnection);

    m_orientationSensor = new QOrientationSensor(this);
    QObject::connect(m_orientationSensor, SIGNAL(readingChanged()), this, SLOT(setScreenOrientationFromSensor()));
    if (!m_orientationSensor->connectToBackend()) {
        qWarning() << "Could not connect to the orientation sensor backend";
    } else {
        if (!m_orientationSensor->start())
            qWarning() << "Could not start the orientation sensor";
    }
    emit HomeApplication::instance()->homeActiveChanged();

    QDesktopServices::setUrlHandler("http", this, "openUrl");
    QDesktopServices::setUrlHandler("https", this, "openUrl");
    QDesktopServices::setUrlHandler("mailto", this, "openUrl");

    connect(QGuiApplication::clipboard(), SIGNAL(dataChanged()), SLOT(clipboardDataChanged()));

    m_recorder = new LipstickRecorderManager;
    addGlobalInterface(m_recorder);
    addGlobalInterface(new AlienManagerGlobal);

    QObject::connect(m_mceNameOwner, &QMceNameOwner::validChanged,
                     this, &LipstickCompositor::processQueuedSetUpdatesEnabledCalls);
    QObject::connect(m_mceNameOwner, &QMceNameOwner::nameOwnerChanged,
                     this, &LipstickCompositor::processQueuedSetUpdatesEnabledCalls);

    if (debuggingCompositorHandover()) {
        // Leave window visible and display on
    } else {
        setUpdatesEnabledNow(false);
    }
    QTimer::singleShot(0, this, SLOT(initialize()));

    setClientFullScreenHint(true);
}

static inline bool displayStateIsDimmed(TouchScreen::DisplayState state)
{
    return state == TouchScreen::DisplayDimmed;
}

static bool displayStateIsOn(TouchScreen::DisplayState state)
{
    return state == TouchScreen::DisplayOn || state == TouchScreen::DisplayDimmed;
}

LipstickCompositor::~LipstickCompositor()
{
    // ~QWindow can a call into onVisibleChanged and QWaylandCompositor after we
    // are destroyed, so disconnect it.
    disconnect(this, SIGNAL(visibleChanged(bool)), this, SLOT(onVisibleChanged(bool)));

    m_instance = nullptr;
}

LipstickCompositor *LipstickCompositor::instance()
{
    return m_instance;
}

void LipstickCompositor::homeApplicationAboutToDestroy()
{
    hide();
    releaseResources();

    // When destroying LipstickCompositor ~QQuickWindow() is called after
    // ~QWaylandQuickCompositor(), so changes to the items in the window may end
    // up calling code such as LipstickCompositorWindow::handleTouchCancel(),
    // which will try to use the compositor, at that point not usable anymore.
    // So delete all the windows here.
    foreach (LipstickCompositorWindow *w, m_windows) {
        delete w;
    }

    QWaylandQuickCompositor::cleanupGraphicsResources();

    m_instance = 0;
    delete this;
}

void LipstickCompositor::classBegin()
{
}

void LipstickCompositor::onVisibleChanged(bool visible)
{
    if (!visible) {
        sendFrameCallbacks(surfaces());
    }
}

void LipstickCompositor::componentComplete()
{
    QScreen * const screen = QGuiApplication::primaryScreen();

    m_output.setGeometry(QRect(QPoint(0, 0), screen->size()));
    m_output.setPhysicalSize(screen->physicalSize().toSize());
}

void LipstickCompositor::surfaceCreated(QWaylandSurface *surface)
{
    connect(surface, SIGNAL(mapped()), this, SLOT(surfaceMapped()));
    connect(surface, SIGNAL(unmapped()), this, SLOT(surfaceUnmapped()));
    connect(surface, SIGNAL(sizeChanged()), this, SLOT(surfaceSizeChanged()));
    connect(surface, SIGNAL(titleChanged()), this, SLOT(surfaceTitleChanged()));
    connect(surface, SIGNAL(windowPropertyChanged(QString,QVariant)), this, SLOT(windowPropertyChanged(QString)));
    connect(surface, SIGNAL(raiseRequested()), this, SLOT(surfaceRaised()));
    connect(surface, SIGNAL(lowerRequested()), this, SLOT(surfaceLowered()));
    connect(surface, SIGNAL(damaged(QRegion)), this, SLOT(surfaceDamaged(QRegion)));
    connect(surface, &QWaylandSurface::redraw, this, &LipstickCompositor::surfaceCommitted);
}

bool LipstickCompositor::openUrl(WaylandClient *client, const QUrl &url)
{
    Q_UNUSED(client)
    return openUrl(url);
}

bool LipstickCompositor::openUrl(const QUrl &url)
{
    openUrlRequested(url);

    return true;
}

void LipstickCompositor::retainedSelectionReceived(QMimeData *mimeData)
{
    if (!m_retainedSelection)
        m_retainedSelection = new QMimeData;

    // Make a copy to allow QClipboard to take ownership of our data
    m_retainedSelection->clear();
    foreach (const QString &format, mimeData->formats())
        m_retainedSelection->setData(format, mimeData->data(format));

    QGuiApplication::clipboard()->setMimeData(m_retainedSelection.data());
}

int LipstickCompositor::windowCount() const
{
    return m_mappedSurfaces.count();
}

int LipstickCompositor::ghostWindowCount() const
{
    return m_totalWindowCount - windowCount();
}

bool LipstickCompositor::homeActive() const
{
    return m_homeActive;
}

void LipstickCompositor::setHomeActive(bool a)
{
    if (a == m_homeActive)
        return;

    m_homeActive = a;

    emit homeActiveChanged();
    emit HomeApplication::instance()->homeActiveChanged();
}

bool LipstickCompositor::debug() const
{
    static enum { Yes, No, Unknown } status = Unknown;
    if (status == Unknown) {
        QByteArray v = qgetenv("LIPSTICK_COMPOSITOR_DEBUG");
        bool value = !v.isEmpty() && v != "0" && v != "false";
        if (value) status = Yes;
        else status = No;
    }
    return status == Yes;
}

QObject *LipstickCompositor::windowForId(int id) const
{
    LipstickCompositorWindow *window = m_windows.value(id, 0);
    return window;
}

void LipstickCompositor::closeClientForWindowId(int id)
{
    LipstickCompositorWindow *window = m_windows.value(id, 0);
    if (window && window->surface())
        destroyClientForSurface(window->surface());
}

QWaylandSurface *LipstickCompositor::surfaceForId(int id) const
{
    LipstickCompositorWindow *window = m_windows.value(id, 0);
    return window ? window->surface() : 0;
}

bool LipstickCompositor::completed()
{
    return m_completed;
}

bool LipstickCompositor::synthesizeBackEvent() const
{
    return m_synthesizeBackEvent;
}

void LipstickCompositor::setSynthesizeBackEvent(bool enable)
{
    if (enable != m_synthesizeBackEvent) {
        m_synthesizeBackEvent = enable;
        emit synthesizeBackEventChanged();
    }
}

int LipstickCompositor::windowIdForLink(int siblingId, uint link) const
{
    if (LipstickCompositorWindow *sibling = m_windows.value(siblingId)) {
        for (LipstickCompositorWindow *window : m_windows) {
            QWaylandSurface *windowSurface  = window->surface();

            if (windowSurface
                    && sibling->processId() == window->processId()
                    && windowSurface->windowProperties().value("WINID", uint(0)).toUInt() == link) {
                return window->windowId();
            }
        }
    }

    return 0;
}

void LipstickCompositor::clearKeyboardFocus()
{
    defaultInputDevice()->setKeyboardFocus(0);
}

void LipstickCompositor::setDisplayOff()
{
    HomeApplication::instance()->setDisplayOff();
}

void LipstickCompositor::surfaceDamaged(const QRegion &)
{
    if (!isVisible()) {
        // If the compositor is not visible, do not throttle.
        // make it conditional to QT_WAYLAND_COMPOSITOR_NO_THROTTLE?
        sendFrameCallbacks(surfaces());
    }
}

QObject *LipstickCompositor::clipboard() const
{
    return QGuiApplication::clipboard();
}

void LipstickCompositor::setTopmostWindowId(int id)
{
    if (id != m_topmostWindowId) {
        m_topmostWindowId = id;
        emit topmostWindowIdChanged();

        LipstickCompositorWindow *window = m_windows.value(id);

        int pid = window ? window->processId() : -1;

        if (m_topmostWindowProcessId != pid) {
            m_topmostWindowProcessId = pid;
            emit privateTopmostWindowProcessIdChanged(m_topmostWindowProcessId);
        }

        QString applicationId = window && !window->policyApplicationId().isEmpty()
                                ? window->policyApplicationId() : "none";

        if (m_topmostWindowPolicyApplicationId != applicationId) {
            m_topmostWindowPolicyApplicationId = applicationId;
            emit privateTopmostWindowPolicyApplicationIdChanged(m_topmostWindowPolicyApplicationId);
        }
    }
}

QWaylandSurfaceView *LipstickCompositor::createView(QWaylandSurface *surface)
{
    QVariantMap properties = surface->windowProperties();
    QString category = properties.value("CATEGORY").toString();

    int id = m_nextWindowId++;
    LipstickCompositorWindow *item = new LipstickCompositorWindow(id, category,
                                                                  static_cast<QWaylandQuickSurface *>(surface));
    item->setParent(this);
    QObject::connect(item, SIGNAL(destroyed(QObject*)), this, SLOT(windowDestroyed()));
    m_windows.insert(item->windowId(), item);
    return item;
}

static LipstickCompositorWindow *surfaceWindow(QWaylandSurface *surface)
{
    return surface->views().isEmpty() ? nullptr
                                      : static_cast<LipstickCompositorWindow *>(surface->views().first());
}

void LipstickCompositor::activateLogindSession()
{
    m_sessionActivationTries++;

    if (m_logindSession.isEmpty()) {
        /* Find the current session based on uid */
        uid_t uid = getuid();
        char **sessions = NULL;
        uid_t *uids = NULL;
        uint count = 0;
        if (sd_seat_get_sessions("seat0", &sessions, &uids, &count) > 0) {
            for (uint i = 0; i < count; ++i) {
                if (uids[i] == uid) {
                    m_logindSession = sessions[i];
                    break;
                }
            }
            for (char **s = sessions; *s ; ++s)
                free(*s);
        }
        free(sessions);
        free(uids);

        if (m_logindSession.isEmpty()) {
            qCWarning(lcLipstickCoreLog) << "Could not read session id, could not activate session";
            return;
        }
    }

    if (sd_session_is_active(m_logindSession.toUtf8()) > 0) {
        qCInfo(lcLipstickCoreLog) << "Session" << m_logindSession << "successfully activated";
        return;
    }

    if (m_sessionActivationTries > 10) {
        qCWarning(lcLipstickCoreLog) << "Could not activate session, giving up";
        return;
    }

    qCDebug(lcLipstickCoreLog) << "Activating session on seat0";

    QDBusMessage method = QDBusMessage::createMethodCall(
                QStringLiteral("org.freedesktop.login1"),
                QStringLiteral("/org/freedesktop/login1"),
                QStringLiteral("org.freedesktop.login1.Manager"),
                QStringLiteral("ActivateSession"));
    method.setArguments({ m_logindSession });

    QDBusPendingCall call = QDBusConnection::systemBus().asyncCall(method);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *call) {
        QDBusPendingReply<void> reply = *call;
        if (reply.isError()) {
            qCWarning(lcLipstickCoreLog) << "Could not activate session:" << reply.error();
        } else {
            // VT switching may fail without notice, check status again a bit later
            QTimer::singleShot(100, this, &LipstickCompositor::activateLogindSession);
        }
        call->deleteLater();
    });

    qCDebug(lcLipstickCoreLog) << "Session" << m_logindSession << "is activating";
}

void LipstickCompositor::initialize()
{
    activateLogindSession();

    TouchScreen *touchScreen = HomeApplication::instance()->touchScreen();
    reactOnDisplayStateChanges(TouchScreen::DisplayUnknown, touchScreen->currentDisplayState());
    connect(touchScreen, &TouchScreen::displayStateChanged, this, &LipstickCompositor::reactOnDisplayStateChanges);

    new LipstickCompositorAdaptor(this);

    QDBusConnection systemBus = QDBusConnection::systemBus();
    if (!systemBus.registerObject("/", this)) {
        qWarning("Unable to register object at path /: %s", systemBus.lastError().message().toUtf8().constData());
    }

    /* We might have for example minui based encryption unlock ui
     * running as compositor and waiting to be able to hand-off
     * to us -> use ReplaceExistingService to facilitate this.
     */
    QDBusReply<QDBusConnectionInterface::RegisterServiceReply> reply =
        systemBus.interface()->registerService(QStringLiteral("org.nemomobile.compositor"),
                                               QDBusConnectionInterface::ReplaceExistingService,
                                               QDBusConnectionInterface::DontAllowReplacement);
    if (!reply.isValid()) {
        qWarning("Unable to register D-Bus service org.nemomobile.compositor: %s",
                 reply.error().message().toUtf8().constData());
    } else if (reply.value() != QDBusConnectionInterface::ServiceRegistered) {
        qWarning("Unable to register D-Bus service org.nemomobile.compositor: %s",
                 "Did not get primary name ownership");
    }

    new FileServiceAdaptor(this);
    QDBusConnection sessionBus = QDBusConnection::sessionBus();
    sessionBus.registerObject(QLatin1String("/"), this);
    sessionBus.registerService(QLatin1String("org.sailfishos.fileservice"));
}

void LipstickCompositor::windowDestroyed(LipstickCompositorWindow *item)
{
    int id = item->windowId();

    m_windows.remove(id);
    surfaceUnmapped(item);
}

void LipstickCompositor::surfaceMapped()
{
    QWaylandSurface *surface = qobject_cast<QWaylandSurface *>(sender());

    LipstickCompositorWindow *item = surfaceWindow(surface);
    if (!item)
        item = static_cast<LipstickCompositorWindow *>(createView(surface));

    // The surface was mapped for the first time
    if (item->m_mapped)
        return;

    QWaylandSurface *transientParent = surface->transientParent();
    if (transientParent) {
        LipstickCompositorWindow *transientParentItem = surfaceWindow(transientParent);
        if (transientParentItem) {
            item->setParentItem(transientParentItem);
            item->setX(surface->transientOffset().x());
            item->setY(surface->transientOffset().y());
        } else {
            qWarning("Surface %s was mapped without visible transient parent", qPrintable(transientParent->className()));
        }
    }

    QVariantMap properties = surface->windowProperties();

    item->m_mapped = true;
    item->m_category = properties.value("CATEGORY").toString();

    if (!item->parentItem()) {
        // TODO why contentItem?
        item->setParentItem(contentItem());
    }

    item->setSize(surface->size());
    m_totalWindowCount++;
    m_mappedSurfaces.insert(item->windowId(), item);

    item->setTouchEventsEnabled(true);

    emit windowCountChanged();
    emit windowAdded(item);

    windowAdded(item->windowId());

    emit availableWinIdsChanged();
}

void LipstickCompositor::surfaceUnmapped()
{
    surfaceUnmapped(qobject_cast<QWaylandSurface *>(sender()));
}

void LipstickCompositor::surfaceSizeChanged()
{
    QWaylandSurface *surface = qobject_cast<QWaylandSurface *>(sender());

    LipstickCompositorWindow *window = surfaceWindow(surface);
    if (window)
        window->setSize(surface->size());
}

void LipstickCompositor::surfaceTitleChanged()
{
    QWaylandSurface *surface = qobject_cast<QWaylandSurface *>(sender());
    LipstickCompositorWindow *window = surfaceWindow(surface);

    if (window) {
        emit window->titleChanged();

        int windowId = window->windowId();

        for (int ii = 0; ii < m_windowModels.count(); ++ii)
            m_windowModels.at(ii)->titleChanged(windowId);
    }
}

void LipstickCompositor::surfaceRaised()
{
    QWaylandSurface *surface = qobject_cast<QWaylandSurface *>(sender());
    LipstickCompositorWindow *window = surfaceWindow(surface);

    if (window && window->m_mapped) {
        emit windowRaised(window);
    }
}

void LipstickCompositor::surfaceLowered()
{
    QWaylandSurface *surface = qobject_cast<QWaylandSurface *>(sender());
    LipstickCompositorWindow *window = surfaceWindow(surface);

    if (window && window->m_mapped) {
        emit windowLowered(window);
    }
}

void LipstickCompositor::windowSwapped()
{
    sendFrameCallbacks(surfaces());
}

void LipstickCompositor::windowDestroyed()
{
    m_totalWindowCount--;
    m_windows.remove(static_cast<LipstickCompositorWindow *>(sender())->windowId());
    emit ghostWindowCountChanged();
}

void LipstickCompositor::windowPropertyChanged(const QString &property)
{
    QWaylandSurface *surface = qobject_cast<QWaylandSurface *>(sender());

    if (debug())
        qDebug() << "Window properties changed:" << surface << surface->windowProperties();

    if (property == QLatin1String("MOUSE_REGION")) {
        LipstickCompositorWindow *window = surfaceWindow(surface);
        if (window)
            window->refreshMouseRegion();
    } else if (property == QLatin1String("GRABBED_KEYS")) {
        LipstickCompositorWindow *window = surfaceWindow(surface);
        if (window)
            window->refreshGrabbedKeys();
    }
}

void LipstickCompositor::surfaceUnmapped(QWaylandSurface *surface)
{
    LipstickCompositorWindow *window = surfaceWindow(surface);
    if (window)
        emit windowHidden(window);
}

void LipstickCompositor::surfaceUnmapped(LipstickCompositorWindow *item)
{
    int id = item->windowId();

    int gc = ghostWindowCount();
    if (m_mappedSurfaces.remove(item->windowId()) == 0)
        // It was unmapped already so nothing to do
        return;

    emit windowCountChanged();
    emit windowRemoved(item);

    if (gc != ghostWindowCount())
        emit ghostWindowCountChanged();

    windowRemoved(id);

    emit availableWinIdsChanged();
}

void LipstickCompositor::windowAdded(int id)
{
    for (int ii = 0; ii < m_windowModels.count(); ++ii)
        m_windowModels.at(ii)->addItem(id);
}

void LipstickCompositor::windowRemoved(int id)
{
    for (int ii = 0; ii < m_windowModels.count(); ++ii)
        m_windowModels.at(ii)->remItem(id);
}

void LipstickCompositor::setTopmostWindowOrientation(Qt::ScreenOrientation topmostWindowOrientation)
{
    if (m_topmostWindowOrientation != topmostWindowOrientation) {
        m_topmostWindowOrientation = topmostWindowOrientation;
        emit topmostWindowOrientationChanged();
    }
}

void LipstickCompositor::setScreenOrientation(Qt::ScreenOrientation screenOrientation)
{
    if (m_screenOrientation != screenOrientation) {
        if (debug())
            qDebug() << "Setting screen orientation on QWaylandCompositor";

        QWaylandCompositor::setScreenOrientation(screenOrientation);
        QWindowSystemInterface::handleScreenOrientationChange(qApp->primaryScreen(),screenOrientation);

        m_screenOrientation = screenOrientation;
        emit screenOrientationChanged();
    }
}

bool LipstickCompositor::displayDimmed() const
{
    TouchScreen *touchScreen = HomeApplication::instance()->touchScreen();
    return displayStateIsDimmed(touchScreen->currentDisplayState());
}

LipstickKeymap *LipstickCompositor::keymap() const
{
    return m_keymap;
}

void LipstickCompositor::setKeymap(LipstickKeymap *keymap)
{
    if (m_keymap == keymap)
        return;

    bool update = true;

    if (m_keymap && keymap)
        update = (*m_keymap != *keymap);

    if (m_keymap)
        disconnect(m_keymap, 0, this, 0);

    m_keymap = keymap;

    if (m_keymap) {
        connect(m_keymap, &LipstickKeymap::rulesChanged, this, &LipstickCompositor::updateKeymap);
        connect(m_keymap, &LipstickKeymap::modelChanged, this, &LipstickCompositor::updateKeymap);
        connect(m_keymap, &LipstickKeymap::layoutChanged, this, &LipstickCompositor::updateKeymap);
        connect(m_keymap, &LipstickKeymap::variantChanged, this, &LipstickCompositor::updateKeymap);
        connect(m_keymap, &LipstickKeymap::optionsChanged, this, &LipstickCompositor::updateKeymap);
    }

    if (update)
        updateKeymap();

    emit keymapChanged();
}

void LipstickCompositor::updateKeymap()
{
    if (m_keymap)
        defaultInputDevice()->setKeymap(m_keymap->waylandKeymap());
    else
        defaultInputDevice()->setKeymap(QWaylandKeymap());
}

void LipstickCompositor::reactOnDisplayStateChanges(TouchScreen::DisplayState oldState, TouchScreen::DisplayState newState)
{
    bool oldOn = displayStateIsOn(oldState);
    bool newOn = displayStateIsOn(newState);

    if (oldOn != newOn) {
        if (newOn) {
            emit displayOn();
        } else {
            QCoreApplication::postEvent(this, new QTouchEvent(QEvent::TouchCancel));
            emit displayOff();
        }
    }

    bool oldDimmed = displayStateIsDimmed(oldState);
    bool newDimmed = displayStateIsDimmed(newState);

    if (oldDimmed != newDimmed) {
        emit displayDimmedChanged();
    }
}

void LipstickCompositor::setScreenOrientationFromSensor()
{
    QOrientationReading* reading = m_orientationSensor->reading();

    if (debug())
        qDebug() << "Screen orientation changed " << reading->orientation();

    Qt::ScreenOrientation sensorOrientation = m_sensorOrientation;
    switch (reading->orientation()) {
        case QOrientationReading::TopUp:
            sensorOrientation = Qt::PortraitOrientation;
            break;
        case QOrientationReading::TopDown:
            sensorOrientation = Qt::InvertedPortraitOrientation;
            break;
        case QOrientationReading::LeftUp:
            sensorOrientation = Qt::InvertedLandscapeOrientation;
            break;
        case QOrientationReading::RightUp:
            sensorOrientation = Qt::LandscapeOrientation;
            break;
        case QOrientationReading::FaceUp:
        case QOrientationReading::FaceDown:
            /* Keep screen orientation at previous state */
            break;
        case QOrientationReading::Undefined:
        default:
            sensorOrientation = Qt::PrimaryOrientation;
            break;
    }

    if (sensorOrientation != m_sensorOrientation) {
        m_sensorOrientation = sensorOrientation;
        emit sensorOrientationChanged();
    }
}

void LipstickCompositor::clipboardDataChanged()
{
    const QMimeData *mimeData = QGuiApplication::clipboard()->mimeData();
    if (mimeData && mimeData != m_retainedSelection)
        overrideSelection(const_cast<QMimeData *>(mimeData));
}

void LipstickCompositor::setUpdatesEnabledNow(bool enabled)
{
    if (m_updatesEnabled != enabled) {
        m_updatesEnabled = enabled;

        if (!m_updatesEnabled) {
            emit displayAboutToBeOff();
            LipstickCompositorWindow *topmostWindow = qobject_cast<LipstickCompositorWindow *>(windowForId(topmostWindowId()));
            if (topmostWindow != 0 && topmostWindow->hasFocus()) {
                m_onUpdatesDisabledUnfocusedWindowId = topmostWindow->windowId();
                clearKeyboardFocus();
            }
            hide();
            if (QWindow::handle()) {
                QGuiApplication::platformNativeInterface()->nativeResourceForIntegration("DisplayOff");
            }
            // trigger frame callbacks which are pending already at this time
            surfaceCommitted();
        } else {
            if (QWindow::handle()) {
                QGuiApplication::platformNativeInterface()->nativeResourceForIntegration("DisplayOn");
            }
            emit displayAboutToBeOn();
            showFullScreen();
            if (m_onUpdatesDisabledUnfocusedWindowId > 0) {
                if (!LipstickSettings::instance()->lockscreenVisible()) {
                    LipstickCompositorWindow *topmostWindow = qobject_cast<LipstickCompositorWindow *>(windowForId(topmostWindowId()));
                    if (topmostWindow != 0 && topmostWindow->windowId() == m_onUpdatesDisabledUnfocusedWindowId) {
                        topmostWindow->takeFocus();
                    }
                }
                m_onUpdatesDisabledUnfocusedWindowId = 0;
            }
        }
    }

    if (m_updatesEnabled && !m_completed) {
        m_completed = true;
        emit completedChanged();
    }
}

void LipstickCompositor::setUpdatesEnabled(bool enabled)
{
    if (!calledFromDBus()) {
        setUpdatesEnabledNow(enabled);
    } else {
        if (message().isReplyRequired())
            setDelayedReply(true);
        m_queuedSetUpdatesEnabledCalls.append(QueuedSetUpdatesEnabledCall(connection(), message(), enabled));
        QMetaObject::invokeMethod(this, "processQueuedSetUpdatesEnabledCalls", Qt::QueuedConnection);
    }
}

void LipstickCompositor::processQueuedSetUpdatesEnabledCalls()
{
    if (m_mceNameOwner->valid()) {
        while (!m_queuedSetUpdatesEnabledCalls.isEmpty()) {
            QueuedSetUpdatesEnabledCall queued(m_queuedSetUpdatesEnabledCalls.takeFirst());
            if (queued.m_message.service() != m_mceNameOwner->nameOwner()) {
                if (queued.m_message.isReplyRequired()) {
                    QDBusMessage reply(queued.m_message.createErrorReply(DBUS_ERROR_ACCESS_DENIED,
                                                                         "Only mce is allowed to call this method"));
                    queued.m_connection.send(reply);
                }
            } else {
                setUpdatesEnabledNow(queued.m_enable);
                if (queued.m_message.isReplyRequired()) {
                    QDBusMessage reply(queued.m_message.createReply());
                    queued.m_connection.send(reply);
                }
            }
        }
    }
}

void LipstickCompositor::readContent()
{
    m_recorder->recordFrame(this);
}

void LipstickCompositor::surfaceCommitted()
{
    if (!isVisible() && m_fakeRepaintTimerId == 0) {
        m_fakeRepaintTimerId = startTimer(1000);
    } else if (isVisible() && m_fakeRepaintTimerId > 0) {
        killTimer(m_fakeRepaintTimerId);
        m_fakeRepaintTimerId = 0;
    }
}

void LipstickCompositor::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == m_fakeRepaintTimerId) {
        frameStarted();
        sendFrameCallbacks(surfaces());
        killTimer(e->timerId());
        m_fakeRepaintTimerId = 0;
    }
}

bool LipstickCompositor::event(QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

        if (m_synthesizeBackEvent && mouseEvent->button() == Qt::RightButton) {
            // see xkeyboard-config/keycodes/evdev: Map to <I166> = 166; #define KEY_BACK
            int scanCode = 166;
            if (mouseEvent->type() == QEvent::MouseButtonPress) {
                sendKeyEvent(QEvent::KeyPress, Qt::Key_Back, scanCode);
            } else {
                sendKeyEvent(QEvent::KeyRelease, Qt::Key_Back, scanCode);
            }
            return true;
        }
    }
    return QQuickWindow::event(event);
}

void LipstickCompositor::sendKeyEvent(QEvent::Type type, Qt::Key key, quint32 nativeScanCode)
{
    QKeyEvent event(type, key, Qt::NoModifier, nativeScanCode, 0, 0);

    // Not all Lipstick windows are real windows
    LipstickCompositorWindow *topmostWindow = qobject_cast<LipstickCompositorWindow *>(windowForId(topmostWindowId()));
    if (topmostWindow && topmostWindow->isInProcess()) {
        QCoreApplication::sendEvent(activeFocusItem(), &event);
    } else {
        defaultInputDevice()->sendFullKeyEvent(&event);
    }
}
