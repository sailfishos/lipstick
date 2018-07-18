/***************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Aaron Kennedy <aaron.kennedy@jollamobile.com>
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

#ifdef HAVE_CONTENTACTION
#include <contentaction.h>
#endif

#include <QWaylandSeat>
#include <QDesktopServices>
#include <QtSensors/QOrientationSensor>
#include <QClipboard>
#include <QMimeData>
#include <QtGui/qpa/qplatformnativeinterface.h>
#include "homeapplication.h"
#include "touchscreen/touchscreen.h"
#include "windowmodel.h"
#include "lipstickcompositorprocwindow.h"
#include "lipstickcompositor.h"
#include "lipstickcompositoradaptor.h"
#include "lipsticksettings.h"
#include "windowpropertymap.h"
#include <qpa/qwindowsysteminterface.h>
#include "hwcrenderstage.h"
#include <private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QWaylandQuickShellSurfaceItem>
#include <QtWaylandCompositor/private/qwlextendedsurface_p.h>

#include <QWaylandXdgShellV5>

#include "alienmanager/alienmanager.h"

LipstickCompositor *LipstickCompositor::m_instance = 0;

LipstickCompositor::LipstickCompositor()
    : m_totalWindowCount(0)
    , m_nextWindowId(1)
    , m_homeActive(true)
    , m_shaderEffect(0)
    , m_fullscreenSurface(0)
    , m_directRenderingActive(false)
    , m_topmostWindowId(0)
    , m_topmostWindowProcessId(0)
    , m_topmostWindowOrientation(Qt::PrimaryOrientation)
    , m_screenOrientation(Qt::PrimaryOrientation)
    , m_sensorOrientation(Qt::PrimaryOrientation)
    , m_retainedSelection(0)
    , m_updatesEnabled(true)
    , m_completed(false)
    , m_onUpdatesDisabledUnfocusedWindowId(0)
    , m_fakeRepaintTriggered(false)
{
    m_window.reset(new QQuickWindow());
    m_window->setColor(Qt::black);
    m_window->setVisible(true);

    m_output.reset(new QWaylandQuickOutput(this, m_window.data()));
    m_output->setSizeFollowsWindow(true);

    m_wlShell.reset(new QWaylandWlShell(this));
    connect(m_wlShell.data(), &QWaylandWlShell::wlShellSurfaceCreated, this, &LipstickCompositor::onShellSurfaceCreated);

    m_xdgShell.reset(new QWaylandXdgShellV5(this));
    connect(m_xdgShell.data(), &QWaylandXdgShellV5::xdgSurfaceCreated, this, &LipstickCompositor::onXdgSurfaceCreated);

    m_alienManager.reset(new AlienManager(this));

    m_surfExtGlob.reset(new QtWayland::SurfaceExtensionGlobal(this));
    connect(m_surfExtGlob.data(), &QtWayland::SurfaceExtensionGlobal::extendedSurfaceReady, this, &LipstickCompositor::onExtendedSurfaceReady);

    setRetainedSelectionEnabled(true);

    if (m_instance) qFatal("LipstickCompositor: Only one compositor instance per process is supported");
    m_instance = this;

    m_orientationLock = new MGConfItem("/lipstick/orientationLock", this);
    connect(m_orientationLock, &MGConfItem::valueChanged, this, &LipstickCompositor::orientationLockChanged);

    // Load legacy settings from the config file and delete it from there
    QSettings legacySettings("nemomobile", "lipstick");
    QString legacyOrientationKey("Compositor/orientationLock");
    if (legacySettings.contains(legacyOrientationKey)) {
        m_orientationLock->set(legacySettings.value(legacyOrientationKey));
        legacySettings.remove(legacyOrientationKey);
    }

    connect(m_window.data(), &QWindow::visibleChanged, this, &LipstickCompositor::onVisibleChanged);
    connect(m_window.data(), &QQuickWindow::afterRendering, this, &LipstickCompositor::windowSwapped);
    connect(HomeApplication::instance(), &HomeApplication::aboutToDestroy, this, &LipstickCompositor::homeApplicationAboutToDestroy);

    m_orientationSensor = new QOrientationSensor(this);
    connect(m_orientationSensor, &QOrientationSensor::readingChanged, this, &LipstickCompositor::setScreenOrientationFromSensor);
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

    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, this, &LipstickCompositor::clipboardDataChanged);

    HwcRenderStage::initialize(this);

    QTimer::singleShot(0, this, &LipstickCompositor::initialize);
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
    disconnect(m_window.data(), &QWindow::visibleChanged, this, &LipstickCompositor::onVisibleChanged);

    delete m_shaderEffect;

    m_instance = nullptr;
}

LipstickCompositor *LipstickCompositor::instance()
{
    return m_instance;
}

QQmlListProperty<QObject> LipstickCompositor::data()
{
    return QQuickWindowPrivate::get(m_window.data())->data();
}

void LipstickCompositor::homeApplicationAboutToDestroy()
{
    m_window->hide();
    m_window->releaseResources();

    // When destroying LipstickCompositor ~QQuickWindow() is called after
    // ~QWaylandQuickCompositor(), so changes to the items in the window may end
    // up calling code such as LipstickCompositorWindow::handleTouchCancel(),
    // which will try to use the compositor, at that point not usable anymore.
    // So delete all the windows here.
    foreach (LipstickCompositorWindow *w, m_windows) {
        delete w;
    }

    m_instance = 0;
    delete this;
}

void LipstickCompositor::onVisibleChanged(bool visible)
{
    if (!visible) {
        m_output->sendFrameCallbacks();
    }
}

LipstickCompositorWindow *LipstickCompositor::surfaceWindow(QWaylandSurface *surface)
{
    for (QWaylandView *view : surface->views()) {
        if (LipstickCompositorWindow *window = qobject_cast<LipstickCompositorWindow *>(view->renderObject())) {
            return window;
        }
    }
    return nullptr;
}

void LipstickCompositor::onShellSurfaceCreated(QWaylandWlShellSurface *shellSurface)
{
    QWaylandSurface * const surface = shellSurface->surface();
    LipstickCompositorWindow * const item = new LipstickCompositorWindow(m_nextWindowId++, QString(), shellSurface);

    m_windows.insert(item->windowId(), item);

    connect(surface, &QWaylandSurface::damaged, this, &LipstickCompositor::surfaceDamaged);
    connect(surface, &QWaylandSurface::redraw, this, &LipstickCompositor::windowSwapped);
}

void LipstickCompositor::onXdgSurfaceCreated(QWaylandXdgSurfaceV5 *xdgSurface)
{
    QWaylandSurface * const surface = xdgSurface->surface();
    LipstickCompositorWindow * const item = new LipstickCompositorWindow(m_nextWindowId++, QString(), xdgSurface);

    m_windows.insert(item->windowId(), item);

    connect(surface, &QWaylandSurface::damaged, this, &LipstickCompositor::surfaceDamaged);
    connect(surface, &QWaylandSurface::redraw, this, &LipstickCompositor::windowSwapped);
}

void LipstickCompositor::onExtendedSurfaceReady(QtWayland::ExtendedSurface *extSurface, QWaylandSurface *surface)
{
    LipstickCompositorWindow *window = surfaceWindow(surface);
    if(window)
        window->setExtendedSurface(extSurface);

    connect(extSurface, &QtWayland::ExtendedSurface::raiseRequested, this, [this, window]() {
        windowRaised(window);
    });
    connect(extSurface, &QtWayland::ExtendedSurface::lowerRequested, this, [this, window]() {
        windowLowered(window);
    });
}

void LipstickCompositor::onAlienSurfaceCreated(AlienSurface *alienSurface, QWaylandSurface *surface)
{
    qDebug() << "Alien surface created";
    LipstickCompositorWindow * const item = new LipstickCompositorWindow(m_nextWindowId++, alienSurface);

    m_windows.insert(item->windowId(), item);

    connect(surface, &QWaylandSurface::damaged, this, &LipstickCompositor::surfaceDamaged);
    connect(surface, &QWaylandSurface::redraw, this, &LipstickCompositor::windowSwapped);
}

bool LipstickCompositor::openUrl(QWaylandClient *client, const QUrl &url)
{
    Q_UNUSED(client)
    return openUrl(url);
}

bool LipstickCompositor::openUrl(const QUrl &url)
{
#if defined(HAVE_CONTENTACTION)
    ContentAction::Action action = url.scheme() == "file"? ContentAction::Action::defaultActionForFile(url.toString()) : ContentAction::Action::defaultActionForScheme(url.toString());
    if (action.isValid()) {
        action.trigger();
    }
    return action.isValid();
#else
    Q_UNUSED(url)
    return false;
#endif
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

LipstickCompositorWindow *LipstickCompositor::windowForId(int id) const
{
    return m_windows.value(id, NULL);
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
    return window?window->surface():0;
}

bool LipstickCompositor::completed()
{
    return m_completed;
}

int LipstickCompositor::windowIdForLink(QWaylandSurface *surface, uint link) const
{
    for (LipstickCompositorWindow *window : m_windows) {
        QWaylandSurface * const windowSurface = window->surface();

        if (windowSurface
                && windowSurface->client()
                && surface->client()
                && window
                && windowSurface->client()->processId() == surface->client()->processId()
                && window->windowProperty(QStringLiteral("WINID")).toUInt() == link) {
            return window->windowId();
        }
    }

    return 0;
}

void LipstickCompositor::clearKeyboardFocus()
{
//    defaultInputDevice()->setKeyboardFocus(NULL);
}

void LipstickCompositor::setDisplayOff()
{
    HomeApplication::instance()->setDisplayOff();
}

void LipstickCompositor::surfaceDamaged(const QRegion &)
{
    if (!m_window->isVisible()) {
        // If the compositor is not visible, do not throttle.
        // make it conditional to QT_WAYLAND_COMPOSITOR_NO_THROTTLE?
        m_output->sendFrameCallbacks();
    }
}

void LipstickCompositor::setFullscreenSurface(QWaylandSurface *surface)
{
    if (surface == m_fullscreenSurface)
        return;

    m_fullscreenSurface = surface;

    emit fullscreenSurfaceChanged();
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

        int pid = -1;
        QWaylandSurface *surface = surfaceForId(m_topmostWindowId);

        if (surface)
            pid = surface->client()->processId();

        if (m_topmostWindowProcessId != pid) {
            m_topmostWindowProcessId = pid;
            emit privateTopmostWindowProcessIdChanged(m_topmostWindowProcessId);
        }
    }
}

void LipstickCompositor::initialize()
{
    TouchScreen *touchScreen = HomeApplication::instance()->touchScreen();
    reactOnDisplayStateChanges(TouchScreen::DisplayUnknown, touchScreen->currentDisplayState());
    connect(touchScreen, &TouchScreen::displayStateChanged, this, &LipstickCompositor::reactOnDisplayStateChanges);

    new LipstickCompositorAdaptor(this);

    QDBusConnection systemBus = QDBusConnection::systemBus();
    if (!systemBus.registerObject("/", this)) {
        qWarning("Unable to register object at path /: %s", systemBus.lastError().message().toUtf8().constData());
    }
    if (!systemBus.registerService("org.nemomobile.compositor")) {
        qWarning("Unable to register D-Bus service org.nemomobile.compositor: %s", systemBus.lastError().message().toUtf8().constData());
    }
}

void LipstickCompositor::windowDestroyed(LipstickCompositorWindow *item)
{
    int id = item->windowId();

    m_windows.remove(id);
    m_totalWindowCount--;

    if (m_mappedSurfaces.remove(id) != 0) {
        emit windowCountChanged();
        emit windowRemoved(item);

        windowRemoved(id);

        emit availableWinIdsChanged();
    }

    emit ghostWindowCountChanged();
}

void LipstickCompositor::surfaceMapped(LipstickCompositorWindow *window, QWaylandSurface *surface)
{
    Q_UNUSED(surface);

    m_mappedSurfaces.insert(window->windowId(), window);

    emit windowCountChanged();

    emit windowAdded(window);

    windowAdded(window->windowId());

    emit availableWinIdsChanged();
}

void LipstickCompositor::windowSwapped()
{
    m_output->sendFrameCallbacks();
}

void LipstickCompositor::surfaceUnmapped(LipstickCompositorWindow *window, QWaylandSurface *surface)
{
    if (surface == m_fullscreenSurface) {
        setFullscreenSurface(nullptr);
    }

    emit windowHidden(window);
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

QQmlComponent *LipstickCompositor::shaderEffectComponent()
{
    const char *qml_source =
        "import QtQuick 2.0\n"
        "ShaderEffect {\n"
            "property QtObject window\n"
            "property ShaderEffectSource source: ShaderEffectSource { sourceItem: window }\n"
        "}";

    if (!m_shaderEffect) {
        m_shaderEffect = new QQmlComponent(qmlEngine(this));
        m_shaderEffect->setData(qml_source, QUrl());
    }
    return m_shaderEffect;
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

        QSize physSize = m_output->physicalSize();
        switch(screenOrientation) {
        case Qt::PrimaryOrientation:
            m_output->setTransform(QWaylandOutput::TransformNormal);
            break;
        case Qt::LandscapeOrientation:
            if(physSize.width() > physSize.height())
                m_output->setTransform(QWaylandOutput::TransformNormal);
            else
                m_output->setTransform(QWaylandOutput::Transform90);
            break;
        case Qt::PortraitOrientation:
            if(physSize.width() > physSize.height())
                m_output->setTransform(QWaylandOutput::Transform90);
            else
                m_output->setTransform(QWaylandOutput::TransformNormal);
            break;
        case Qt::InvertedLandscapeOrientation:
            if(physSize.width() > physSize.height())
                m_output->setTransform(QWaylandOutput::Transform180);
            else
                m_output->setTransform(QWaylandOutput::Transform270);
            break;
        case Qt::InvertedPortraitOrientation:
            if(physSize.width() > physSize.height())
                m_output->setTransform(QWaylandOutput::Transform270);
            else
                m_output->setTransform(QWaylandOutput::Transform180);
            break;
        }
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

void LipstickCompositor::setUpdatesEnabled(bool enabled)
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
            m_window->hide();
            if (m_window->handle()) {
                QGuiApplication::platformNativeInterface()->nativeResourceForIntegration("DisplayOff");
            }
            // trigger frame callbacks which are pending already at this time
            surfaceCommitted();
        } else {
            if (m_window->handle()) {
                QGuiApplication::platformNativeInterface()->nativeResourceForIntegration("DisplayOn");
            }
            emit displayAboutToBeOn();
            m_window->showFullScreen();
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

void LipstickCompositor::surfaceCommitted()
{
    if (!m_window->isVisible() && !m_fakeRepaintTriggered) {
        startTimer(1000);
        m_fakeRepaintTriggered = true;
    }
}

void LipstickCompositor::timerEvent(QTimerEvent *e)
{
    Q_UNUSED(e)

    m_output->frameStarted();
    m_output->sendFrameCallbacks();
    killTimer(e->timerId());
}
