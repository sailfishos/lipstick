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

#include <QCoreApplication>
#include <QWaylandCompositor>
#include <QWaylandSeat>
#include <QTimer>
#include <sys/types.h>
#include <signal.h>
#include "lipstickcompositor.h"
#include "lipstickcompositorwindow.h"
#include "windowpropertymap.h"
#include "windowmodel.h"

#include "hwcimage.h"
#include "hwcrenderstage.h"
#include <EGL/egl.h>
#include <private/qquickwindow_p.h>
#include <QtWaylandCompositor/private/qwlextendedsurface_p.h>
#include <QtWaylandCompositor/private/qwlqtkey_p.h>

#include <QQmlEngine>
#include <QWaylandXdgSurfaceV5>
#include "alienmanager/aliensurface.h"

LipstickCompositorWindow::LipstickCompositorWindow(
        int windowId, const QString &category, QWaylandSurface *surface)
    : QWaylandQuickItem()
    , m_category(category)
    , m_compositor(LipstickCompositor::instance())
    , m_windowId(windowId)
    , m_isAlien(false)
    , m_delayRemove(false)
    , m_windowClosed(false)
    , m_removePosted(false)
    , m_interceptingTouch(false)
    , m_mapped(false)
    , m_noHardwareComposition(false)
    , m_focusOnTouch(false)
    , m_hasVisibleReferences(false)
    , m_transient(false)
    , m_exposed(true)
    , m_exposedAsCover(false)
    , m_explicitMouseRegion(false)
{
    setFlags(QQuickItem::ItemIsFocusScope | flags());

    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

    // Handle ungrab situations
    connect(this, SIGNAL(visibleChanged()), SLOT(handleTouchCancel()));
    connect(this, SIGNAL(enabledChanged()), SLOT(handleTouchCancel()));
    connect(this, SIGNAL(touchEventsEnabledChanged()), SLOT(handleTouchCancel()));

    if(surface) {
        connect(surface, &QWaylandSurface::surfaceDestroyed, this, [this]() {
            m_windowClosed = true;
            tryRemove();
        });

        connect(surface, &QWaylandSurface::sizeChanged, this, [this]() {
            setSize(QWaylandQuickItem::surface()->size());
        });

        connect(surface, &QWaylandSurface::hasContentChanged, this, [this](){
            QWaylandSurface * const surface = QWaylandQuickItem::surface();

            if (!surface) {
                // The signal has been emitted by the surface object so it exists, but during
                // cleanup the QWaylandQuickItem may have cleared its pointer before the signal
                // was received.
            } else if (!surface->hasContent()) {
                m_compositor->surfaceUnmapped(this, surface);
            } else if (!m_mapped) {
                m_mapped = true;
                if (m_extSurface) {
                    m_category = m_extSurface->windowProperty(QStringLiteral("CATEGORY")).toString();
                }
                setSize(surface->size());
                setTouchEventsEnabled(true);

                m_compositor->surfaceMapped(this, surface);
            }
        });

        setSurface(surface);
    }
}

LipstickCompositorWindow::LipstickCompositorWindow(
        int windowId, const QString &category, QWaylandWlShellSurface *wlSurface)
    : LipstickCompositorWindow(windowId, category, wlSurface->surface())
{
    m_wlShellSurface = wlSurface;
    connect(m_wlShellSurface.data(), &QWaylandWlShellSurface::classNameChanged,
            this, &LipstickCompositorWindow::classNameChanged);
    connect(m_wlShellSurface.data(), &QWaylandWlShellSurface::pong,
            this, &LipstickCompositorWindow::pong);

    connect(m_wlShellSurface.data(), &QWaylandWlShellSurface::titleChanged, this, [this]() {
        setTitle(m_wlShellSurface->title());
    });

    connect(m_wlShellSurface.data(),
                &QWaylandWlShellSurface::setTransient,
                this,
                [this](QWaylandSurface *transientParent, const QPoint &relativeToParent) {
        if (LipstickCompositorWindow *parentItem = LipstickCompositor::surfaceWindow(transientParent)) {
            m_transient = true;
            setParentItem(parentItem);
            setPosition(relativeToParent);
            setVisible(m_mapped);
        } else {
            qWarning("Surface was mapped without visible transient parent");
        }
    });
    connect(m_wlShellSurface.data(),
                &QWaylandWlShellSurface::setFullScreen,
                this,
                [this](QWaylandWlShellSurface::FullScreenMethod, uint, QWaylandOutput *output) {
        if (!output) {
            output = m_compositor->m_output.data();
        }

        m_wlShellSurface->sendConfigure(output->geometry().size(), QWaylandWlShellSurface::NoneEdge);
    });

    setTitle(m_wlShellSurface->title());
}

