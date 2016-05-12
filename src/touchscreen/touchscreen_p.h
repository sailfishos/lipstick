/***************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
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

#ifndef TOUCHEVENTFILTER_P_H
#define TOUCHEVENTFILTER_P_H

#include <qmdisplaystate.h>

#include "touchscreen.h"
#include "homeapplication.h"


class TouchScreenPrivate {
public:
    explicit TouchScreenPrivate(TouchScreen *q);

    void handleDisplayStateChange(TouchScreen::DisplayState state);
    bool touchBlocked() const;

    bool eatEvents;
    TouchScreen::DisplayState currentDisplayState;
    bool waitForTouchBegin;
    int touchUnblockingDelayTimer;
    MeeGo::QmDisplayState *displayState;

    TouchScreen *q_ptr;

    Q_DECLARE_PUBLIC(TouchScreen)
};

#endif // TOUCHEVENTFILTER_P_H
