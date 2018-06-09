/***************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Robin Burchell <robin.burchell@jollamobile.com>
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
#include "hwcrenderstage.h"
#include "lipstickcompositor.h"
#include "screenshotservice.h"

#include <QGuiApplication>
#include <QImage>
#include <QScreen>
#include <QTransform>
#include <private/qquickwindow_p.h>

bool ScreenshotService::saveScreenshot(const QString &path)
{
    if (path.isEmpty()) {
        qWarning() << "Screenshot path is empty.";
        return false;
    }

    LipstickCompositor *compositor = LipstickCompositor::instance();
    if (!compositor) {
        return false;
    }

    QQuickWindowPrivate *wd = QQuickWindowPrivate::get(compositor);
    HwcRenderStage *renderStage = (HwcRenderStage *) wd->customRenderStage;
    if (renderStage)
        renderStage->setBypassHwc(true);

    QImage grab(compositor->grabWindow());

    int rotation(QGuiApplication::primaryScreen()->angleBetween(Qt::PrimaryOrientation, compositor->topmostWindowOrientation()));
    if (rotation != 0) {
        QTransform xform;
        xform.rotate(rotation);
        grab = grab.transformed(xform, Qt::SmoothTransformation);
    }

    const bool saved = grab.save(path);

    if (renderStage) {
        renderStage->setBypassHwc(false);
    }

    return saved;
}