LipstickCompositorWindow::LipstickCompositorWindow(
        int windowId, const QString &category, QWaylandXdgSurfaceV5 *xdgSurface)
    : LipstickCompositorWindow(windowId, category, xdgSurface->surface())
{
    m_xdgSurface = xdgSurface;

    connect(m_xdgSurface.data(), &QWaylandXdgSurfaceV5::titleChanged, this, [this]() {
        setTitle(m_xdgSurface->title());
    });
    connect(m_xdgSurface.data(), &QWaylandXdgSurfaceV5::setTransient, this, [this]() {
        if (LipstickCompositorWindow *parentItem = LipstickCompositor::surfaceWindow(m_xdgSurface->parentSurface()->surface())) {
            m_transient = true;
            setParentItem(parentItem);
            setVisible(m_mapped);
        } else {
            qWarning("Surface was mapped without visible transient parent");
        }
    });
    connect(m_xdgSurface.data(), &QWaylandXdgSurfaceV5::setFullscreen, this, [this]() {
        QWaylandOutput *output = m_compositor->m_output.data();

        m_xdgSurface->sendConfigure(
                    output->geometry().size(),
                    QVector<QWaylandXdgSurfaceV5::State>() << QWaylandXdgSurfaceV5::FullscreenState);
    });
    connect(m_compositor->m_xdgShell.data(), &QWaylandXdgShellV5::pong, this, [this](uint serial) {
        if (m_pingSerial == serial) {
            m_pingSerial = 0;
            emit pong();
        }
    });

    setTitle(m_xdgSurface->title());
}

LipstickCompositorWindow::LipstickCompositorWindow(int windowId, AlienSurface *alienSurface)
    : LipstickCompositorWindow(windowId, QString(), alienSurface->surface())
{
    m_alienSurface = alienSurface;
    m_isAlien = true;

    connect(m_alienSurface.data(), &AlienSurface::titleChanged, this, &LipstickCompositorWindow::setTitle);
    connect(m_alienSurface.data(), &AlienSurface::pong, this, &LipstickCompositorWindow::pong);

    setTitle(m_alienSurface->title());
}


LipstickCompositorWindow::~LipstickCompositorWindow()
{
    // We don't want tryRemove() posting an event anymore, we're dying anyway
    m_removePosted = true;
    LipstickCompositor::instance()->windowDestroyed(this);
}

QVariant LipstickCompositorWindow::userData() const
{
    return m_data;
}

void LipstickCompositorWindow::setUserData(QVariant data)
{
    if (m_data == data)
        return;

    m_data = data;
    emit userDataChanged();
}

int LipstickCompositorWindow::windowId() const
{
    return m_windowId;
}

bool LipstickCompositorWindow::isAlien() const
{
    return m_isAlien;
}

qint64 LipstickCompositorWindow::processId() const
{
    if (surface())
        return surface()->client()->processId();
    else return 0;
}

bool LipstickCompositorWindow::delayRemove() const
{
    return m_delayRemove;
}

void LipstickCompositorWindow::setDelayRemove(bool delay)
{
    if (delay == m_delayRemove)
        return;

    m_delayRemove = delay;

    emit delayRemoveChanged();

    tryRemove();
}

QString LipstickCompositorWindow::category() const
{
    return m_category;
}

QtWayland::ExtendedSurface *LipstickCompositorWindow::extendedSurface()
{
    return m_extSurface.data();
}

void LipstickCompositorWindow::setExtendedSurface(QtWayland::ExtendedSurface *extSurface)
{
    m_extSurface = extSurface;
    connect(m_extSurface.data(), &QtWayland::ExtendedSurface::windowFlagsChanged,
            this, &LipstickCompositorWindow::windowFlagsChanged);

    connect(m_extSurface.data(), &QtWayland::ExtendedSurface::windowPropertyChanged,
            this, [this](const QString &key, const QVariant &value) {
        if (key == QLatin1String("MOUSE_REGION")) {
            updateMouseRegion(value);
        }
        if (key == QLatin1String("GRABBED_KEYS")) {
            updateGrabbedKeys(value);
        }
    });
}

