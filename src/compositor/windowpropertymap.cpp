/***************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Aaron Kennedy <aaron.kennedy@jollamobile.com>
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

#include "windowpropertymap.h"

#include "lipstickcompositorwindow.h"
#include "lipstickcompositor.h"

#include <private/qwlextendedsurface_p.h>

WindowPropertyMap::WindowPropertyMap(
        QtWayland::ExtendedSurface *surface, QWaylandSurface *waylandSurface, QObject *parent)
    : QQmlPropertyMap(parent)
    , m_surface(surface)
    , m_waylandSurface(waylandSurface)
{
    if (m_surface) {
        // this must use a queued connection in order to avoid QTBUG-32859
        connect(m_surface.data(), &QtWayland::ExtendedSurface::windowPropertyChanged,
                this, &WindowPropertyMap::insertWindowProperty, Qt::QueuedConnection);

        const auto windowProperties = m_surface->windowProperties();
        for (auto it = windowProperties.begin(); it != windowProperties.end(); ++it) {
            insertWindowProperty(it.key(), it.value());
        }
    }
}

WindowPropertyMap::~WindowPropertyMap()
{
}

QVariant WindowPropertyMap::fixupWindowProperty(
        LipstickCompositor *compositor, QWaylandSurface *surface, const QVariant &value)
{
    if (value.type() == QVariant::String) {
        const QString string = value.toString();
        if (string.startsWith(QLatin1String("__winref:"))) {
            const uint id = string.midRef(9).toUInt();
            if (id != 0) {
                return compositor->windowIdForLink(surface, id);
            } else {
                return 0;
            }
        }
    }
    return value;
}

QVariant WindowPropertyMap::updateValue(const QString &key, const QVariant &input)
{
    if (m_surface) {
        m_surface->setWindowProperty(key, input);
    }
    return QQmlPropertyMap::updateValue(key, input);
}

void WindowPropertyMap::insertWindowProperty(const QString &key, const QVariant &value)
{
    insert(key, fixupWindowProperty(LipstickCompositor::instance(), m_waylandSurface.data(), value));
}
