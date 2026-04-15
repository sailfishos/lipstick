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

#ifndef XDGSHELL_H
#define XDGSHELL_H

#include <QObject>
#include <QtCompositor/QWaylandGlobalInterface>

#include "qwayland-server-xdg-shell.h"

class XdgShellGlobal : public QObject, public QWaylandGlobalInterface
{
public:
    explicit XdgShellGlobal(QObject *parent = nullptr);

    const wl_interface *interface() const override;
    void bind(wl_client *client, uint32_t version, uint32_t id) override;
};

class XdgShell : public QObject, public QtWaylandServer::xdg_wm_base
{
public:
    XdgShell(wl_client *client, uint32_t version, uint32_t id, QObject *parent);
    ~XdgShell();

    void ping(uint32_t serial, QWaylandSurface *surface);

protected:
    void xdg_wm_base_destroy_resource(Resource *resource) override;
    void xdg_wm_base_destroy(Resource *resource) override;
    void xdg_wm_base_create_positioner(Resource *resource, uint32_t id) override;
    void xdg_wm_base_get_xdg_surface(Resource *resource, uint32_t id, ::wl_resource *surface) override;
    void xdg_wm_base_pong(Resource *resource, uint32_t serial) override;

private:
    QMap<uint32_t, QWaylandSurface *> m_pings;
};

#endif
