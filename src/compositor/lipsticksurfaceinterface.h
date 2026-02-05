/***************************************************************************
**
** Copyright (c) 2014 Jolla Ltd.
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

#ifndef LIPSTICKSURFACEINTERFACE
#define LIPSTICKSURFACEINTERFACE

#include <QtCompositor/QWaylandSurfaceInterface>

#include "lipstickglobal.h"

class LIPSTICK_EXPORT LipstickOomScoreOp : public QWaylandSurfaceOp
{
public:
    enum { Type = QWaylandSurfaceOp::UserType + 1 };
    LipstickOomScoreOp(int score);

    int score() const { return m_score; }

private:
    int m_score;
};

class LipstickScaleOp : public QWaylandSurfaceOp
{
public:
    enum { Type = QWaylandSurfaceOp::UserType + 2 };
    LipstickScaleOp(qreal scale);

    qreal scale() const { return m_scale; }

private:
    qreal m_scale;
};

class LipstickGetViewportOp : public QWaylandSurfaceOp
{
public:
    enum { Type = QWaylandSurfaceOp::UserType + 3 };
    LipstickGetViewportOp();

    const QRectF &sourceRect() const { return m_sourceRect; }
    const QSize &destSize() const { return m_destSize; }

private:
    QRectF m_sourceRect;
    QSize m_destSize;

    friend class LipstickViewport;
};

class LipstickBufferScaleOp : public QWaylandSurfaceOp
{
public:
    enum { Type = QWaylandSurfaceOp::UserType + 4 };
    LipstickBufferScaleOp(qreal scale);

    qreal scale() const { return m_scale; }

private:
    qreal m_scale;
};

#endif
