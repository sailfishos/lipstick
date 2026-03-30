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

#ifndef XDGPOSITIONER_H
#define XDGPOSITIONER_H

#include <QObject>
#include <QRect>

#include "qwayland-server-xdg-shell.h"

class XdgShell;
class XdgSurface;

class XdgPositioner : public QObject, public QtWaylandServer::xdg_positioner
{
public:
    XdgPositioner(XdgShell *mgr, wl_client *client, uint32_t version, uint32_t id);
    ~XdgPositioner();

    static XdgPositioner *fromResource(::wl_resource *resource);

    QRect toRect(const XdgSurface *surface) const;

protected:
    void xdg_positioner_destroy_resource(Resource *resource) override;
    void xdg_positioner_destroy(Resource *resource) override;
    void xdg_positioner_set_size(Resource *resource, int32_t width, int32_t height) override;
    void xdg_positioner_set_anchor_rect(Resource *resource, int32_t x, int32_t y, int32_t width, int32_t height) override;
    void xdg_positioner_set_anchor(Resource *resource, uint32_t anchor) override;
    void xdg_positioner_set_gravity(Resource *resource, uint32_t gravity) override;
    void xdg_positioner_set_constraint_adjustment(Resource *resource, uint32_t adjustment) override;
    void xdg_positioner_set_offset(Resource *resource, int32_t x, int32_t y) override;

private:
    QSize m_size;
    QRect m_anchorRect;
    Qt::Edges m_anchor;
    Qt::Edges m_gravity;
    uint32_t m_adjustment;
    QPoint m_offset;
};

#endif
