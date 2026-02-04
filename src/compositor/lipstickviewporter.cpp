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
#include "lipstickviewporter.h"

ViewporterGlobal::ViewporterGlobal(QObject *parent)
    : QObject(parent)
{
}

const wl_interface *ViewporterGlobal::interface() const
{
    return &wp_viewporter_interface;
}

void ViewporterGlobal::bind(wl_client *client, uint32_t version, uint32_t id)
{
    new LipstickViewporter(client, version, id, this);
}

LipstickViewporter::LipstickViewporter(wl_client *client, uint32_t version, uint32_t id, QObject *parent)
    : QObject(parent)
    , QtWaylandServer::wp_viewporter(client, id, version)
{
}

LipstickViewporter::~LipstickViewporter()
{
    wl_resource_set_implementation(resource()->handle, nullptr, nullptr, nullptr);
}

void LipstickViewporter::wp_viewporter_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource);
    delete this;
}

void LipstickViewporter::wp_viewporter_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void LipstickViewporter::wp_viewporter_get_viewport(Resource *resource, uint32_t id, ::wl_resource *surface)
{
    Q_UNUSED(resource);
    QWaylandSurface *surf = QWaylandSurface::fromResource(surface);
    new LipstickViewport(surf, wl_resource_get_version(resource->handle), id, this);
}

LipstickViewport::LipstickViewport(QWaylandSurface *surface, uint32_t version, uint32_t id, QObject *parent)
    : QObject(parent)
    , QWaylandSurfaceInterface(surface)
    , QtWaylandServer::wp_viewport(surface->client()->client(), id, version)
{
}

LipstickViewport::~LipstickViewport()
{
    wl_resource_set_implementation(resource()->handle, nullptr, nullptr, nullptr);
}

bool LipstickViewport::runOperation(QWaylandSurfaceOp *op)
{
    switch (op->type()) {
        case LipstickGetViewportOp::Type: {
            LipstickGetViewportOp *vp = static_cast<LipstickGetViewportOp *>(op);
            vp->m_sourceRect = m_sourceRect;
            vp->m_destSize = m_destSize;
            return true;
        }
        default:
            break;
    }
    return false;
}

void LipstickViewport::wp_viewport_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource);
    delete this;
}

void LipstickViewport::wp_viewport_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void LipstickViewport::wp_viewport_set_source(Resource *resource, wl_fixed_t x, wl_fixed_t y, wl_fixed_t width, wl_fixed_t height)
{
    Q_UNUSED(resource);
    m_sourceRect.setRect(wl_fixed_to_double(x), wl_fixed_to_double(y),
                         wl_fixed_to_double(width), wl_fixed_to_double(height));
}

void LipstickViewport::wp_viewport_set_destination(Resource *resource, int32_t width, int32_t height)
{
    Q_UNUSED(resource);
    m_destSize = QSize(width, height);
}
