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

#include "lipstickcompositor.h"
#include "lipsticksurfaceinterface.h"
#include "xdgshell.h"
#include "xdgsurface.h"
#include "xdgtoplevel.h"

XdgToplevel::XdgToplevel(XdgSurface *xdgSurface, uint32_t version, uint32_t id)
    : QObject(xdgSurface)
    , QWaylandSurfaceInterface(xdgSurface->surface())
    , QtWaylandServer::xdg_toplevel(surface()->client()->client(), id, version)
    , m_xdgSurface(xdgSurface)
    , m_hidden(false)
    , m_coverized(false)
    , m_scale(1.0)
{
    setSurfaceType(QWaylandSurface::Toplevel);

    LipstickCompositor *compositor = LipstickCompositor::instance();
    m_size = QSize(compositor->width(), compositor->height());
    sendConfigure();

    connect(xdgSurface, &XdgSurface::geometryChanged, this, &XdgToplevel::geometryChanged);
    connect(surface(), &QWaylandSurface::configure, this, &XdgToplevel::configure);
}

XdgToplevel::~XdgToplevel()
{
    wl_resource_set_implementation(resource()->handle, nullptr, nullptr, nullptr);
    surface()->setMapped(false);
    m_xdgSurface->roleDestroyed();
}

qreal XdgToplevel::scale() const
{
    return m_scale;
}

bool XdgToplevel::runOperation(QWaylandSurfaceOp *op)
{
    switch (op->type()) {
        case QWaylandSurfaceOp::Close:
            send_close();
            return true;
        case QWaylandSurfaceOp::SetVisibility: {
            QWindow::Visibility v = static_cast<QWaylandSurfaceSetVisibilityOp *>(op)->visibility();
            m_hidden = v == QWindow::Hidden;
            m_coverized = v == QWindow::Minimized;
            sendConfigure();
            return true;
        }
        case QWaylandSurfaceOp::Ping:
            m_xdgSurface->manager()->ping(static_cast<QWaylandSurfacePingOp *>(op)->serial(), surface());
            return true;
        case QWaylandSurfaceOp::Resize:
            m_size = static_cast<QWaylandSurfaceResizeOp *>(op)->size();
            sendConfigure();
            return true;
        case LipstickScaleOp::Type:
            m_scale = static_cast<LipstickScaleOp *>(op)->scale();
            m_xdgSurface->setPreferredScale(m_scale);
            sendConfigure();
            return true;
        default:
            break;
    }
    return false;
}

void XdgToplevel::xdg_toplevel_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource);
    delete this;
}

void XdgToplevel::xdg_toplevel_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void XdgToplevel::xdg_toplevel_set_title(Resource *resource, const QString &title)
{
    Q_UNUSED(resource);
    setSurfaceTitle(title);
}

void XdgToplevel::xdg_toplevel_set_app_id(Resource *resource, const QString &appId)
{
    Q_UNUSED(resource);
    setSurfaceClassName(appId);
}

void XdgToplevel::xdg_toplevel_set_minimized(Resource *resource)
{
    Q_UNUSED(resource);
    emit surface()->lowerRequested();
}

void XdgToplevel::configure(bool hasBuffer)
{
    if (hasBuffer && m_xdgSurface->geometry().isNull()) {
        m_size = surface()->size() * m_scale;
    }
    surface()->setMapped(hasBuffer);
}

void XdgToplevel::sendConfigure()
{
    QVector<uint32_t> states;
    states << XDG_TOPLEVEL_STATE_MAXIMIZED;
    if (!m_coverized && !m_hidden) {
        states << XDG_TOPLEVEL_STATE_ACTIVATED;
    }
    QByteArray data = QByteArray::fromRawData((char *)states.data(), states.size() * sizeof(uint32_t));

    send_configure(m_size.width() / m_scale, m_size.height() / m_scale, data);
    m_xdgSurface->sendConfigure();
}

void XdgToplevel::geometryChanged()
{
    if (m_xdgSurface->geometry().isValid()) {
        m_size = m_xdgSurface->geometry().size() * m_scale;
    }
}
