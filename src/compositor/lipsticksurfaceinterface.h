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

class LipstickSetPopupAreaOp : public QWaylandSurfaceOp
{
public:
    enum { Type = QWaylandSurfaceOp::UserType + 2 };
    LipstickSetPopupAreaOp(const QRect &bounds);

    const QRect &bounds() const { return m_bounds; }

private:
    QRect m_bounds;
};

class LipstickGetShellStateOp : public QWaylandSurfaceOp
{
public:
    enum { Type = QWaylandSurfaceOp::UserType + 3 };
    LipstickGetShellStateOp();

    bool m_resizeAcked : 1;
};

#endif