void LipstickCompositorWindow::resize(const QSize &size)
{
    if (m_wlShellSurface) {
        m_wlShellSurface->sendConfigure(size, QWaylandWlShellSurface::BottomLeftEdge);
    } else if (m_alienSurface) {
        m_alienSurface->resize(size);
    }
}

void LipstickCompositorWindow::ping()
{
    if (m_wlShellSurface) {
        m_wlShellSurface->ping();
    } else if (QWaylandSurface *surface = m_xdgSurface ? QWaylandQuickItem::surface() : nullptr) {
        m_pingSerial = m_compositor->m_xdgShell->ping(surface->client());
    }
}

void LipstickCompositorWindow::close()
{
    if (m_extSurface) {
        // Qt extension, will only work with Qt applications, difficult to find as it's a
        // code generated member of an undocumented class. Definition is in surface-extension.xml
        m_extSurface->send_close();
    } else if (m_xdgSurface) {
        // Works with applications that support xdg-shell-v5. Qt applications don't by default
        // but can if QT_WAYLAND_SHELL_INTEGRATION=xdg-shell-v5 is exported.
        m_xdgSurface->sendClose();
    } else if (m_alienSurface) {
        m_alienSurface->close();
    } else if (QWaylandSurface *surface = m_wlShellSurface ? QWaylandQuickItem::surface() : nullptr) {
        // This is a somewhat brutal method. It will disconnect the socket connection to the
        // application and Qt at least does not know how to deal with that gracefully and will
        // assert out.
        surface->client()->close();
    }
}

void LipstickCompositorWindow::closePopup()
{
    if (m_wlShellSurface) {
        m_wlShellSurface->sendPopupDone();
    } else if (m_xdgSurface) {
        m_xdgSurface->sendClose();
    } else if (m_extSurface) {
        m_extSurface->send_close();
    } else if (m_alienSurface) {
        m_alienSurface->close();
    }
}

void LipstickCompositorWindow::sendOomScore(int score)
{
    if (m_alienSurface) {
        m_alienSurface->sendOomScore(score);
    }
}

bool LipstickCompositorWindow::isExposed() const
{
    return m_exposed;
}

void LipstickCompositorWindow::setExposed(bool exposed)
{
    if (m_exposed != exposed) {
        m_exposed = exposed;

        if (m_alienSurface) {
            if (m_exposed) {
                m_alienSurface->show(false);
            } else if (m_exposedAsCover) {
                m_alienSurface->show(true);
            } else {
                m_alienSurface->hide();
            }
        }

        emit exposedChanged();
    }
}

bool LipstickCompositorWindow::isExposedAsCover() const
{
    return m_exposedAsCover;
}

void LipstickCompositorWindow::setExposedAsCover(bool exposed)
{
    if (m_exposedAsCover != exposed) {
        m_exposedAsCover = exposed;

        if (m_alienSurface) {
            if (m_exposed) {
                // We don't care about the cover state.
            } else if (m_exposedAsCover) {
                m_alienSurface->show(true);
            } else {
                m_alienSurface->hide();
            }
        }

        emit exposedAsCoverChanged();
    }
}


qint16 LipstickCompositorWindow::windowFlags()
{
    if (m_extSurface) {
        return m_extSurface->windowFlags();
    }
    return 0;
}

WindowPropertyMap *LipstickCompositorWindow::windowProperties()
{
    if (!m_windowProperties) {
        m_windowProperties.reset(new WindowPropertyMap(m_extSurface.data(), surface()));
    }
    return m_windowProperties.data();
}

QVariant LipstickCompositorWindow::windowProperty(const QString &key)
{
    return m_extSurface
            ? WindowPropertyMap::fixupWindowProperty(m_compositor, surface(), m_extSurface->windowProperty(key))
            : QVariant();
}

void LipstickCompositorWindow::setTitle(const QString &title)
{
    if (m_title != title) {
        m_title = title;

        emit titleChanged();

        for (WindowModel *model : LipstickCompositor::instance()->m_windowModels) {
            model->titleChanged(m_windowId);
        }
    }
}

QString LipstickCompositorWindow::title() const
{
    return m_title;
}

QString LipstickCompositorWindow::className() const
{
    return m_wlShellSurface
            ? m_wlShellSurface->className()
            : QString();
}

QRect LipstickCompositorWindow::mouseRegionBounds() const
{
    return m_mouseRegion;
}

