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

#ifndef ALIENMANAGER_H
#define ALIENMANAGER_H

#include <QWaylandCompositorExtension>

#include "qwayland-server-alien-manager.h"

class AlienClient;
class AlienSurface;

class AlienManager : public QWaylandCompositorExtensionTemplate<AlienManager>, public QtWaylandServer::alien_manager
{
public:
    AlienManager(QWaylandCompositor *compositor);
    ~AlienManager();

    void initialize() override;

    static const struct wl_interface *interface();
    static QByteArray interfaceName();

    void ping(uint32_t serial, AlienSurface *surface);

protected:
    void alien_manager_destroy(Resource *resource) override;
    void alien_manager_create_alien_client(Resource *resource, uint32_t id, const QString &package) override;
    void alien_manager_pong(Resource *resource, uint32_t serial) override;

private:
    QMap<uint32_t, AlienSurface *> m_pings;
};

class AlienClient : public QWaylandCompositorExtensionTemplate<AlienManager>, public QtWaylandServer::alien_client
{
public:
    AlienClient(AlienManager *mgr, wl_client *client, uint32_t version, uint32_t id, const QString &package);
    ~AlienClient();

    static const struct wl_interface *interface();
    static QByteArray interfaceName();

    AlienManager *manager() const;

protected:
    void alien_client_destroy(Resource *resource) override;
    void alien_client_get_alien_surface(Resource *resource, uint32_t id, ::wl_resource *surface) override;

private:
    QString m_package;
    AlienManager *m_manager;
};

#endif
