/***************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Giulio Camuffo <giulio.camuffo@jollamobile.com>
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

#include "alienmanager.h"
#include "aliensurface.h"

#include <QWaylandCompositor>

#include <lipstickcompositor.h>

AlienManager::AlienManager(QWaylandCompositor *compositor)
    : QWaylandShellTemplate(compositor)
    , QtWaylandServer::alien_manager(compositor->display(), 1)
{
}

AlienManager::~AlienManager()
{
}

void AlienManager::ping(uint32_t serial, AlienSurface *surface)
{
    if (Resource *resource = resourceMap().value(surface->surface()->client()->client())) {
        m_pings.insert(serial, surface);
        send_ping(resource->handle, serial);
    }
}

void AlienManager::alien_manager_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void AlienManager::alien_manager_create_alien_client(Resource *resource, uint32_t id, const QString &package)
{
    new AlienClient(this, resource->client(), wl_resource_get_version(resource->handle), id, package);
}

void AlienManager::alien_manager_pong(Resource *resource, uint32_t serial)
{
    Q_UNUSED(resource);

    AlienSurface * const surface = m_pings.take(serial);
    if (surface)
        surface->pong();
}

AlienClient::AlienClient(AlienManager *mgr, wl_client *client, uint32_t version, uint32_t id, const QString &package)
           : QWaylandCompositorExtensionTemplate<AlienManager>()
           , QtWaylandServer::alien_client(client, id, version)
           , m_package(package)
           , m_manager(mgr)
{
}

AlienClient::~AlienClient()
{
}

AlienManager *AlienClient::manager() const
{
    return m_manager;
}

void AlienClient::alien_client_destroy(Resource *resource)
{
    delete this;
}

void AlienClient::alien_client_get_alien_surface(
        Resource *resource, uint32_t id, ::wl_resource *surface_resource)
{
    QWaylandSurface *surface = QWaylandSurface::fromResource(surface_resource);

    AlienSurface *alienSurface = new AlienSurface(
                resource->client(), id, wl_resource_get_version(resource->handle), surface, this, m_package);

    LipstickCompositor::instance()->onAlienSurfaceCreated(alienSurface, surface);
}