void LipstickCompositorWindow::updateMouseRegion(const QVariant &value)
{
    m_explicitMouseRegion = value.isValid();

    const QRect rect = m_explicitMouseRegion
            ? value.value<QRegion>().boundingRect()
            : QRect(0, 0, width(), height());
    if (m_mouseRegion != rect) {
        m_mouseRegion = rect;
        emit mouseRegionBoundsChanged();
    }
}

void LipstickCompositorWindow::updateGrabbedKeys(const QVariant &value)
{
    const QStringList grabbedKeys = value.value<QStringList>();

    if (m_grabbedKeys.isEmpty() && !grabbedKeys.isEmpty()) {
        qApp->installEventFilter(this);
    } else if (!m_grabbedKeys.isEmpty() && grabbedKeys.isEmpty() && m_pressedGrabbedKeys.isEmpty()) {
        // we don't remove the event filter if m_pressedGrabbedKeys contains still some key.
        // we wait the key release for that.
        qApp->removeEventFilter(this);
    }

    m_grabbedKeys.clear();
    for (const QString &key : grabbedKeys) {
        m_grabbedKeys.append(key.toInt());
    }

    if (LipstickCompositor::instance()->debug()) {
        qDebug() << "Window" << windowId() << "grabbed keys changed:" << grabbedKeys;
    }
}


bool LipstickCompositorWindow::isTransient() const
{
    return m_transient;
}

void LipstickCompositorWindow::imageAddref(QQuickItem *item)
{
    Q_ASSERT(!m_refs.contains(item));
    m_refs << item;
}

void LipstickCompositorWindow::imageRelease(QQuickItem *item)
{
    Q_ASSERT(m_refs.contains(item));
    m_refs.remove(m_refs.indexOf(item));
    Q_ASSERT(!m_refs.contains(item));

    tryRemove();
}

bool LipstickCompositorWindow::canRemove() const
{
    return m_windowClosed && !m_delayRemove && m_refs.size() == 0;
}

void LipstickCompositorWindow::tryRemove()
{
    if (canRemove() && !m_removePosted) {
        m_removePosted = true;
        QCoreApplication::postEvent(this, new QEvent(QEvent::User));
    }
}

bool LipstickCompositorWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == window() && m_interceptingTouch) {
        switch (event->type()) {
        case QEvent::TouchUpdate: {
            QTouchEvent *te = static_cast<QTouchEvent *>(event);
            // If we get press/release, don't intercept the event, but send it through QQuickWindow.
            // These are sent through to QQuickWindow so that the integrity of the touch
            // handling is maintained.
            if (te->touchPointStates() & (Qt::TouchPointPressed | Qt::TouchPointReleased))
                return false;
            handleTouchEvent(static_cast<QTouchEvent *>(event));
            return true;
        }
        case QEvent::TouchEnd: // Intentional fall through...
        case QEvent::TouchCancel:
            obj->removeEventFilter(this);
            m_interceptingTouch = false;
        default:
            break;
        }
        return false;
    }

    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        QWaylandSurface *m_surface = surface();
        if (QtWayland::QtKeyExtensionGlobal *ext = m_surface
                    && (m_grabbedKeys.contains(ke->key()) || m_pressedGrabbedKeys.contains(ke->key()))
                    && !ke->isAutoRepeat()
                ? QtWayland::QtKeyExtensionGlobal::findIn(compositor())
                : nullptr) {
            if (event->type() == QEvent::KeyPress) {
                m_pressedGrabbedKeys << ke->key();
            }

            ext->postQtKeyEvent(ke, m_surface);

            if (event->type() == QEvent::KeyRelease) {
                m_pressedGrabbedKeys.removeOne(ke->key());
                if (m_grabbedKeys.isEmpty()) {
                    qApp->removeEventFilter(this);
                }
            }
            return true;
        }
    }

    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        QWaylandSurface *m_surface = surface();
        if (m_surface) {
            QWaylandSeat *inputDevice = m_surface->compositor()->seatFor(ke);
            if (event->type() == QEvent::KeyPress)
                inputDevice->setKeyboardFocus(m_surface);
            inputDevice->sendFullKeyEvent(ke);
            if (event->type() == QEvent::KeyRelease)
                qApp->removeEventFilter(this);
            return true;
        }
    }
    return false;
}

bool LipstickCompositorWindow::isInProcess() const
{
    return false;
}

