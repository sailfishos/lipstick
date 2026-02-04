/***************************************************************************
**
** Copyright (c) 2026 Affe Null <affenull2345@gmail.com>
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
    , m_xdgParent(nullptr)
    , m_toplevel(nullptr)
    , m_serial(0)
{
    connect(surface, &QWaylandSurface::configure, this, &XdgSurface::handleCommit);
}

XdgSurface::~XdgSurface()
{
    wl_resource_set_implementation(resource()->handle, nullptr, nullptr, nullptr);
}

XdgShell *XdgSurface::manager() const
{
    return m_manager;
}

uint32_t XdgSurface::serial() const
{
    return m_serial;
}

XdgSurface *XdgSurface::xdgParent() const
{
    return m_xdgParent;
}

const QRect& XdgSurface::geometry() const
{
    return m_geometry;
}

XdgToplevel *XdgSurface::findToplevel(QPointF *offset) const
{
    if (m_toplevel) {
        return m_toplevel;
    }

    if (m_xdgParent) {
        *offset -= surface()->transientOffset();
        return m_xdgParent->findToplevel(offset);
    }

    return nullptr;
}

void XdgSurface::sendConfigure()
{
    m_serial = wl_display_next_serial(surface()->compositor()->waylandDisplay());
    send_configure(m_serial);
}

void XdgSurface::roleDestroyed()
{
    setParent(m_manager);
    m_xdgParent = nullptr;
    m_toplevel = nullptr;
    m_geometry = QRect();
    m_pendingGeometry = QRect();
}

bool XdgSurface::runOperation(QWaylandSurfaceOp *op)
{
    Q_UNUSED(op);
    return false;
}

void XdgSurface::xdg_surface_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource);
    delete this;
}

void XdgSurface::xdg_surface_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void XdgSurface::xdg_surface_get_toplevel(Resource *resource, uint32_t id)
{
    if (m_xdgParent || m_toplevel)
        return;

    m_toplevel = new XdgToplevel(this, wl_resource_get_version(resource->handle), id);
}

void XdgSurface::xdg_surface_get_popup(Resource *resource, uint32_t id, ::wl_resource *parent, ::wl_resource *positioner)
{
    Q_UNUSED(resource);

    if (!parent || m_xdgParent || m_toplevel)
        return;

    m_xdgParent = static_cast<XdgSurface *>(Resource::fromResource(parent)->xdg_surface_object);
    setParent(m_xdgParent);

    QRect pos = XdgPositioner::fromResource(positioner)->toRect(m_xdgParent);
    new XdgPopup(this, pos, wl_resource_get_version(resource->handle), id);
}

void XdgSurface::xdg_surface_set_window_geometry(Resource *resource, int32_t x, int32_t y, int32_t width, int32_t height)
{
    Q_UNUSED(resource);
    m_pendingGeometry.setRect(x, y, width, height);
}

void XdgSurface::xdg_surface_ack_configure(Resource *resource, uint32_t serial)
{
    Q_UNUSED(resource);
    if (serial == m_serial) {
        m_serial = 0;
    }
}

void XdgSurface::handleCommit()
{
    if (m_geometry != m_pendingGeometry) {
        m_geometry = m_pendingGeometry;
        emit geometryChanged();
    }
}
