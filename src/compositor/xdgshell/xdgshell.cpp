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

#include <QtCompositor/QWaylandSurface>

#include "xdgshell.h"
#include "xdgpositioner.h"
#include "xdgsurface.h"

XdgShellGlobal::XdgShellGlobal(QObject *parent)
    : QObject(parent)
{
}

const wl_interface *XdgShellGlobal::interface() const
{
    return &xdg_wm_base_interface;
}

void XdgShellGlobal::bind(wl_client *client, uint32_t version, uint32_t id)
{
    new XdgShell(client, version, id, this);
}

XdgShell::XdgShell(wl_client *client, uint32_t version, uint32_t id, QObject *parent)
    : QObject(parent)
    , QtWaylandServer::xdg_wm_base(client, id, version)
{
}

XdgShell::~XdgShell()
{
    wl_resource_set_implementation(resource()->handle, nullptr, nullptr, nullptr);
}

void XdgShell::ping(uint32_t serial, QWaylandSurface *surface)
{
    m_pings.insert(serial, surface);
    send_ping(serial);
}

void XdgShell::xdg_wm_base_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource)
    delete this;
}

void XdgShell::xdg_wm_base_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void XdgShell::xdg_wm_base_create_positioner(Resource *resource, uint32_t id)
{
    new XdgPositioner(this, resource->client(), wl_resource_get_version(resource->handle), id);
}

void XdgShell::xdg_wm_base_get_xdg_surface(Resource *resource, uint32_t id, ::wl_resource *surface)
{
    QWaylandSurface *surf = QWaylandSurface::fromResource(surface);
    new XdgSurface(this, surf, wl_resource_get_version(resource->handle), id);
}

void XdgShell::xdg_wm_base_pong(Resource *resource, uint32_t serial)
{
    Q_UNUSED(resource)
    QWaylandSurface *surf = m_pings.value(serial);
    if (surf)
        surf->pong();
}
