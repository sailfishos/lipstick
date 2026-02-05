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

#ifndef XDGPOPUP_H
#define XDGPOPUP_H

#include <QtCompositor/QWaylandSurfaceInterface>

#include "qwayland-server-xdg-shell.h"

class XdgSurface;

class XdgPopup
    : public QObject
    , public QWaylandSurfaceInterface
    , public QtWaylandServer::xdg_popup
{
public:
    XdgPopup(XdgSurface *xdgSurface, const QRect &rect, uint32_t version, uint32_t id);
    ~XdgPopup();

protected:
    bool runOperation(QWaylandSurfaceOp *op) Q_DECL_OVERRIDE;

    void xdg_popup_destroy_resource(Resource *resource) Q_DECL_OVERRIDE;
    void xdg_popup_destroy(Resource *resource) Q_DECL_OVERRIDE;
    void xdg_popup_reposition(Resource *resource, ::wl_resource *positioner, uint32_t token) Q_DECL_OVERRIDE;

private:
    void updateOffset();
    void configure(bool hasBuffer);
    void sendConfigure();

    XdgSurface *m_xdgSurface;
    QRect m_rect;
};

#endif
