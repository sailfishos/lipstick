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

#ifndef NOTIFICATIONPREVIEWPRESENTER_H
#define NOTIFICATIONPREVIEWPRESENTER_H

#include "lipstickglobal.h"
#include <QObject>
#include <QTimer>

namespace NemoDeviceLock {
class DeviceLock;
}

class HomeWindow;
class LipstickNotification;
class NotificationFeedbackPlayer;
class ScreenLock;

/*!
 * \class NotificationPreviewPresenter
 *
 * \brief Presents notification previews one at a time.
 *
 * Creates a transparent notification window which can be used to show
 * notification previews.
 */
class LIPSTICK_EXPORT NotificationPreviewPresenter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(LipstickNotification *notification READ notification NOTIFY notificationChanged)

public:
    explicit NotificationPreviewPresenter(ScreenLock *screenLock, NemoDeviceLock::DeviceLock *deviceLock,
                                          QObject *parent = 0);
    virtual ~NotificationPreviewPresenter();

    /*!
     * Returns the notification to be currently shown or 0 if no notification
     * should be shown.
     *
     * \return the notification to be currently shown or 0 if no notification should be shown
     */
    LipstickNotification *notification() const;

signals:
    //! Sent when the notification to be shown has changed.
    void notificationChanged();

public slots:
    /*!
     * Shows the next notification to be shown, if any. If the notification
     * window is not yet visible, shows the window. If there is no
     * notification to be shown but the window is visible, hides the window.
     */
    void showNextNotification();

private slots:
    /*!
     * Updates the notification with the given ID.
     *
     * \param id the ID of the notification to be updated
     */
    void updateNotification(uint id);

    /*!
     * Removes the notification with the given ID.
     *
     * \param id the ID of the notification to be removed
     */
    void removeNotification(uint id);

    //! Creates the notification window if it has not been created yet.
    void createWindowIfNecessary();

private:
    bool notificationShouldBeShown(LipstickNotification *notification);

    //! Sets the given notification as the current notification
    void setCurrentNotification(LipstickNotification *notification, bool background = false);

    //! The notification window
    HomeWindow *m_window;

    //! Notifications to be shown
    QList<LipstickNotification *> m_notificationQueue;

    //! Notification currently being shown
    LipstickNotification *m_currentNotification;

    //! Player for notification feedbacks
    NotificationFeedbackPlayer *m_notificationFeedbackPlayer;

    //! For getting information about the touch screen lock state
    ScreenLock *m_screenLock;

    NemoDeviceLock::DeviceLock *m_deviceLock;
    QTimer m_backgroundNotificationTimer;
    bool m_currentInBackground;

#ifdef UNIT_TEST
    friend class Ut_NotificationPreviewPresenter;
#endif
};

#endif // NOTIFICATIONPREVIEWPRESENTER_H
