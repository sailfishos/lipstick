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

#ifndef XDGTOPLEVEL_H
#define XDGTOPLEVEL_H

#include <QtCompositor/QWaylandSurfaceInterface>

#include "qwayland-server-xdg-shell.h"

class XdgSurface;

class XdgToplevel
    : public QObject
    , public QWaylandSurfaceInterface
    , public QtWaylandServer::xdg_toplevel
{
public:
    XdgToplevel(XdgSurface *xdgSurface, uint32_t version, uint32_t id);
    ~XdgToplevel();

    qreal scale() const;

protected:
    bool runOperation(QWaylandSurfaceOp *op) Q_DECL_OVERRIDE;

    void xdg_toplevel_destroy_resource(Resource *resource) Q_DECL_OVERRIDE;
    void xdg_toplevel_destroy(Resource *resource) Q_DECL_OVERRIDE;
    void xdg_toplevel_set_title(Resource *resource, const QString &title) Q_DECL_OVERRIDE;
    void xdg_toplevel_set_app_id(Resource *resource, const QString &appId) Q_DECL_OVERRIDE;
    void xdg_toplevel_set_minimized(Resource *resource) Q_DECL_OVERRIDE;

private:
    void geometryChanged();
    void configure(bool hasBuffer);
    void sendConfigure();

    XdgSurface *m_xdgSurface;
    bool m_hidden;
    bool m_coverized;
    QSize m_size;
    qreal m_scale;
};

#endif
