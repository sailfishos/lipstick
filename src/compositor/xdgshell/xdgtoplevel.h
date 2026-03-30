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
    Q_OBJECT

public:
    XdgToplevel(XdgSurface *xdgSurface, uint32_t version, uint32_t id);
    ~XdgToplevel();

    const QRect &popupArea() const { return m_popupArea; }

signals:
    void popupAreaChanged(const QRect &bounds);

protected:
    bool runOperation(QWaylandSurfaceOp *op) override;

    void xdg_toplevel_destroy_resource(Resource *resource) override;
    void xdg_toplevel_destroy(Resource *resource) override;
    void xdg_toplevel_set_parent(Resource *resource, ::wl_resource *parent) override;
    void xdg_toplevel_set_title(Resource *resource, const QString &title) override;
    void xdg_toplevel_set_app_id(Resource *resource, const QString &appId) override;
    void xdg_toplevel_set_minimized(Resource *resource) override;

private slots:
    void configure(bool hasBuffer);
    void geometryChanged();
    void parentPopupAreaChanged(const QRect &bounds);
    void parentUnmapped();

private:
    void setPopupArea(const QRect &bounds);
    void setParentToplevel(XdgToplevel *parent);
    void sendConfigure();

    XdgSurface *m_xdgSurface;
    XdgToplevel *m_parentToplevel;
    bool m_hidden;
    bool m_coverized;
    QSize m_size;
    QRect m_popupArea;
};

#endif
