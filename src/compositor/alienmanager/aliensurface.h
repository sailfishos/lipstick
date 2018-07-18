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

#ifndef ALIENSURFACE_H
#define ALIENSURFACE_H

#include <QSize>
#include <QWaylandCompositorExtension>

#include "qwayland-server-alien-manager.h"

QT_BEGIN_NAMESPACE
class QWaylandSurface;
QT_END_NAMESPACE

class AlienClient;

class AlienSurface : public QWaylandCompositorExtensionTemplate<AlienSurface>, public QtWaylandServer::alien_surface
{
    Q_OBJECT
public:
    AlienSurface(
            struct wl_client *client,
            uint32_t id,
            int version,
            QWaylandSurface *surface,
            AlienClient *alien,
            const QString &package);
    ~AlienSurface();

    QString title() const;
    QString className() const;

    QWaylandSurface *surface() const;

    void show(bool isCover);
    void hide();
    void resize(const QSize &size);
    void close();
    void ping();
    void sendOomScore(int score);

signals:
    void titleChanged(const QString &title);
    void pong();

protected:
    void alien_surface_destroy(Resource *resource) override;
    void alien_surface_set_title(Resource *resource, const QString &title) override;
    void alien_surface_ack_configure(Resource *resource, uint32_t serial) override;
    void alien_surface_request_state(Resource *resource, wl_array *states, uint32_t serial) override;
    void alien_surface_set_minimized(Resource *resource) override;

private:
    inline void sendConfigure();

    AlienClient * const m_client;
    QWaylandSurface * const m_surface;
    QSize m_lastSize;
    QString m_title;
    const QString m_className;
    uint32_t m_serial = 0;
    uint32_t m_lastSerial = 0;
    bool m_hidden = false;
    bool m_coverized = false;
};

#endif
