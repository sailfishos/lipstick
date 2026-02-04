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

#ifndef LIPSTICKFRACTIONALSCALE_H
#define LIPSTICKFRACTIONALSCALE_H

#include <QObject>
#include <QtCompositor/QWaylandGlobalInterface>
#include <QtCompositor/QWaylandSurfaceInterface>

#include "qwayland-server-fractional-scale-v1.h"

class QWaylandSurface;

class FractionalScaleGlobal : public QObject, public QWaylandGlobalInterface
{
public:
    explicit FractionalScaleGlobal(QObject *parent = nullptr);

    const wl_interface *interface() const Q_DECL_OVERRIDE;
    void bind(wl_client *client, uint32_t version, uint32_t id) Q_DECL_OVERRIDE;
};

class FractionalScaleManager : public QObject, public QtWaylandServer::wp_fractional_scale_manager_v1
{
public:
    FractionalScaleManager(wl_client *client, uint32_t version, uint32_t id, QObject *parent);
    ~FractionalScaleManager();

protected:
    void wp_fractional_scale_manager_v1_destroy_resource(Resource *resource) Q_DECL_OVERRIDE;
    void wp_fractional_scale_manager_v1_destroy(Resource *resource) Q_DECL_OVERRIDE;
    void wp_fractional_scale_manager_v1_get_fractional_scale(Resource *resource, uint32_t id, ::wl_resource *surface) Q_DECL_OVERRIDE;
};

class FractionalScale
    : public QObject
    , public QWaylandSurfaceInterface
    , public QtWaylandServer::wp_fractional_scale_v1
{
public:
    FractionalScale(QWaylandSurface *surface, uint32_t version, uint32_t id, QObject *parent);
    ~FractionalScale();

protected:
    bool runOperation(QWaylandSurfaceOp *op) Q_DECL_OVERRIDE;

    void wp_fractional_scale_v1_destroy_resource(Resource *resource) Q_DECL_OVERRIDE;
    void wp_fractional_scale_v1_destroy(Resource *resource) Q_DECL_OVERRIDE;
};

#endif
