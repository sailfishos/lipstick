/***************************************************************************
**
** Copyright (c) 2013 - 2018 Jolla Ltd.
** Copyright (c) 2020 Open Mobile Platform LLC.
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

#include <QtCompositorVersion>

#include <QCoreApplication>
#include <QWaylandCompositor>
#include <QWaylandInputDevice>
#include <QWaylandClient>
#include <QTimer>
#include <sys/types.h>
#include <signal.h>
#include "lipstickcompositor.h"
#include "lipstickcompositorwindow.h"


LipstickCompositorWindow::LipstickCompositorWindow(int windowId, const QString &category,
                                                   QWaylandQuickSurface *surface, QQuickItem *parent)
    : QWaylandSurfaceItem(surface, parent)
    , m_processId(0)
    , m_windowId(windowId)
    , m_isAlien(false)
    , m_category(category)
    , m_delayRemove(false)
    , m_windowClosed(false)
    , m_removePosted(false)
    , m_mouseRegionValid(false)
    , m_interceptingTouch(false)
    , m_mapped(false)
    , m_focusOnTouch(false)
{
    setFlags(QQuickItem::ItemIsFocusScope | flags());
    refreshMouseRegion();

    // Handle ungrab situations
    connect(this, SIGNAL(visibleChanged()), SLOT(handleTouchCancel()));
    connect(this, SIGNAL(enabledChanged()), SLOT(handleTouchCancel()));
    connect(this, SIGNAL(touchEventsEnabledChanged()), SLOT(handleTouchCancel()));
    connect(this, &QWaylandSurfaceItem::surfaceDestroyed, this, [this]() {
        m_windowClosed = true;
        tryRemove();
    });

    if (surface) {
        m_processId = surface->client()->processId();

        m_isAlien = surface->property("alienSurface").toBool();

        connect(surface, &QWaylandSurface::clientDestroyedSurface, this, &LipstickCompositorWindow::closed);

        connect(surface, &QWaylandSurface::titleChanged, this, &LipstickCompositorWindow::titleChanged);
        connect(surface, &QWaylandSurface::configure, this, &LipstickCompositorWindow::committed);
    }

    updatePolicyApplicationId();
}

LipstickCompositorWindow::~LipstickCompositorWindow()
{
    // We don't want tryRemove() posting an event anymore, we're dying anyway
    m_removePosted = true;
    LipstickCompositor::instance()->windowDestroyed(this);
}

void LipstickCompositorWindow::updatePolicyApplicationId()
{
    if (m_processId <= 0) {
        return;
    }

    QString statFile = QString::fromLatin1("/proc/%1/stat").arg(m_processId);
    QFile f(statFile);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << Q_FUNC_INFO << "Cannot open proc stat for" << statFile;
        return;
    }

    // stat line values are split by ' ' in /proc/*/stat
    QByteArray data = f.readAll();
    QList<QByteArray> statFields = data.split(' ');

    // starttime is field 22, format %lld in stat, see man page for details.
    if (statFields.count() < 22) {
        qWarning() << Q_FUNC_INFO << "fields count < 22";
        return;
    }

    QString starttime = QString::fromUtf8(statFields.at(21));
    bool ok;
    qint64 value = starttime.toLongLong(&ok, 10);

    if (!ok) {
        qWarning() << Q_FUNC_INFO << "toLongLong not ok:" << starttime;
        return;
    }

    // format value as hex
    m_policyApplicationId = QString("%1").arg(value, 0, 16);
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
    return m_processId;
}

