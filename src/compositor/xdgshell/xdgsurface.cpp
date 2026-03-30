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

#include <QtCompositor/QWaylandClient>
#include <QtCompositor/QWaylandCompositor>
#include <QtCompositor/QWaylandSurface>

#include "lipsticksurfaceinterface.h"
#include "xdgshell.h"
#include "xdgpositioner.h"
#include "xdgsurface.h"
#include "xdgtoplevel.h"
#include "xdgpopup.h"

XdgSurface::XdgSurface(XdgShell *mgr, QWaylandSurface *surface, uint32_t version, uint32_t id)
    : QObject(mgr)
    , QWaylandSurfaceInterface(surface)
    , QtWaylandServer::xdg_surface(surface->client()->client(), id, version)
    , m_manager(mgr)
    , m_popupParent(nullptr)
    , m_toplevel(nullptr)
    , m_serial(0)
    , m_ackPending(false)
{
    connect(surface, &QWaylandSurface::configure, this, &XdgSurface::handleCommit);
    surface->setProperty("xdgSurface", true);
}

XdgSurface::~XdgSurface()
{
    wl_resource_set_implementation(resource()->handle, nullptr, nullptr, nullptr);
}

QRect XdgSurface::popupArea() const
{
    if (m_toplevel) {
        return m_toplevel->popupArea();
    }

    if (m_popupParent) {
        return m_popupParent->popupArea().translated(-surface()->transientOffset().toPoint());
    }

    return QRect();
}

void XdgSurface::sendConfigure()
{
    m_serial = wl_display_next_serial(surface()->compositor()->waylandDisplay());
    send_configure(m_serial);
}

void XdgSurface::roleDestroyed()
{
    setParent(m_manager);
    m_popupParent = nullptr;
    m_toplevel = nullptr;
    m_geometry = QRect();
    m_pendingGeometry = QRect();
}

bool XdgSurface::runOperation(QWaylandSurfaceOp *op)
{
    switch (op->type()) {
    case QWaylandSurfaceOp::Ping:
        m_manager->ping(static_cast<QWaylandSurfacePingOp *>(op)->serial(), surface());
        return true;
    case LipstickGetShellStateOp::Type:
        static_cast<LipstickGetShellStateOp *>(op)->m_resizeAcked = m_ackPending;
        m_ackPending = false;
        return true;
    default:
        break;
    }
    return false;
}

void XdgSurface::xdg_surface_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource)
    delete this;
}

void XdgSurface::xdg_surface_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void XdgSurface::xdg_surface_get_toplevel(Resource *resource, uint32_t id)
{
    if (m_popupParent || m_toplevel)
        return;

    m_toplevel = new XdgToplevel(this, wl_resource_get_version(resource->handle), id);
}

void XdgSurface::xdg_surface_get_popup(Resource *resource, uint32_t id, ::wl_resource *parent, ::wl_resource *positioner)
{
    Q_UNUSED(resource)

    if (!parent || m_popupParent || m_toplevel)
        return;

    m_popupParent = static_cast<XdgSurface *>(Resource::fromResource(parent)->xdg_surface_object);
    setParent(m_popupParent);

    QRect pos = XdgPositioner::fromResource(positioner)->toRect(m_popupParent);
    new XdgPopup(this, pos, wl_resource_get_version(resource->handle), id);
}

void XdgSurface::xdg_surface_set_window_geometry(Resource *resource, int32_t x, int32_t y, int32_t width, int32_t height)
{
    Q_UNUSED(resource)
    m_pendingGeometry.setRect(x, y, width, height);
}

void XdgSurface::xdg_surface_ack_configure(Resource *resource, uint32_t serial)
{
    Q_UNUSED(resource)

    if (serial == m_serial) {
        m_serial = 0;
        m_ackPending = true;
    }
}

void XdgSurface::handleCommit()
{
    if (m_geometry != m_pendingGeometry) {
        m_geometry = m_pendingGeometry;
        emit geometryChanged();
    }
}
