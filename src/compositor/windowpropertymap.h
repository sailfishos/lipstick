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

#ifndef WINDOWPROPERTY_H
#define WINDOWPROPERTY_H

#include <QObject>
#include <QPointer>
#include <QVariant>
#include "lipstickcompositorwindow.h"
#include <QQmlPropertyMap>

class QWaylandSurface;


class LIPSTICK_EXPORT WindowPropertyMap : public QQmlPropertyMap
{
    Q_OBJECT
public:
    WindowPropertyMap(QtWayland::ExtendedSurface *surface, QWaylandSurface *waylandSurface, QObject *parent = nullptr);
    ~WindowPropertyMap();

    static QVariant fixupWindowProperty(
            LipstickCompositor *compositor, QWaylandSurface *surface, const QVariant &value);

protected:
    QVariant updateValue(const QString &key, const QVariant &input) override;

private:
    inline void insertWindowProperty(const QString &key, const QVariant &value);

    QPointer<QtWayland::ExtendedSurface> m_surface;
    QPointer<QWaylandSurface> m_waylandSurface;
};

#endif // WINDOWPROPERTY_H
