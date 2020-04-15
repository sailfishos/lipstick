/***************************************************************************
**
** Copyright (c) 2016 Jolla Ltd.
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

#ifndef UT_TOUCHSCREEN_H
#define UT_TOUCHSCREEN_H

#include <QObject>
#include <qmdisplaystate.h>

class TouchScreen;

class Ut_TouchScreen : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void initTestCase();
    void cleanupTestCase();

    void testEnabled();
    void testTouchBlocking();

private:
    void updateDisplayState(MeeGo::QmDisplayState::DisplayState oldState, MeeGo::QmDisplayState::DisplayState newState);
    void fakeDisplayOnAndReady();

    TouchScreen *touchScreen;
};

#endif