QString LipstickCompositorWindow::policyApplicationId() const
{
    return m_policyApplicationId;
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

QString LipstickCompositorWindow::title() const
{
    if (surface())
        return surface()->title();
    return QString();
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

QRect LipstickCompositorWindow::mouseRegionBounds() const
{
    if (m_mouseRegionValid)
        return m_mouseRegion.boundingRect();
    else
        return QRect(0, 0, width(), height());
}

void LipstickCompositorWindow::refreshMouseRegion()
{
    QWaylandSurface *s = surface();
    if (s) {
        QVariantMap properties = s->windowProperties();
        if (properties.contains(QLatin1String("MOUSE_REGION"))) {
            m_mouseRegion = s->windowProperties().value("MOUSE_REGION").value<QRegion>();
            m_mouseRegionValid = true;
            if (LipstickCompositor::instance()->debug())
                qDebug() << "Window" << windowId() << "mouse region set:" << m_mouseRegion;
        } else {
            m_mouseRegionValid = false;
            if (LipstickCompositor::instance()->debug())
                qDebug() << "Window" << windowId() << "mouse region cleared";
        }

        emit mouseRegionBoundsChanged();
    }
}

void LipstickCompositorWindow::refreshGrabbedKeys()
{
    QWaylandSurface *s = surface();
    if (s) {
        const QStringList grabbedKeys = s->windowProperties().value(
                    QLatin1String("GRABBED_KEYS")).value<QStringList>();

        if (m_grabbedKeys.isEmpty() && !grabbedKeys.isEmpty()) {
            qApp->installEventFilter(this);
        } else if (!m_grabbedKeys.isEmpty() && grabbedKeys.isEmpty() && m_pressedGrabbedKeys.keys.isEmpty()) {
            // we don't remove the event filter if m_pressedGrabbedKeys.keys contains still some key.
            // we wait the key release for that.
            qApp->removeEventFilter(this);
        }

        m_grabbedKeys.clear();
        foreach (const QString &key, grabbedKeys)
            m_grabbedKeys.append(key.toInt());

        if (LipstickCompositor::instance()->debug())
            qDebug() << "Window" << windowId() << "grabbed keys changed:" << grabbedKeys;
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

        if (m_surface
                && (m_grabbedKeys.contains(ke->key()) || m_pressedGrabbedKeys.keys.contains(ke->key()))
                && !ke->isAutoRepeat()) {
            QWaylandInputDevice *inputDevice = m_surface->compositor()->defaultInputDevice();
            if (event->type() == QEvent::KeyPress) {
                if (m_pressedGrabbedKeys.keys.isEmpty()) {
                    QWaylandSurface *old = inputDevice->keyboardFocus();
                    m_pressedGrabbedKeys.oldFocus = old;
                    inputDevice->setKeyboardFocus(m_surface);
                }
                m_pressedGrabbedKeys.keys << ke->key();
            }
            inputDevice->sendFullKeyEvent(ke);
            if (event->type() == QEvent::KeyRelease) {
                m_pressedGrabbedKeys.keys.removeOne(ke->key());
                if (m_pressedGrabbedKeys.keys.isEmpty()) {
                    inputDevice->setKeyboardFocus(m_pressedGrabbedKeys.oldFocus.data());
                    if (m_grabbedKeys.isEmpty())
                        qApp->removeEventFilter(this);
                }
            }
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
    }
    QWaylandSurfaceItem::itemChange(change, data);
}

bool LipstickCompositorWindow::event(QEvent *e)
{
    bool rv = QWaylandSurfaceItem::event(e);
    if (e->type() == QEvent::User) {
        m_removePosted = false;
        if (canRemove()) delete this;
    }
    return rv;
}

void LipstickCompositorWindow::mousePressEvent(QMouseEvent *event)
{
    QWaylandSurface *m_surface = surface();
    if (m_surface
            && (!m_mouseRegionValid || m_mouseRegion.contains(event->pos()))
            && m_surface->inputRegionContains(event->pos()) && event->source() != Qt::MouseEventSynthesizedByQt) {
        QWaylandInputDevice *inputDevice = m_surface->compositor()->defaultInputDevice();

        if (inputDevice->mouseFocus() != this) {
            inputDevice->setMouseFocus(this, event->pos(), event->globalPos());
            if (m_focusOnTouch && inputDevice->keyboardFocus() != m_surface) {
                takeFocus();
            }
        }
        inputDevice->sendMousePressEvent(event->button(), event->pos(), event->globalPos());
    } else {
        event->ignore();
    }
}

void LipstickCompositorWindow::mouseMoveEvent(QMouseEvent *event)
{
    QWaylandSurface *m_surface = surface();
    if (m_surface && event->source() != Qt::MouseEventSynthesizedByQt) {
        QWaylandInputDevice *inputDevice = m_surface->compositor()->defaultInputDevice();
        inputDevice->sendMouseMoveEvent(this, event->pos(), event->globalPos());
    } else {
        event->ignore();
    }
}

void LipstickCompositorWindow::mouseReleaseEvent(QMouseEvent *event)
{
    QWaylandSurface *m_surface = surface();
    if (m_surface && event->source() != Qt::MouseEventSynthesizedByQt) {
        QWaylandInputDevice *inputDevice = m_surface->compositor()->defaultInputDevice();
        inputDevice->sendMouseReleaseEvent(event->button(), event->pos(), event->globalPos());
    } else {
        event->ignore();
    }
}

void LipstickCompositorWindow::wheelEvent(QWheelEvent *event)
{
    QWaylandSurface *m_surface = surface();
    if (m_surface
            && (!m_mouseRegionValid || m_mouseRegion.contains(event->pos()))
            && m_surface->inputRegionContains(event->pos()) && event->source() != Qt::MouseEventSynthesizedByQt) {
        QWaylandInputDevice *inputDevice = m_surface->compositor()->defaultInputDevice();
        if (inputDevice->mouseFocus() != this) {
            inputDevice->setMouseFocus(this, event->pos(), event->globalPos());
            if (m_focusOnTouch && inputDevice->keyboardFocus() != m_surface) {
                takeFocus();
            }
        }
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
            if ((m_mouseRegionValid && !m_mouseRegion.contains(p.pos().toPoint()))
                    || !m_surface->inputRegionContains(p.pos().toPoint())) {
                event->ignore();
                return;
            }
        }
    }

    QWaylandInputDevice *inputDevice = m_surface->compositor()->defaultInputDevice();
    event->accept();

    if (inputDevice->mouseFocus() != this) {
        QPoint pointPos;
        if (!points.isEmpty())
            pointPos = points.at(0).pos().toPoint();
        inputDevice->setMouseFocus(this, pointPos, pointPos);

        if (m_focusOnTouch && inputDevice->keyboardFocus() != m_surface) {
            takeFocus();
        }
    }
    inputDevice->sendFullTouchEvent(event);
}

void LipstickCompositorWindow::handleTouchCancel()
{
    QWaylandSurface *m_surface = surface();
    if (!m_surface)
        return;
    QWaylandInputDevice *inputDevice = m_surface->compositor()->defaultInputDevice();
    if (inputDevice->mouseFocus() == this
            && (!isVisible() || !isEnabled() || !touchEventsEnabled())) {
        inputDevice->sendTouchCancelEvent();
        inputDevice->setMouseFocus(0, QPointF());
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

void LipstickCompositorWindow::resize(const QSize &size)
{
    if (surface()->size() != size) {
        surface()->requestSize(size);
        emit resized();
    }
}