void LipstickCompositorWindow::itemChange(ItemChange change, const ItemChangeData &data)
{
    if (change == ItemSceneChange) {
        handleTouchCancel();

        if (QQuickWindow *w = window()) {
            disconnect(w, &QQuickWindow::beforeSynchronizing, this, &LipstickCompositorWindow::onSync);
        }

        if (data.window) {
            connect(data.window, &QQuickWindow::beforeSynchronizing, this, &LipstickCompositorWindow::onSync, Qt::DirectConnection);
        }

    }
    QWaylandQuickItem::itemChange(change, data);
}

void LipstickCompositorWindow::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QWaylandQuickItem::geometryChanged(newGeometry, oldGeometry);

    if (!m_explicitMouseRegion) {
        QRect rect(0, 0, width(), height());

        if (m_mouseRegion != rect) {
            m_mouseRegion = rect;
            emit mouseRegionBoundsChanged();
        }
    }
}

bool LipstickCompositorWindow::event(QEvent *e)
{
    bool rv = QWaylandQuickItem::event(e);
    if (e->type() == QEvent::User) {
        m_removePosted = false;
        if (canRemove()) delete this;
    }
    return rv;
}

void LipstickCompositorWindow::mousePressEvent(QMouseEvent *event)
{
    QWaylandSurface *m_surface = surface();
    if (m_surface && m_surface->inputRegionContains(event->pos()) && event->source() != Qt::MouseEventSynthesizedByQt) {
        QWaylandSeat *inputDevice = m_surface->compositor()->seatFor(event);
        QWaylandView *v = view();
        if (inputDevice->mouseFocus() != v) {
            inputDevice->setMouseFocus(v);
            if (m_focusOnTouch && inputDevice->keyboardFocus() != m_surface) {
                takeFocus();
            }
        }
        inputDevice->sendMousePressEvent(event->button());
    } else {
        event->ignore();
    }
}

void LipstickCompositorWindow::mouseMoveEvent(QMouseEvent *event)
{
    QWaylandSurface *m_surface = surface();
    if (m_surface && event->source() != Qt::MouseEventSynthesizedByQt) {
        QWaylandView *v = view();
        QWaylandSeat *inputDevice = m_surface->compositor()->seatFor(event);
        inputDevice->sendMouseMoveEvent(v, event->localPos(), event->globalPos());
    } else {
        event->ignore();
    }
}

void LipstickCompositorWindow::mouseReleaseEvent(QMouseEvent *event)
{
    QWaylandSurface *m_surface = surface();
    if (m_surface && event->source() != Qt::MouseEventSynthesizedByQt) {
        QWaylandSeat *inputDevice = m_surface->compositor()->seatFor(event);
        inputDevice->sendMouseReleaseEvent(event->button());
    } else {
        event->ignore();
    }
}

void LipstickCompositorWindow::wheelEvent(QWheelEvent *event)
{
    QWaylandSurface *m_surface = surface();
    if (m_surface) {
        QWaylandSeat *inputDevice = m_surface->compositor()->seatFor(event);
        inputDevice->sendMouseWheelEvent(event->orientation(), event->delta());
    } else {
        event->ignore();
    }
}

void LipstickCompositorWindow::touchEvent(QTouchEvent *event)
{
    if (touchEventsEnabled() && surface()) {
        handleTouchEvent(event);

        static bool lipstick_touch_interception = qEnvironmentVariableIsEmpty("LIPSTICK_NO_TOUCH_INTERCEPTION");
        if (lipstick_touch_interception && event->type() == QEvent::TouchBegin) {
            // On TouchBegin, start intercepting
            if (event->isAccepted() && !m_interceptingTouch) {
                m_interceptingTouch = true;
                window()->installEventFilter(this);
            }
        }
    } else {
        event->ignore();
    }
}

void LipstickCompositorWindow::handleTouchEvent(QTouchEvent *event)
{
    QList<QTouchEvent::TouchPoint> points = event->touchPoints();

    QWaylandSurface *m_surface = surface();
    if (!m_surface) {
        event->ignore();
        return;
    }

    if (event->touchPointStates() & Qt::TouchPointPressed) {
        foreach (const QTouchEvent::TouchPoint &p, points) {
            if (!m_surface->inputRegionContains(p.pos().toPoint())) {
                event->ignore();
                return;
            }
        }
    }

    QWaylandSeat *inputDevice = m_surface->compositor()->seatFor(event);
    event->accept();

    QWaylandView *vview = view();
    if (vview && (!vview->surface() || vview->surface()->isCursorSurface()))
        vview = Q_NULLPTR;
    inputDevice->setMouseFocus(vview);

    QWaylandView *v = view();
    if (inputDevice->mouseFocus() != v) {
        QPoint pointPos;
        if (!points.isEmpty())
            pointPos = points.at(0).pos().toPoint();
        inputDevice->setMouseFocus(v);

        if (m_focusOnTouch && inputDevice->keyboardFocus() != m_surface) {
            takeFocus();
        }
    }
    inputDevice->sendFullTouchEvent(surface(), event);
}

