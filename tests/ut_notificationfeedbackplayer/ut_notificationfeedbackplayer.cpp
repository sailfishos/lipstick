/***************************************************************************
**
** Copyright (c) 2012 - 2021 Jolla Ltd.
** Copyright (c) 2021 Open Mobile Platform LLC.
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

#include <QtTest/QtTest>
#include "notificationmanager.h"
#include "notificationfeedbackplayer.h"
#include "lipstickcompositor_stub.h"
#include "lipsticknotification.h"
#include "ngfclient_stub.h"
#include "ut_notificationfeedbackplayer.h"

#include <QWaylandSurface>

NotificationManager::NotificationManager(QObject *parent, bool owner) : QObject(parent)
{
    Q_UNUSED(owner)
}

NotificationManager::~NotificationManager()
{
}

void NotificationManager::invokeAction(const QString &, const QString &)
{
}

void NotificationManager::removeNotificationIfUserRemovable(uint)
{
}

void NotificationManager::removeNotificationsWithCategory(const QString &)
{
}

void NotificationManager::updateNotificationsWithCategory(const QString &)
{
}

void NotificationManager::commit()
{
}

void NotificationManager::removeUserRemovableNotifications()
{
}

void NotificationManager::expire()
{
}

void NotificationManager::reportModifications()
{
}

void NotificationManager::identifiedGetNotifications()
{
}

void NotificationManager::identifiedGetNotificationsByCategory()
{
}

void NotificationManager::identifiedCloseNotification()
{
}

void NotificationManager::identifiedNotify()
{
}

void ClientIdentifier::getPidReply(QDBusPendingCallWatcher *getPidWatcher)
{
    Q_UNUSED(getPidWatcher);
}

void ClientIdentifier::identifyReply(QDBusPendingCallWatcher *identifyWatcher) {
    Q_UNUSED(identifyWatcher);
}

NotificationManager *notificationManagerInstance = 0;
NotificationManager *NotificationManager::instance(bool owner)
{
    if (notificationManagerInstance == 0) {
        notificationManagerInstance = new NotificationManager(qApp, owner);
    }
    return notificationManagerInstance;
}

QHash<uint, LipstickNotification *> notificationManagerNotification;
LipstickNotification *NotificationManager::notification(uint id) const
{
    return notificationManagerNotification.value(id);
}

QList<uint> NotificationManager::notificationIds() const
{
    return notificationManagerNotification.keys();
}

LipstickNotification *createNotification(uint id, int urgency = 0, QVariant priority = QVariant())
{
    QVariantHash hints;
    hints.insert(LipstickNotification::HINT_FEEDBACK, "feedback");
    hints.insert(LipstickNotification::HINT_URGENCY, urgency);
    if (priority.isValid()) {
        hints.insert(LipstickNotification::HINT_PRIORITY, priority);
    }
    LipstickNotification *notification = new LipstickNotification("ut_notificationfeedbackplayer", "", "", id, "", "", "", QStringList(), hints, -1);
    notificationManagerNotification.insert(id, notification);
    return notification;
}

QVariantMap qWaylandSurfaceWindowProperties;
QVariantMap QWaylandSurface::windowProperties() const
{
    return qWaylandSurfaceWindowProperties;
}

void QTimer::singleShot(int, const QObject *receiver, const char *member)
{
    // The "member" string is of form "1member()", so remove the trailing 1 and the ()
    int memberLength = strlen(member) - 3;
    char modifiedMember[memberLength + 1];
    strncpy(modifiedMember, member + 1, memberLength);
    modifiedMember[memberLength] = 0;
    QMetaObject::invokeMethod(const_cast<QObject *>(receiver), modifiedMember, Qt::DirectConnection);
}

void Ut_NotificationFeedbackPlayer::initTestCase()
{
    gLipstickCompositorStub->stubSetReturnValue("instance", new LipstickCompositor());
}

void Ut_NotificationFeedbackPlayer::init()
{
    player = new NotificationFeedbackPlayer();
}

void Ut_NotificationFeedbackPlayer::cleanup()
{
    delete player;

    gClientStub->stubReset();
    gLipstickCompositorStub->stubSetReturnValue("surfaceForId", (QWaylandSurface *)0);
}

void Ut_NotificationFeedbackPlayer::testAddAndRemoveNotification()
{
    gClientStub->stubSetReturnValue("play", (quint32)1);

    // Create a notification
    createNotification(1);
    player->addNotification(1);

    // Check that NGFAdapter::play() was called for the feedback
    QCOMPARE(gClientStub->stubCallCount("play"), 1);
    QCOMPARE(gClientStub->stubLastCallTo("play").parameter<QString>(0), QString("feedback"));

    // Stop has been called as well
    QCOMPARE(gClientStub->stubCallCount("stop"), 1);
    QCOMPARE(gClientStub->stubLastCallTo("stop").parameter<QString>(0), QString("feedback"));

    // Remove the notification
    player->removeNotification(1);

    // Check that NGFAdapter::stop() was called for the notification
    QCOMPARE(gClientStub->stubCallCount("stop"), 2);
    QCOMPARE(gClientStub->stubLastCallTo("stop").parameter<quint32>(0), 1u);

    // Check that NGFAdapter::stop() is not called for an already stopped notification
    player->removeNotification(1);
    QCOMPARE(gClientStub->stubCallCount("stop"), 2);
}

void Ut_NotificationFeedbackPlayer::testWithoutFeedbackId()
{
    // Create a notification
    LipstickNotification *notification = createNotification(1);
    notification->setHints(QVariantHash());
    player->addNotification(1);

    // Check that NGFAdapter::play() was not called
    QCOMPARE(gClientStub->stubCallCount("play"), 0);
}

void Ut_NotificationFeedbackPlayer::testMultipleFeedbackIds()
{
    gClientStub->stubSetReturnValueList("play", QList<quint32>() << 1 << 2);

    // Create a notification
    LipstickNotification *notification = createNotification(1);
    QVariantHash hints(notification->hints());
    hints.insert(LipstickNotification::HINT_FEEDBACK, "feedback,foldback");
    notification->setHints(hints);
    player->addNotification(1);

    // Check that NGFAdapter::play() was called for both feedback events
    QCOMPARE(gClientStub->stubCallsTo("play").count(), 2);
    QSet<QString> events;
    events.insert(gClientStub->stubCallsTo("play").at(0)->parameter<QString>(0));
    events.insert(gClientStub->stubCallsTo("play").at(1)->parameter<QString>(0));
    QCOMPARE(events, QSet<QString>() << QString("feedback") << QString("foldback"));

    QCOMPARE(gClientStub->stubCallsTo("stop").count(), 2);

    // Remove the notification
    player->removeNotification(1);

    QCOMPARE(gClientStub->stubCallsTo("play").count(), 2);

    // Check that NGFAdapter::stop() was called for the events
    QCOMPARE(gClientStub->stubCallsTo("stop").count(), 4);
    QSet<quint32> eventIds;
    eventIds.insert(gClientStub->stubCallsTo("stop").at(2)->parameter<quint32>(0));
    eventIds.insert(gClientStub->stubCallsTo("stop").at(3)->parameter<quint32>(0));
    QCOMPARE(eventIds, QSet<quint32>() << 1 << 2);
}

void Ut_NotificationFeedbackPlayer::testNotificationSoundSuppressed()
{
    gClientStub->stubSetReturnValue("play", (quint32)1);

    // Create a notification
    LipstickNotification *notification = createNotification(1);
    QVariantHash hints(notification->hints());
    hints.insert(LipstickNotification::HINT_SUPPRESS_SOUND, true);
    notification->setHints(hints);
    player->addNotification(1);

    // Check that NGFAdapter::play() was called for the feedback, with audio disabled
    QCOMPARE(gClientStub->stubCallCount("play"), 1);
    QCOMPARE(gClientStub->stubLastCallTo("play").parameter<QString>(0), QString("feedback"));

    QMap<QString, QVariant> properties(gClientStub->stubLastCallTo("play").parameter<QMap<QString, QVariant> >(1));
    QVERIFY(properties.contains("media.audio"));
    QCOMPARE(properties.value("media.audio").toBool(), false);
}

void Ut_NotificationFeedbackPlayer::testUpdateNotification()
{
    gClientStub->stubSetReturnValue("play", (quint32)1);

    // Create a notification
    LipstickNotification *notification = createNotification(1);
    player->addNotification(1);

    // Check that NGFAdapter::play() was called for the feedback
    QCOMPARE(gClientStub->stubCallCount("play"), 1);
    QCOMPARE(gClientStub->stubLastCallTo("play").parameter<QString>(0), QString("feedback"));

    QCOMPARE(gClientStub->stubCallCount("stop"), 1);

    // Update the notification
    player->addNotification(1);

    // Check that NGFAdapter::stop() was called for the notification
    QCOMPARE(gClientStub->stubCallCount("stop"), 3);
    QCOMPARE(gClientStub->stubLastCallTo("stop").parameter<QString>(0), QString("feedback"));

    // Check that NGFAdapter::play() was called again for the feedback
    QCOMPARE(gClientStub->stubCallCount("play"), 2);
    QCOMPARE(gClientStub->stubLastCallTo("play").parameter<QString>(0), QString("feedback"));

    // Change the feedback and update
    QVariantHash hints(notification->hints());
    hints.insert(LipstickNotification::HINT_FEEDBACK, "foldback");
    notification->setHints(hints);
    player->addNotification(1);

    // Check that NGFAdapter::stop() was called again
    QCOMPARE(gClientStub->stubCallCount("stop"), 5);
    QCOMPARE(gClientStub->stubLastCallTo("stop").parameter<QString>(0), QString("foldback"));

    // Check that NGFAdapter::play() was called again
    QCOMPARE(gClientStub->stubCallCount("play"), 3);
    QCOMPARE(gClientStub->stubLastCallTo("play").parameter<QString>(0), QString("foldback"));

    // Remove the feedback and update
    hints = notification->hints();
    hints.remove(LipstickNotification::HINT_FEEDBACK);
    notification->setHints(hints);
    player->addNotification(1);

    QCOMPARE(gClientStub->stubCallCount("play"), 3);

    // Check that NGFAdapter::stop() was not called again
    QCOMPARE(gClientStub->stubCallCount("stop"), 6);
}

void Ut_NotificationFeedbackPlayer::testUpdateNotificationAfterRestart()
{
    // Create a notification
    LipstickNotification *notification = createNotification(1);

    // Mark the notification as restored from storage
    notification->setRestored(true);

    // Update the notification
    player->addNotification(1);

    // Check that NGFAdapter::play() was not called
    QCOMPARE(gClientStub->stubCallCount("play"), 0);
}

QWaylandSurface *surface = (QWaylandSurface *)1;
void Ut_NotificationFeedbackPlayer::testNotificationPreviewsDisabled_data()
{
    QTest::addColumn<QWaylandSurface *>("surface");
    QTest::addColumn<QVariantMap>("windowProperties");
    QTest::addColumn<int>("urgency");
    QTest::addColumn<int>("playCount");

    QVariantMap allNotificationsEnabled;
    QVariantMap applicationNotificationsDisabled;
    QVariantMap systemNotificationsDisabled;
    QVariantMap allNotificationsDisabled;
    allNotificationsEnabled.insert("NOTIFICATION_PREVIEWS_DISABLED", 0);
    applicationNotificationsDisabled.insert("NOTIFICATION_PREVIEWS_DISABLED", 1);
    systemNotificationsDisabled.insert("NOTIFICATION_PREVIEWS_DISABLED", 2);
    allNotificationsDisabled.insert("NOTIFICATION_PREVIEWS_DISABLED", 3);
    QTest::newRow("No surface, application notification") << (QWaylandSurface *)0 << QVariantMap() << 1 << 1;
    QTest::newRow("Surface, no properties, application notification") << surface << QVariantMap() << 1 << 1;
    QTest::newRow("Surface, all notifications enabled, application notification") << surface << allNotificationsEnabled << 1 << 1;
    QTest::newRow("Surface, application notifications disabled, application notification") << surface << applicationNotificationsDisabled << 1 << 0;
    QTest::newRow("Surface, system notifications disabled, application notification") << surface << systemNotificationsDisabled << 1 << 1;
    QTest::newRow("Surface, all notifications disabled, application notification") << surface << allNotificationsDisabled << 1 << 0;
    QTest::newRow("No surface, system notification") << (QWaylandSurface *)0 << QVariantMap() << 2 << 1;
    QTest::newRow("Surface, no properties, system notification") << surface << QVariantMap() << 2 << 1;
    QTest::newRow("Surface, all notifications enabled, system notification") << surface << allNotificationsEnabled << 2 << 1;
    QTest::newRow("Surface, application notifications disabled, system notification") << surface << applicationNotificationsDisabled << 2 << 1;
    QTest::newRow("Surface, system notifications disabled, system notification") << surface << systemNotificationsDisabled << 2 << 0;
    QTest::newRow("Surface, all notifications disabled, system notification") << surface << allNotificationsDisabled << 2 << 0;
}

void Ut_NotificationFeedbackPlayer::testNotificationPreviewsDisabled()
{
    QFETCH(QWaylandSurface *, surface);
    QFETCH(QVariantMap, windowProperties);
    QFETCH(int, urgency);
    QFETCH(int, playCount);

    gLipstickCompositorStub->stubSetReturnValue("surfaceForId", surface);
    qWaylandSurfaceWindowProperties = windowProperties;

    createNotification(1, urgency);
    player->addNotification(1);

    QCOMPARE(gClientStub->stubCallCount("play"), playCount);
}

QTEST_MAIN(Ut_NotificationFeedbackPlayer)
