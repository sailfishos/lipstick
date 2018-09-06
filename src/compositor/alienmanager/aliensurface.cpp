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

#include <QWaylandSurface>

#include "aliensurface.h"
#include "alienmanager.h"

#include <lipstickcompositor.h>
#include <lipstickcompositorwindow.h>

AlienSurface::AlienSurface(
        struct wl_client *client,
        uint32_t id,
        int version,
        QWaylandSurface *surface,
        AlienClient *alien,
        const QString &package)
    : QWaylandCompositorExtensionTemplate()
    , QtWaylandServer::alien_surface(client, id, version)
    , m_client(alien)
    , m_surface(surface)
    , m_className(package)
{
    m_lastSize = LipstickCompositor::instance()->quickWindow()->size();
    connect(surface, &QWaylandSurface::sizeChanged, this, [this]() {
        m_lastSize = m_surface->size();
    });

    sendConfigure();
}

AlienSurface::~AlienSurface()
{
}

QString AlienSurface::title() const
{
    return m_title;
}

QString AlienSurface::className() const
{
    return m_className;
}

QWaylandSurface *AlienSurface::surface() const
{
    return m_surface;
}

void AlienSurface::show(bool isCover)
{
    m_hidden = false;
    m_coverized = isCover;
    sendConfigure();
}

void AlienSurface::hide()
{
    m_hidden = true;
    m_coverized = false;
    sendConfigure();
}

void AlienSurface::resize(const QSize &size)
{
    m_lastSize = size;
    sendConfigure();
}

void AlienSurface::close()
{
    send_close();
}

void AlienSurface::ping()
{
    m_client->manager()->ping(m_surface->compositor()->nextSerial(), this);
}

void AlienSurface::sendOomScore(int score)
{
    m_client->send_oom_score(score);
}

void AlienSurface::alien_surface_destroy(Resource *)
{
    delete this;
}

void AlienSurface::alien_surface_set_title(Resource *resource, const QString &title)
{
    Q_UNUSED(resource)
    if (m_title != title) {
        m_title = title;
        emit titleChanged(title);
    }
}

void AlienSurface::alien_surface_ack_configure(Resource *resource, uint32_t serial)
{
    Q_UNUSED(resource);
    if (serial == m_serial) {
        m_serial = 0;
    }
}

void AlienSurface::alien_surface_request_state(Resource *resource, wl_array *array, uint32_t serial)
{
    Q_UNUSED(resource);
    bool wasHiddenOrCover = m_hidden | m_coverized;
    m_hidden = false;

    for (uint32_t *ptr = (uint32_t *)array->data; (char *)ptr < (char *)array->data + array->size; ++ptr) {
        if (*ptr == ALIEN_SURFACE_STATE_HIDDEN) {
            m_hidden = true;
        }
    }
    // do some focus stealing prevenction
    if (wasHiddenOrCover && !m_hidden && serial == m_lastSerial) {
        if (LipstickCompositorWindow *window = LipstickCompositor::surfaceWindow(m_surface)) {
            LipstickCompositor::instance()->windowRaised(window);
        }
    } else {
        sendConfigure();
    }
}

void AlienSurface::alien_surface_set_minimized(Resource *resource)
{
    Q_UNUSED(resource)
    if (LipstickCompositorWindow *window = LipstickCompositor::surfaceWindow(m_surface)) {
        LipstickCompositor::instance()->windowLowered(window);
    }
}

void AlienSurface::sendConfigure()
{
    QVector<uint32_t> states;
    if (m_hidden) {
        states << ALIEN_SURFACE_STATE_HIDDEN;
    }
    if (m_coverized) {
        states << ALIEN_SURFACE_STATE_COVER;
    }
    QByteArray data = QByteArray::fromRawData((char *)states.data(), states.size() * sizeof(uint32_t));
    m_serial = m_surface->compositor()->nextSerial();
    m_lastSerial = m_serial;

    send_configure(m_lastSize.width(), m_lastSize.height(), data, m_serial);
}

