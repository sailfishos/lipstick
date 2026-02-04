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

#ifndef XDGSURFACE_H
#define XDGSURFACE_H

#include <QObject>
#include <QRect>
#include <QtCompositor/QWaylandSurfaceInterface>

#include "qwayland-server-xdg-shell.h"

class QWaylandSurface;

class XdgShell;
class XdgToplevel;

class XdgSurface
    : public QObject
    , public QWaylandSurfaceInterface
    , public QtWaylandServer::xdg_surface
{
    Q_OBJECT

public:
    XdgSurface(XdgShell *mgr, QWaylandSurface *surface, uint32_t version, uint32_t id);
    ~XdgSurface();

    XdgShell *manager() const;
    uint32_t serial() const;
    XdgSurface *xdgParent() const;
    const QRect& geometry() const;

    XdgToplevel *findToplevel(QPointF *offset) const;

    void setPreferredScale(qreal scale);

    void sendConfigure();
    void roleDestroyed();

signals:
    void geometryChanged();

protected:
    bool runOperation(QWaylandSurfaceOp *op) Q_DECL_OVERRIDE;

    void xdg_surface_destroy_resource(Resource *resource) Q_DECL_OVERRIDE;
    void xdg_surface_destroy(Resource *resource) Q_DECL_OVERRIDE;
    void xdg_surface_get_toplevel(Resource *resource, uint32_t id) Q_DECL_OVERRIDE;
    void xdg_surface_get_popup(Resource *resource, uint32_t id, ::wl_resource *parent, ::wl_resource *positioner) Q_DECL_OVERRIDE;
    void xdg_surface_set_window_geometry(Resource *resource, int32_t x, int32_t y, int32_t width, int32_t height) Q_DECL_OVERRIDE;
    void xdg_surface_ack_configure(Resource *resource, uint32_t serial) Q_DECL_OVERRIDE;

private slots:
    void handleCommit();

private:
    XdgShell *m_manager;
    XdgSurface *m_xdgParent;
    XdgToplevel *m_toplevel;
    QRect m_pendingGeometry;
    QRect m_geometry;
    qreal m_preferredScale;
    uint32_t m_serial;
};

#endif
