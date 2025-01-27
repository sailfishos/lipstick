/***************************************************************************
**
** Copyright (c) 2012-2019 Jolla Ltd.
** Copyright (c) 2020 Open Mobile Platform LLC.
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

#ifndef NOTIFICATIONFEEDBACKPLAYER_H
#define NOTIFICATIONFEEDBACKPLAYER_H

#include "lipstickglobal.h"
#include <QObject>
#include <QHash>
#include <MDConfItem>
#include <profilecontrol.h>

class LipstickNotification;
namespace Ngf {
    class Client;
}

/*!
 * \class NotificationFeedbackPlayer
 *
 * \brief Plays non-graphical feedback for notifications.
 */
class LIPSTICK_EXPORT NotificationFeedbackPlayer : public QObject
{
    Q_OBJECT

public:
    explicit NotificationFeedbackPlayer(QObject *parent = 0);

    bool doNotDisturbMode() const;

public slots:
    /*!
     * Removes the notification with the given ID.
     *
     * \param id the ID of the notification to be removed
     */
    void removeNotification(uint id);

private slots:
    void init();

    /*!
     * Adds the notification with the given ID.
     *
     * \param id the ID of the notification to be added
     */
    bool addNotification(uint id);

private:
    //! Check whether feedbacks should be enabled for the given notification
    bool isEnabled(LipstickNotification *notification);

    //! Non-graphical feedback player
    Ngf::Client *m_ngfClient;

    //! A mapping between notification IDs and NGF play IDs.
    QMultiHash<LipstickNotification *, uint> m_idToEventId;

    MDConfItem m_doNotDisturbSetting;
    ProfileControl m_profileControl;

    friend class NotificationPreviewPresenter;
#ifdef UNIT_TEST
    friend class Ut_NotificationFeedbackPlayer;
#endif
};

#endif // NOTIFICATIONFEEDBACKPLAYER_H
