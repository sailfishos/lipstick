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

#include <QtCompositor/QWaylandClient>
#include <QtCompositor/QWaylandSurface>

#include "lipsticksurfaceinterface.h"
#include "lipstickfractionalscale.h"

FractionalScaleGlobal::FractionalScaleGlobal(QObject *parent)
    : QObject(parent)
{
}

const wl_interface *FractionalScaleGlobal::interface() const
{
    return &wp_fractional_scale_manager_v1_interface;
}

void FractionalScaleGlobal::bind(wl_client *client, uint32_t version, uint32_t id)
{
    new FractionalScaleManager(client, version, id, this);
}

FractionalScaleManager::FractionalScaleManager(wl_client *client, uint32_t version, uint32_t id, QObject *parent)
    : QObject(parent)
    , QtWaylandServer::wp_fractional_scale_manager_v1(client, id, version)
{
}

FractionalScaleManager::~FractionalScaleManager()
{
    wl_resource_set_implementation(resource()->handle, nullptr, nullptr, nullptr);
}

void FractionalScaleManager::wp_fractional_scale_manager_v1_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource);
    delete this;
}

void FractionalScaleManager::wp_fractional_scale_manager_v1_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void FractionalScaleManager::wp_fractional_scale_manager_v1_get_fractional_scale(Resource *resource, uint32_t id, ::wl_resource *surface)
{
    Q_UNUSED(resource);
    QWaylandSurface *surf = QWaylandSurface::fromResource(surface);
    new FractionalScale(surf, wl_resource_get_version(resource->handle), id, this);
}

FractionalScale::FractionalScale(QWaylandSurface *surface, uint32_t version, uint32_t id, QObject *parent)
    : QObject(parent)
    , QWaylandSurfaceInterface(surface)
    , QtWaylandServer::wp_fractional_scale_v1(surface->client()->client(), id, version)
{
}

FractionalScale::~FractionalScale()
{
    wl_resource_set_implementation(resource()->handle, nullptr, nullptr, nullptr);
}

bool FractionalScale::runOperation(QWaylandSurfaceOp *op)
{
    switch (op->type()) {
        case LipstickBufferScaleOp::Type:
            send_preferred_scale(static_cast<LipstickBufferScaleOp *>(op)->scale() * 120);
            return true;
        default:
            break;
    }
    return false;
}

void FractionalScale::wp_fractional_scale_v1_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource);
    delete this;
}

void FractionalScale::wp_fractional_scale_v1_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}