void LipstickCompositorWindow::handleTouchCancel()
{
    QWaylandSurface *m_surface = surface();
    if (!m_surface)
        return;
    QWaylandSeat *inputDevice = m_surface->compositor()->defaultSeat();
    QWaylandView *v = view();
    if (inputDevice->mouseFocus() == v &&
            (!isVisible() || !isEnabled() || !touchEventsEnabled())) {
        inputDevice->sendTouchCancelEvent(surface()->client());
        inputDevice->setMouseFocus(0);
    }
    if (QWindow *w = window())
        w->removeEventFilter(this);
    m_interceptingTouch = false;
}

void LipstickCompositorWindow::terminateProcess(int killTimeout)
{
    pid_t pid = processId();
    if (pid > 0) {
        kill(pid, SIGTERM);
        QTimer::singleShot(killTimeout, this, SLOT(killProcess()));
    }
}

void LipstickCompositorWindow::killProcess()
{
    pid_t pid = processId();
    if (pid > 0) {
        kill(pid, SIGKILL);
    }
}

static bool hwc_windowsurface_is_enabled();

typedef EGLBoolean (EGLAPIENTRYP Ptr_eglHybrisAcquireNativeBufferWL)(EGLDisplay dpy, struct wl_resource *wlBuffer, EGLClientBuffer *buffer);
typedef EGLBoolean (EGLAPIENTRYP Ptr_eglHybrisNativeBufferHandle)(EGLDisplay dpy, EGLClientBuffer buffer, void **handle);
typedef EGLBoolean (EGLAPIENTRYP Ptr_eglHybrisReleaseNativeBuffer)(EGLClientBuffer buffer);
static Ptr_eglHybrisAcquireNativeBufferWL eglHybrisAcquireNativeBufferWL;
static Ptr_eglHybrisNativeBufferHandle eglHybrisNativeBufferHandle;
static Ptr_eglHybrisReleaseNativeBuffer eglHybrisReleaseNativeBuffer;

class LipstickCompositorWindowHwcNode : public HwcNode
{
public:
    LipstickCompositorWindowHwcNode(QQuickWindow *window) : HwcNode(window) { }
    ~LipstickCompositorWindowHwcNode();

    void update(EGLClientBuffer newBuffer, void *newHandle, QSGNode *contentNode);

    EGLClientBuffer eglBuffer = nullptr;
    QWaylandBufferRef waylandBuffer;
};

class LipstickCompositorWindowNode : public QSGNode
{
public:
    ~LipstickCompositorWindowNode()
    {
        delete hwcNode;
        delete contentNode;
    }

    void destroyHwcNode()
    {
        contentNode->setFlag(QSGNode::OwnedByParent, false);
        delete hwcNode;
        hwcNode = nullptr;

        if (!contentNode->parent()) {
            appendChildNode(contentNode);
        }
    }

    QSGNode *contentNode = nullptr;
    LipstickCompositorWindowHwcNode *hwcNode = nullptr;
};

static bool lcw_checkForVisibleReferences(const QVector<QQuickItem *> &refs)
{
    foreach (QQuickItem *i, refs)
        if (i->opacity() > 0 && i->isVisible())
            return true;
    return false;
}

void LipstickCompositorWindow::onSync()
{
    const bool hasReferences = !isVisible()
                || lcw_checkForVisibleReferences(m_refs)
                || HwcImage::hasEffectReferences(this);
    if (m_hasVisibleReferences != hasReferences) {
        m_hasVisibleReferences = hasReferences;
        update();
    }
}

