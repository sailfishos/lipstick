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
    : QWaylandCompositorExtensionTemplate(compositor)
{
}

AlienManager::~AlienManager()
{
}

void AlienManager::initialize()
{
    QWaylandCompositorExtensionTemplate::initialize();
    QWaylandCompositor *compositor = static_cast<QWaylandCompositor *>(extensionContainer());
    init(compositor->display(), 1);
}

const wl_interface *AlienManager::interface()
{
    return &alien_manager_interface;
}

QByteArray AlienManager::interfaceName()
{
    return QByteArrayLiteral("alien_manager");
}

void AlienManager::ping(uint32_t serial, AlienSurface *surface)
{
    qDebug() << Q_FUNC_INFO;
    m_pings.insert(serial, surface);
    send_ping(serial);
}

void AlienManager::alien_manager_destroy(Resource *resource)
{
    qDebug() << Q_FUNC_INFO;
    wl_resource_destroy(resource->handle);
}

void AlienManager::alien_manager_create_alien_client(Resource *resource, uint32_t id, const QString &package)
{
    qDebug() << Q_FUNC_INFO;
    new AlienClient(this, resource->client(), wl_resource_get_version(resource->handle), id, package);
}

void AlienManager::alien_manager_pong(Resource *resource, uint32_t serial)
{
    qDebug() << Q_FUNC_INFO;
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
    qDebug() << Q_FUNC_INFO;
}

const struct wl_interface *AlienClient::interface()
{
    return &alien_client_interface;
}

QByteArray AlienClient::interfaceName()
{
    return QByteArrayLiteral("alien_client");
}

AlienManager *AlienClient::manager() const
{
    return m_manager;
}

void AlienClient::alien_client_destroy(Resource *resource)
{
    qDebug() << Q_FUNC_INFO;
    wl_resource_destroy(resource->handle);
}

void AlienClient::alien_client_get_alien_surface(
        Resource *resource, uint32_t id, ::wl_resource *surface_resource)
{
    qDebug() << Q_FUNC_INFO;
    QWaylandSurface *surface = QWaylandSurface::fromResource(surface_resource);

    AlienSurface *alienSurface = new AlienSurface(
                resource->client(), id, wl_resource_get_version(resource->handle), surface, this, m_package);

    LipstickCompositor::instance()->onAlienSurfaceCreated(alienSurface, surface);
}
