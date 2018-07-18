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

#ifndef WINDOWPIXMAPITEM_H
#define WINDOWPIXMAPITEM_H

#include "lipstickglobal.h"

#include <QWaylandQuickItem>

class QWaylandUnmapLock;

class LipstickCompositor;
class LipstickCompositorWindow;
class LIPSTICK_EXPORT WindowPixmapItem : public QWaylandQuickItem
{
    Q_OBJECT
    Q_PROPERTY(int windowId READ windowId WRITE setWindowId NOTIFY windowIdChanged)
    Q_PROPERTY(bool hasPixmap READ hasPixmap NOTIFY hasPixmapChanged)
    Q_PROPERTY(bool opaque READ opaque WRITE setOpaque NOTIFY opaqueChanged)
    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged)
    Q_PROPERTY(QSize windowSize READ windowSize NOTIFY windowSizeChanged)
    Q_PROPERTY(qreal xOffset READ xOffset WRITE setXOffset NOTIFY xOffsetChanged)
    Q_PROPERTY(qreal yOffset READ yOffset WRITE setYOffset NOTIFY yOffsetChanged)
    Q_PROPERTY(qreal xScale READ xScale WRITE setXScale NOTIFY xScaleChanged)
    Q_PROPERTY(qreal yScale READ yScale WRITE setYScale NOTIFY yScaleChanged)

public:
    WindowPixmapItem();
    ~WindowPixmapItem();

    int windowId() const;
    void setWindowId(int);

    bool hasPixmap() const;

    bool opaque() const;
    void setOpaque(bool);

    qreal radius() const;
    void setRadius(qreal);

    QSize windowSize() const;

    qreal xOffset() const;
    void setXOffset(qreal);

    qreal yOffset() const;
    void setYOffset(qreal);

    qreal xScale() const;
    void setXScale(qreal);

    qreal yScale() const;
    void setYScale(qreal);

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;

signals:
    void windowIdChanged();
    void hasPixmapChanged();
    void opaqueChanged();
    void radiusChanged();
    void windowSizeChanged();
    void xOffsetChanged();
    void yOffsetChanged();
    void xScaleChanged();
    void yScaleChanged();


private:
    inline void handleWindowSizeChanged();
    inline void handleHasContentChanged();
    inline void updateItem();
    inline void surfaceDestroyed();
    inline void cleanupOpenGL();

    int m_id = 0;
    qreal m_radius = 0;
    qreal m_xOffset = 0;
    qreal m_yOffset = 0;
    qreal m_xScale = 1;
    qreal m_yScale = 1;
    QSize m_windowSize;
    bool m_opaque = false;
    bool m_hasBuffer = false;
    bool m_hasPixmap = false;
    bool m_surfaceDestroyed = false;
    bool m_haveSnapshot = false;
    QSGTextureProvider *m_textureProvider = nullptr;

    static struct SnapshotProgram *s_snapshotProgram;
};

#endif // WINDOWPIXMAPITEM_H