QSGNode *LipstickCompositorWindow::updatePaintNode(QSGNode *old, UpdatePaintNodeData *data)
{
    if (!hwc_windowsurface_is_enabled()) {
        return QWaylandQuickItem::updatePaintNode(old, data);
    }

    LipstickCompositorWindowNode *node = static_cast<LipstickCompositorWindowNode *>(old);
    QSGNode * const contentNode = QWaylandQuickItem::updatePaintNode(node ? node->contentNode : nullptr, data);

    if (contentNode && !node) {
       node = new LipstickCompositorWindowNode;
    }

    if (node) {
        node->contentNode = contentNode;
    }

    if (!contentNode) {
        delete node;
        return nullptr;
    }

    // qCDebug(LIPSTICK_LOG_HWC, "LipstickCompositorWindow(%p)::updatePaintNode(), old=%p", this, old);

    // If we have visible references, then the texture is being used in a
    // WindowPixmapItem. The hwc logic below would interfere with that, such
    // as texture being destroyed too early so we just use the standard node
    // in this case (and disable HWC which is anyways not needed). Similarily,
    // if we are in 'fallback' mode and there are no more references, we want
    // to switch to the hwc path
    // Added to this logic, we have the case of a window surface suddenly
    // appearing with a shm buffer. We then need to switch to normal
    // composition.
    QWaylandBufferRef buffer = view()->currentBuffer();
    const bool hwBuffer = !m_noHardwareComposition
            && (buffer.bufferFormatEgl() == QWaylandBufferRef::BufferFormatEgl_RGB
             || buffer.bufferFormatEgl() == QWaylandBufferRef::BufferFormatEgl_RGBA);

    // If there is no source node or we don't want a composer node return the node from
    // QWaylandQuickItem now.
    if (!m_hasVisibleReferences || !hwBuffer) {
        node->destroyHwcNode();
        return node;
    }

    // QWaylandQuickItem will return a valid source node after the surface has been destroyed
    // if isBufferLocked() is true. If that happens don't update the compositor node and instead
    // return the last appropriate node.
    wl_resource *surfaceBufferHandle = buffer.hasContent() ? buffer.wl_buffer() : nullptr;
    if (!surfaceBufferHandle) {
        qCDebug(LIPSTICK_LOG_HWC, " - visible with attached 'null' buffer, reusing previous buffer");
        return node;
    }

    // Check if we can extract the HWC handles from the wayland buffer. If
    // not, we disable hardware composition for this window.
    EGLDisplay display = eglGetCurrentDisplay();
    EGLClientBuffer eglBuffer = 0;
    void *hwcHandle = 0;
    if (!eglHybrisAcquireNativeBufferWL(display, surfaceBufferHandle, &eglBuffer)) {
        qCDebug(LIPSTICK_LOG_HWC, " - failed to acquire native buffer (buffers are probably not allocated server-side)");
        m_noHardwareComposition = true;
        node->destroyHwcNode();
        return node;
    }
    eglHybrisNativeBufferHandle(eglGetCurrentDisplay(), eglBuffer, &hwcHandle);
    Q_ASSERT(hwcHandle);

    // At this point we know we are visible and we have access to hwc buffers,
    // make sure we have  an HwcNode instance.
    if (!node->hwcNode) {
        node->hwcNode = new LipstickCompositorWindowHwcNode(window());
    }

    node->hwcNode->waylandBuffer = buffer;

    if (node->contentNode->parent() == node) {
        node->removeChildNode(node->contentNode);
    }

    if (!node->contentNode->parent()) {
        node->hwcNode->appendChildNode(node->contentNode);
    }

    if (!node->hwcNode->parent()) {
        node->appendChildNode(node->hwcNode);
    }

    node->hwcNode->update(eglBuffer, hwcHandle, node->contentNode);

    return node;
}

bool LipstickCompositorWindow::focusOnTouch() const
{
    return m_focusOnTouch;
}

void LipstickCompositorWindow::setFocusOnTouch(bool focusOnTouch)
{
    if (m_focusOnTouch == focusOnTouch)
        return;

    m_focusOnTouch = focusOnTouch;
    emit focusOnTouchChanged();
}

