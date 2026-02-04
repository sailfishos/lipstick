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

#include <QtCompositorVersion>

#include <QtCompositor/QWaylandClient>
#include <QtCompositor/QWaylandSurface>

#include <private/qwlsurface_p.h>

#include "xdgshell.h"
#include "xdgpositioner.h"
#include "xdgsurface.h"
#include "xdgpopup.h"

XdgPopup::XdgPopup(XdgSurface *xdgSurface, const QRect &rect, uint32_t version, uint32_t id)
    : QObject(xdgSurface)
    , QWaylandSurfaceInterface(xdgSurface->surface())
    , QtWaylandServer::xdg_popup(surface()->client()->client(), id, version)
    , m_xdgSurface(xdgSurface)
    , m_rect(rect)
{
    setSurfaceType(QWaylandSurface::Popup);

    XdgSurface *xdgParent = xdgSurface->xdgParent();
    surface()->handle()->setTransientParent(xdgParent->surface()->handle());
    updateOffset();
    sendConfigure();

    connect(xdgParent, &XdgSurface::geometryChanged, this, &XdgPopup::updateOffset);
    connect(xdgSurface, &XdgSurface::geometryChanged, this, &XdgPopup::updateOffset);
    connect(surface(), &QWaylandSurface::configure, this, &XdgPopup::configure);
}

XdgPopup::~XdgPopup()
{
    wl_resource_set_implementation(resource()->handle, nullptr, nullptr, nullptr);
    surface()->setMapped(false);
    m_xdgSurface->roleDestroyed();
}

bool XdgPopup::runOperation(QWaylandSurfaceOp *op)
{
    switch (op->type()) {
        case QWaylandSurfaceOp::Close:
            send_popup_done();
            return true;
        case QWaylandSurfaceOp::Ping:
            m_xdgSurface->manager()->ping(static_cast<QWaylandSurfacePingOp *>(op)->serial(), surface());
            return true;
        default:
            break;
    }
    return false;
}

void XdgPopup::xdg_popup_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource);
    delete this;
}

void XdgPopup::xdg_popup_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void XdgPopup::xdg_popup_reposition(Resource *resource, ::wl_resource *positioner, uint32_t token)
{
    Q_UNUSED(resource);
    m_rect = XdgPositioner::fromResource(positioner)->toRect(m_xdgSurface->xdgParent());
    updateOffset();
    send_repositioned(token);
    sendConfigure();
}

void XdgPopup::configure(bool hasBuffer)
{
    surface()->setMapped(hasBuffer);
}

void XdgPopup::updateOffset()
{
    const QRect &geometry = m_xdgSurface->geometry();
    const QRect &parentGeometry = m_xdgSurface->xdgParent()->geometry();

    int x = parentGeometry.left() + m_rect.left() - geometry.left();
    int y = parentGeometry.top() + m_rect.top() - geometry.top();

    surface()->handle()->setTransientOffset(x, y);
}

void XdgPopup::sendConfigure()
{
    send_configure(m_rect.left(), m_rect.top(), m_rect.width(), m_rect.height());
    m_xdgSurface->sendConfigure();
}
