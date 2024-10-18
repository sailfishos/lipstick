/***************************************************************************
**
** Copyright (c) 2012 Jolla Ltd.
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

#ifndef UT_NOTIFICATIONPREVIEWPRESENTER_H
#define UT_NOTIFICATIONPREVIEWPRESENTER_H

#include <QObject>

namespace NemoDeviceLock {
class DeviceLock;
}

class ScreenLock;
class TouchScreen;

class Ut_NotificationPreviewPresenter : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void testAddNotificationWhenWindowNotOpen();
    void testAddNotificationWhenWindowAlreadyOpen();
    void testUpdateNotification();
    void testRemoveNotification();
    void testNotificationNotShownIfNoSummaryOrBody_data();
    void testNotificationNotShownIfNoSummaryOrBody();
    void testNotificationNotShownIfRestored();
    void testShowingOnlyCriticalNotifications();
    void testNotificationNotShownIfTouchScreenIsLockedAndDisplayIsOff_data();
    void testNotificationNotShownIfTouchScreenIsLockedAndDisplayIsOff();
    void testCriticalNotificationIsMarkedAfterShowing();
    void testNotificationPreviewsDisabled_data();
    void testNotificationPreviewsDisabled();

private:
    TouchScreen *touchScreen;
    ScreenLock *screenLock;
    NemoDeviceLock::DeviceLock *deviceLock;
};

#endif