static bool hwc_windowsurface_is_enabled()
{
    if (!HwcRenderStage::isHwcEnabled())
        return false;

    static int checked = 0;
    if (!checked) {
        const char *acquireNativeBufferExtension = "EGL_HYBRIS_WL_acquire_native_buffer";
        const char *nativeBuffer2Extensions = "EGL_HYBRIS_native_buffer2";
        const char *extensions = eglQueryString(eglGetCurrentDisplay(), EGL_EXTENSIONS);

        if (strstr(extensions, acquireNativeBufferExtension) != 0) {
            eglHybrisAcquireNativeBufferWL = (Ptr_eglHybrisAcquireNativeBufferWL) eglGetProcAddress("eglHybrisAcquireNativeBufferWL");
            checked |= 0x1;
        } else {
            qCDebug(LIPSTICK_LOG_HWC, "Missing required EGL extension: '%s'", acquireNativeBufferExtension);
        }
        if (strstr(extensions, nativeBuffer2Extensions) != 0) {
            eglHybrisNativeBufferHandle = (Ptr_eglHybrisNativeBufferHandle) eglGetProcAddress("eglHybrisNativeBufferHandle");
            eglHybrisReleaseNativeBuffer = (Ptr_eglHybrisReleaseNativeBuffer) eglGetProcAddress("eglHybrisReleaseNativeBuffer");
            checked |= 0x2;
        } else {
            qCDebug(LIPSTICK_LOG_HWC, "Missing required EGL extension: '%s'", nativeBuffer2Extensions);
        }

        // If both extensions were found
        if (checked == (0x1 | 0x2)) {
            qCDebug(LIPSTICK_LOG_HWC, "HWC composition of window surfaces is enabled");
        } else {
            qCDebug(LIPSTICK_LOG_HWC, "HWC composition of window surfaces is disabled");
            checked = -1;
        }

    }
    return checked > 0;
}

class LipstickCompositorWindowReleaseEvent : public QEvent
{
public:
    LipstickCompositorWindowReleaseEvent(LipstickCompositorWindowHwcNode *n)
        : QEvent(QEvent::User)
        , eventTarget(n->renderStage())
        , eglBuffer(n->eglBuffer)
        , waylandBuffer(n->waylandBuffer)
    {
    }

    HwcRenderStage *eventTarget;
    EGLClientBuffer eglBuffer;
    QWaylandBufferRef waylandBuffer;
};

void hwc_windowsurface_release_native_buffer(void *, void *callbackData)
{
    LipstickCompositorWindowReleaseEvent *e = (LipstickCompositorWindowReleaseEvent *) callbackData;
    // qCDebug(LIPSTICK_LOG_HWC, " - window surface buffers released: handle=%p, eglBuffer=%post", handle, e->eglBuffer);
    eglHybrisReleaseNativeBuffer(e->eglBuffer);
    e->eglBuffer = 0;

    // This may seem a bit odd, and indeed it is.. We need to release the
    // QWaylandBufferRef on the GUI thread, so we post the event to a QObject
    // which lives on the GUI thread, so the event destructor runs there which
    // will in turn release the wlBuffer there. Since the
    // LipstickCompositorWindow could have been nuked we post it to the
    // HwcRenderStage which is always there.
    QCoreApplication::postEvent(e->eventTarget, e);
}

void LipstickCompositorWindowHwcNode::update(EGLClientBuffer newBuffer, void *newHandle, QSGNode *contentNode)
{
    // If we're taking a new buffer into use when there already was
    // one, set up the old to be removed.
    if (handle() && handle() != newHandle) {
        // qCDebug(LIPSTICK_LOG_HWC, " - releasing old buffer, EGLClientBuffer=%p, gralloc=%p", eglBuffer, handle());
        Q_ASSERT(eglBuffer);
        LipstickCompositorWindowReleaseEvent *e = new LipstickCompositorWindowReleaseEvent(this);
        renderStage()->signalOnBufferRelease(hwc_windowsurface_release_native_buffer, handle(), e);
    }
    // qCDebug(LIPSTICK_LOG_HWC, " - setting buffers on HwcNode, EGLClientBuffer=%p, gralloc=%p", newBuffer, newHandle);
    eglBuffer = newBuffer;

    Q_ASSERT(contentNode->type() == QSGNode::GeometryNodeType);
    HwcNode::update(static_cast<QSGGeometryNode *>(contentNode), newHandle);
}

LipstickCompositorWindowHwcNode::~LipstickCompositorWindowHwcNode()
{
    // qCDebug(LIPSTICK_LOG_HWC, " - window surface node destroyed, node=%p, handle=%p, eglBuffer=%p", this, handle(), eglBuffer);
    Q_ASSERT(handle());
    Q_ASSERT(eglBuffer);
    LipstickCompositorWindowReleaseEvent *e = new LipstickCompositorWindowReleaseEvent(this);
    renderStage()->signalOnBufferRelease(hwc_windowsurface_release_native_buffer, handle(), e);
}

