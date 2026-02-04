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

#ifndef LIPSTICKVIEWPORTER_H
#define LIPSTICKVIEWPORTER_H

#include <QObject>
#include <QRect>
#include <QtCompositor/QWaylandGlobalInterface>
#include <QtCompositor/QWaylandSurfaceInterface>

#include "qwayland-server-viewporter.h"

class QWaylandSurface;

class ViewporterGlobal : public QObject, public QWaylandGlobalInterface
{
public:
    explicit ViewporterGlobal(QObject *parent = nullptr);

    const wl_interface *interface() const Q_DECL_OVERRIDE;
    void bind(wl_client *client, uint32_t version, uint32_t id) Q_DECL_OVERRIDE;
};

class LipstickViewporter : public QObject, public QtWaylandServer::wp_viewporter
{
public:
    LipstickViewporter(wl_client *client, uint32_t version, uint32_t id, QObject *parent);
    ~LipstickViewporter();

protected:
    void wp_viewporter_destroy_resource(Resource *resource) Q_DECL_OVERRIDE;
    void wp_viewporter_destroy(Resource *resource) Q_DECL_OVERRIDE;
    void wp_viewporter_get_viewport(Resource *resource, uint32_t id, ::wl_resource *surface) Q_DECL_OVERRIDE;
};

class LipstickViewport
    : public QObject
    , public QWaylandSurfaceInterface
    , public QtWaylandServer::wp_viewport
{
public:
    LipstickViewport(QWaylandSurface *surface, uint32_t version, uint32_t id, QObject *parent);
    ~LipstickViewport();

protected:
    bool runOperation(QWaylandSurfaceOp *op) Q_DECL_OVERRIDE;

    void wp_viewport_destroy_resource(Resource *resource) Q_DECL_OVERRIDE;
    void wp_viewport_destroy(Resource *resource) Q_DECL_OVERRIDE;
    void wp_viewport_set_source(Resource *resource, wl_fixed_t x, wl_fixed_t y, wl_fixed_t width, wl_fixed_t height) Q_DECL_OVERRIDE;
    void wp_viewport_set_destination(Resource *resource, int32_t width, int32_t height) Q_DECL_OVERRIDE;

private:
    QRectF m_sourceRect;
    QSize m_destSize;
};

#endif
