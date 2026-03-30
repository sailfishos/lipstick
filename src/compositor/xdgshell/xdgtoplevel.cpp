/***************************************************************************
**
** Copyright (c) 2026 Jolla Mobile Ltd
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

#include <QtCompositor/QWaylandClient>
#include <QtCompositor/QWaylandSurface>

#include <private/qwlsurface_p.h>

#include "lipstickcompositor.h"
#include "lipsticksurfaceinterface.h"
#include "xdgshell.h"
#include "xdgsurface.h"
#include "xdgtoplevel.h"

XdgToplevel::XdgToplevel(XdgSurface *xdgSurface, uint32_t version, uint32_t id)
    : QObject(xdgSurface)
    , QWaylandSurfaceInterface(xdgSurface->surface())
    , QtWaylandServer::xdg_toplevel(surface()->client()->client(), id, version)
    , m_xdgSurface(xdgSurface)
    , m_parentToplevel(nullptr)
    , m_hidden(false)
    , m_coverized(false)
{
    setSurfaceType(QWaylandSurface::Toplevel);

    LipstickCompositor *compositor = LipstickCompositor::instance();
    m_size = QSize(compositor->width(), compositor->height());
    sendConfigure();

    connect(xdgSurface, &XdgSurface::geometryChanged, this, &XdgToplevel::geometryChanged);
    connect(surface(), &QWaylandSurface::configure, this, &XdgToplevel::configure);
}

XdgToplevel::~XdgToplevel()
{
    wl_resource_set_implementation(resource()->handle, nullptr, nullptr, nullptr);
    surface()->setMapped(false);
    m_xdgSurface->roleDestroyed();
}

bool XdgToplevel::runOperation(QWaylandSurfaceOp *op)
{
    switch (op->type()) {
    case QWaylandSurfaceOp::Close:
        send_close();
        return true;
    case QWaylandSurfaceOp::SetVisibility: {
        QWindow::Visibility v = static_cast<QWaylandSurfaceSetVisibilityOp *>(op)->visibility();
        m_hidden = v == QWindow::Hidden;
        m_coverized = v == QWindow::Minimized;
        sendConfigure();
        return true;
    }
    case QWaylandSurfaceOp::Resize:
        m_size = static_cast<QWaylandSurfaceResizeOp *>(op)->size();
        sendConfigure();
        return true;
    case LipstickSetPopupAreaOp::Type:
        setPopupArea(static_cast<LipstickSetPopupAreaOp *>(op)->bounds());
        return true;
    default:
        break;
    }
    return false;
}

void XdgToplevel::xdg_toplevel_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource)
    delete this;
}

void XdgToplevel::xdg_toplevel_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void XdgToplevel::xdg_toplevel_set_parent(Resource *resource, ::wl_resource *parent)
{
    Q_UNUSED(resource)
    setParentToplevel(parent
        ? static_cast<XdgToplevel *>(Resource::fromResource(parent)->xdg_toplevel_object)
        : nullptr);
}

void XdgToplevel::xdg_toplevel_set_title(Resource *resource, const QString &title)
{
    Q_UNUSED(resource)
    setSurfaceTitle(title);
}

void XdgToplevel::xdg_toplevel_set_app_id(Resource *resource, const QString &appId)
{
    Q_UNUSED(resource)
    setSurfaceClassName(appId);
}

void XdgToplevel::xdg_toplevel_set_minimized(Resource *resource)
{
    Q_UNUSED(resource)
    emit surface()->lowerRequested();
}

void XdgToplevel::configure(bool hasBuffer)
{
    if (hasBuffer && m_xdgSurface->geometry().isNull()) {
        m_size = surface()->size();
    }
    surface()->setMapped(hasBuffer);
}

void XdgToplevel::sendConfigure()
{
    QVector<uint32_t> states;
    states << XDG_TOPLEVEL_STATE_MAXIMIZED;
    if (!m_coverized && !m_hidden) {
        states << XDG_TOPLEVEL_STATE_ACTIVATED;
    }
    QByteArray data = QByteArray::fromRawData((char *)states.data(), states.size() * sizeof(uint32_t));

    send_configure(m_size.width(), m_size.height(), data);
    m_xdgSurface->sendConfigure();
}

void XdgToplevel::geometryChanged()
{
    if (m_xdgSurface->geometry().isValid()) {
        m_size = m_xdgSurface->geometry().size();
    }
}

void XdgToplevel::parentPopupAreaChanged(const QRect &bounds)
{
    setPopupArea(bounds);
    surface()->handle()->setTransientOffset(bounds.x(), bounds.y());
    m_size = bounds.size();
    sendConfigure();
}

void XdgToplevel::parentUnmapped()
{
    setParentToplevel(m_parentToplevel->m_parentToplevel);
}

void XdgToplevel::setPopupArea(const QRect &bounds)
{
    if (m_popupArea != bounds) {
        m_popupArea = bounds;
        emit popupAreaChanged(bounds);
    }
}

void XdgToplevel::setParentToplevel(XdgToplevel *parent)
{
    if (m_parentToplevel == parent)
        return;

    if (m_parentToplevel) {
        disconnect(m_parentToplevel, &XdgToplevel::popupAreaChanged,
                   this, &XdgToplevel::parentPopupAreaChanged);
        disconnect(m_parentToplevel->surface(), &QWaylandSurface::unmapped,
                   this, &XdgToplevel::parentUnmapped);
    }

    m_parentToplevel = parent;

    bool mapped = surface()->isMapped();
    surface()->setMapped(false);

    if (parent) {
        parentPopupAreaChanged(parent->popupArea());

        QWaylandSurface *parentSurface = parent->surface();
        surface()->handle()->setTransientParent(parentSurface->handle());

        connect(parent, &XdgToplevel::popupAreaChanged,
                this, &XdgToplevel::parentPopupAreaChanged);
        connect(parentSurface, &QWaylandSurface::unmapped,
                this, &XdgToplevel::parentUnmapped);
    } else {
        surface()->handle()->setTransientParent(nullptr);
    }

    surface()->setMapped(mapped);
}
