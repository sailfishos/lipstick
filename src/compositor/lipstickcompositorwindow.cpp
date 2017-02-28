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

#include "hwcimage.h"
#include "hwcrenderstage.h"
#include <EGL/egl.h>
#include <private/qwaylandsurface_p.h>
#include <private/qquickwindow_p.h>
#include <QtWaylandCompositor/private/qwlextendedsurface_p.h>

LipstickCompositorWindow::LipstickCompositorWindow(int windowId, const QString &category,
                                                   QWaylandSurface *surface, QQuickItem *parent)
: QWaylandQuickItem(), m_windowId(windowId), m_category(category),
  m_delayRemove(false), m_windowClosed(false), m_removePosted(false),
  m_interceptingTouch(false), m_mapped(false), m_noHardwareComposition(false),
  m_focusOnTouch(false), m_hasVisibleReferences(false)
{
    setFlags(QQuickItem::ItemIsFocusScope | flags());

    // Handle ungrab situations
    connect(this, SIGNAL(visibleChanged()), SLOT(handleTouchCancel()));
    connect(this, SIGNAL(enabledChanged()), SLOT(handleTouchCancel()));
    connect(this, SIGNAL(touchEventsEnabledChanged()), SLOT(handleTouchCancel()));

    if(surface) {
        connect(surface, SIGNAL(surfaceDestroyed()), this, SLOT(deleteLater()));
        setSurface(surface);
    }
    Q_UNUSED(parent)
    connectSurfaceSignals();
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
    if (m_delayRemove)
        disconnect(surface(), SIGNAL(surfaceDestroyed()), this, SLOT(deleteLater()));
    else
        connect(surface(), SIGNAL(surfaceDestroyed()), this, SLOT(deleteLater()));

    emit delayRemoveChanged();

    tryRemove();
}

QString LipstickCompositorWindow::category() const
{
    return m_category;
}

QtWayland::ExtendedSurface *LipstickCompositorWindow::extendedSurface()
{
    return m_extSurface;
}

void LipstickCompositorWindow::setExtendedSurface(QtWayland::ExtendedSurface *extSurface)
{
    m_extSurface = extSurface;
    connect(m_extSurface, SIGNAL(windowFlagsChanged()), this, SIGNAL(windowFlagsChanged()));
}

qint16 LipstickCompositorWindow::windowFlags()
{
    if (m_extSurface) {
        return m_extSurface->windowFlags();
    }
    return 0;
}

QVariantMap LipstickCompositorWindow::windowProperties()
{
    if (m_extSurface)
        return m_extSurface->windowProperties();
    return QVariantMap();
}

void LipstickCompositorWindow::setTitle(QString title)
{
    m_title = title;
}

QString LipstickCompositorWindow::title() const
{
    return m_title;
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

void LipstickCompositorWindow::connectSurfaceSignals()
{
    foreach (const QMetaObject::Connection &connection, m_surfaceConnections) {
        disconnect(connection);
    }

    m_surfaceConnections.clear();
    if (surface()) {
        m_surfaceConnections << connect(surface(), &QWaylandSurface::configure, this, &LipstickCompositorWindow::committed);
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
    LipstickCompositorWindowHwcNode(QQuickWindow *window) : HwcNode(window), eglBuffer(0) { }
    ~LipstickCompositorWindowHwcNode();

    void update(QWaylandSurfacePrivate *s, EGLClientBuffer newBuffer, void *newHandle, QSGNode *contentNode);

    EGLClientBuffer eglBuffer;
    QWaylandBufferRef waylandBuffer;
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
    if (!hwc_windowsurface_is_enabled() || m_noHardwareComposition)
        return QWaylandQuickItem::updatePaintNode(old, data);

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

                               // TODO: a327ca8d8a1f6e0a44a3aa6bd4dac716911c434e
    bool hwBuffer = surface(); // TODO && surface()->type() == QWaylandSurface::Texture;
    int wantedNodeType = m_hasVisibleReferences || !hwBuffer ? QSGNode::GeometryNodeType : QSG_HWC_NODE_TYPE;
    if (old && old->type() != wantedNodeType) {
        delete old;
        old = 0;
    }
    if (m_hasVisibleReferences || !hwBuffer)
        return QWaylandQuickItem::updatePaintNode(old, data);

    // No surface, abort..
    if (!surface() || !QWaylandSurfacePrivate::get(surface()) || !surface()->hasContent() || !textureProvider()->texture()) {
        delete old;
        return 0;
    }

    // Follow the unmaplock logic inside QtWayland. If a surface is still
    // visible but has a null buffer attached, we retain the copy we have
    // until further notice...
    QWaylandSurfacePrivate *s = QWaylandSurfacePrivate::get(surface());
    wl_resource *surfaceBufferHandle = s->bufferRef.hasBuffer() ? s->bufferRef.wl_buffer() : 0;
    if (!surfaceBufferHandle && surface()->hasContent()) {
        qCDebug(LIPSTICK_LOG_HWC, " - visible with attached 'null' buffer, reusing previous buffer");
        return old;
    }

    // Find the old node if applicable and call updatePaintNode on the wayland
    // surface item. If this results in a null node, we abort.
    LipstickCompositorWindowHwcNode *hwcNode = old ? static_cast<LipstickCompositorWindowHwcNode *>(old) : 0;
    QSGNode *oldContentNode = hwcNode ? hwcNode->firstChild() : 0;
    QSGNode *newContentNode = QWaylandQuickItem::updatePaintNode(oldContentNode, data);
    if (!newContentNode) {
        delete old;
        return 0;
    }

    // Check if we can extract the HWC handles from the wayland buffer. If
    // not, we disable hardware composition for this window.
    EGLDisplay display = eglGetCurrentDisplay();
    EGLClientBuffer eglBuffer = 0;
    void *hwcHandle = 0;
    if (!eglHybrisAcquireNativeBufferWL(display, surfaceBufferHandle, &eglBuffer)) {
        qCDebug(LIPSTICK_LOG_HWC, " - failed to acquire native buffer (buffers are probably not allocated server-side)");
        m_noHardwareComposition = true;
        delete old;
        return QWaylandQuickItem::updatePaintNode(0, data);
    }
    eglHybrisNativeBufferHandle(eglGetCurrentDisplay(), eglBuffer, &hwcHandle);
    Q_ASSERT(hwcHandle);

    // At this point we know we are visible and we have access to hwc buffers,
    // make sure we have  an HwcNode instance.
    if (!hwcNode)
        hwcNode = new LipstickCompositorWindowHwcNode(window());

    // Add the new content node. The old one, if present would already have
    // been removed because it was deleted in the
    // QWaylandQuickItem::updatePaintNode() call above.
    if (newContentNode != hwcNode->firstChild())
        hwcNode->appendChildNode(newContentNode);

    hwcNode->update(s, eglBuffer, hwcHandle, newContentNode);
    return hwcNode;
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

void LipstickCompositorWindowHwcNode::update(QWaylandSurfacePrivate *s, EGLClientBuffer newBuffer, void *newHandle, QSGNode *contentNode)
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
    waylandBuffer = QWaylandBufferRef(s->bufferRef);

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

