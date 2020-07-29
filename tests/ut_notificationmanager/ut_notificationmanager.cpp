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

#include <QtTest/QtTest>
#include "ut_notificationmanager.h"
#include "aboutsettings_stub.h"

#include "notificationmanager.h"
#include "notificationmanageradaptor_stub.h"
#include "lipsticknotification.h"
#include "categorydefinitionstore_stub.h"
#include "androidprioritystore_stub.h"
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QSqlRecord>
#include <QSqlError>
#include <mremoteaction.h>


// MRemoteAction stubs
QStringList mRemoteActionTrigger;
void MRemoteAction::trigger()
{
    mRemoteActionTrigger.append(toString());
}

void Ut_NotificationManager::init()
{
    mRemoteActionTrigger.clear();
}

void Ut_NotificationManager::cleanup()
{
    delete NotificationManager::s_instance;
    NotificationManager::s_instance = 0;
}

void Ut_NotificationManager::testManagerIsSingleton()
{
    NotificationManager *manager1 = NotificationManager::instance();
    NotificationManager *manager2 = NotificationManager::instance();
    QVERIFY(manager1 != NULL);
    QCOMPARE(manager2, manager1);
}

void Ut_NotificationManager::testCapabilities()
{
    // Check the supported capabilities includes all the Nemo hints
    QStringList capabilities = NotificationManager::instance()->GetCapabilities();
    QCOMPARE(capabilities.count(), 14);
    QCOMPARE((bool)capabilities.contains("body"), true);
    QCOMPARE((bool)capabilities.contains("actions"), true);
    QCOMPARE((bool)capabilities.contains("persistence"), true);
    QCOMPARE((bool)capabilities.contains("sound"), true);
    QCOMPARE((bool)capabilities.contains(LipstickNotification::HINT_APP_ICON), true);
    QCOMPARE((bool)capabilities.contains(LipstickNotification::HINT_ITEM_COUNT), true);
    QCOMPARE((bool)capabilities.contains(LipstickNotification::HINT_TIMESTAMP), true);
    QCOMPARE((bool)capabilities.contains(LipstickNotification::HINT_PREVIEW_BODY), true);
    QCOMPARE((bool)capabilities.contains(LipstickNotification::HINT_PREVIEW_SUMMARY), true);
    QCOMPARE((bool)capabilities.contains("x-nemo-remote-actions"), true);
    QCOMPARE((bool)capabilities.contains(LipstickNotification::HINT_USER_REMOVABLE), true);
    QCOMPARE((bool)capabilities.contains("x-nemo-get-notifications"), true);
    QCOMPARE((bool)capabilities.contains(LipstickNotification::HINT_ORIGIN), true);
    QCOMPARE((bool)capabilities.contains(LipstickNotification::HINT_MAX_CONTENT_LINES), true);
}

void Ut_NotificationManager::testRemovingInexistingNotification()
{
    NotificationManager *manager = NotificationManager::instance();
    QSignalSpy removedSpy(manager, SIGNAL(notificationRemoved(uint)));
    QSignalSpy closedSpy(manager, SIGNAL(NotificationClosed(uint, uint)));
    manager->CloseNotification(1253554); // random enough not to exist...
    QCOMPARE(removedSpy.count(), 0);
    QCOMPARE(closedSpy.count(), 0);
}

void Ut_NotificationManager::testServerInformation()
{
    // Check that the server information uses application information from qApp
    QString name, vendor, version, spec_version;
    qApp->setApplicationName("testApp");
    qApp->setApplicationVersion("1.2.3");
    QString testVendor("test vendor");
    gAboutSettingsStub->stubSetReturnValue(OperatingSystemName, testVendor);

    name = NotificationManager::instance()->GetServerInformation(vendor, version, spec_version);
    QCOMPARE(name, qApp->applicationName());
    QCOMPARE(vendor, testVendor);
    QCOMPARE(version, qApp->applicationVersion());
    QCOMPARE(spec_version, QString("1.2"));
}

void Ut_NotificationManager::testModifyingCategoryDefinitionUpdatesNotifications()
{
    NotificationManager *manager = NotificationManager::instance();

    // Check the signal connection
    QCOMPARE(disconnect(manager->m_categoryDefinitionStore, SIGNAL(categoryDefinitionModified(QString)),
                        manager, SLOT(updateNotificationsWithCategory(QString))), true);

    QSignalSpy multiModifiedSpy(manager, SIGNAL(notificationsModified(QList<uint>)));

    // Add two notifications, one with category "category1" and one with category "category2"
    QVariantHash hints1;
    QVariantHash hints2;
    hints1.insert(LipstickNotification::HINT_CATEGORY, "category1");
    hints1.insert(LipstickNotification::HINT_PREVIEW_BODY, "previewBody1");
    hints1.insert(LipstickNotification::HINT_PREVIEW_SUMMARY, "previewSummary1");
    hints2.insert(LipstickNotification::HINT_CATEGORY, "category2");
    hints2.insert(LipstickNotification::HINT_PREVIEW_BODY, "previewBody2");
    hints2.insert(LipstickNotification::HINT_PREVIEW_SUMMARY, "previewSummary2");
    uint id1 = manager->Notify("app1", 0, QString(), QString(), QString(), QStringList(), hints1, 0);
    uint id2 = manager->Notify("app2", 0, QString(), QString(), QString(), QStringList(), hints2, 0);

    QTRY_COMPARE(multiModifiedSpy.count(), 1);
    QList<uint> modifiedIds(multiModifiedSpy.last().at(0).value<QList<uint> >());
    QCOMPARE(modifiedIds.count(), 2);
    QVERIFY(modifiedIds.contains(id1));
    QVERIFY(modifiedIds.contains(id2));

    // Updating notifications with category "category2" should only update the notification with that category
    QSignalSpy modifiedSpy(manager, SIGNAL(notificationModified(uint)));
    manager->updateNotificationsWithCategory("category2");
    QTRY_COMPARE(modifiedSpy.count(), 1);
    QCOMPARE(modifiedSpy.last().at(0).toUInt(), id2);

    // The updated notifications should be marked as restored until modified
    QCOMPARE(manager->notification(id1)->restored(), false);
    QCOMPARE(manager->notification(id2)->restored(), true);
}

void Ut_NotificationManager::testUninstallingCategoryDefinitionRemovesNotifications()
{
    NotificationManager *manager = NotificationManager::instance();

    // Check the signal connection
    QCOMPARE(disconnect(manager->m_categoryDefinitionStore, SIGNAL(categoryDefinitionUninstalled(QString)),
                        manager, SLOT(removeNotificationsWithCategory(QString))), true);

    // Add two notifications, one with category "category1" and one with category "category2"
    QVariantHash hints1;
    QVariantHash hints2;
    hints1.insert(LipstickNotification::HINT_CATEGORY, "category1");
    hints2.insert(LipstickNotification::HINT_CATEGORY, "category2");
    uint id1 = manager->Notify("app1", 0, QString(), QString(), QString(), QStringList(), hints1, 0);
    uint id2 = manager->Notify("app2", 0, QString(), QString(), QString(), QStringList(), hints2, 0);

    // Removing notifications with category "category2" should only remove the notification with that category
    QSignalSpy removedSpy(manager, SIGNAL(notificationRemoved(uint)));
    manager->removeNotificationsWithCategory("category2");
    QCOMPARE(removedSpy.count(), 1);
    QCOMPARE(removedSpy.last().at(0).toUInt(), id2);
    QVERIFY(manager->notification(id1) != 0);
    QCOMPARE(manager->notification(id2), (LipstickNotification *)0);
}

void Ut_NotificationManager::testActionIsInvokedIfDefined()
{
    // Add two notifications, only the first one with an action named "action1"
    NotificationManager *manager = NotificationManager::instance();
    uint id1 = manager->Notify("app1", 0, QString(), QString(), QString(), QStringList() << "action1" << "Action 1", QVariantHash(), 0);
    uint id2 = manager->Notify("app2", 0, QString(), QString(), QString(), QStringList() << "action2" << "Action 2", QVariantHash(), 0);
    LipstickNotification *notification1 = manager->notification(id1);
    LipstickNotification *notification2 = manager->notification(id2);
    connect(this, SIGNAL(actionInvoked(QString)), notification1, SIGNAL(actionInvoked(QString)));
    connect(this, SIGNAL(actionInvoked(QString)), notification2, SIGNAL(actionInvoked(QString)));

    // Make both notifications emit the actionInvoked() signal for action "action1"; only the first one contains it and should be invoked
    QSignalSpy spy(manager, SIGNAL(ActionInvoked(uint, QString)));
    emit actionInvoked("action1");
    QCoreApplication::processEvents();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.last().at(0).toUInt(), id1);
    QCOMPARE(spy.last().at(1).toString(), QString("action1"));
}

void Ut_NotificationManager::testActionIsNotInvokedIfIncomplete()
{
    // Add two notifications, the first one with an incomplete action named "action1"
    NotificationManager *manager = NotificationManager::instance();
    uint id1 = manager->Notify("app1", 0, QString(), QString(), QString(), QStringList() << "action1", QVariantHash(), 0);
    uint id2 = manager->Notify("app2", 0, QString(), QString(), QString(), QStringList() << "action2" << "Action 2", QVariantHash(), 0);
    LipstickNotification *notification1 = manager->notification(id1);
    LipstickNotification *notification2 = manager->notification(id2);
    connect(this, SIGNAL(actionInvoked(QString)), notification1, SIGNAL(actionInvoked(QString)));
    connect(this, SIGNAL(actionInvoked(QString)), notification2, SIGNAL(actionInvoked(QString)));

    // Make both notifications emit the actionInvoked() signal for action "action1"; no action should be invoked
    QSignalSpy spy(manager, SIGNAL(ActionInvoked(uint, QString)));
    emit actionInvoked("action1");
    QCoreApplication::processEvents();
    QCOMPARE(spy.count(), 0);
}

void Ut_NotificationManager::testRemoteActionIsInvokedIfDefined()
{
    // Add a notifications with an action named "action"
    NotificationManager *manager = NotificationManager::instance();
    QVariantHash hints;
    hints.insert(QString(LipstickNotification::HINT_REMOTE_ACTION_PREFIX) + "action", "a b c d");
    uint id = manager->Notify("app", 0, QString(), QString(), QString(), QStringList(), hints, 0);
    LipstickNotification *notification = manager->notification(id);
    connect(this, SIGNAL(actionInvoked(QString)), notification, SIGNAL(actionInvoked(QString)));

    // Invoking the notification should trigger the remote action
    emit actionInvoked("action");
    QCoreApplication::processEvents();
    QCOMPARE(mRemoteActionTrigger.count(), 1);
    QCOMPARE(mRemoteActionTrigger.last(), hints.value(QString(LipstickNotification::HINT_REMOTE_ACTION_PREFIX) + "action").toString());
}

void Ut_NotificationManager::testInvokingActionClosesNotificationIfUserRemovable()
{
    // Add three notifications with user removability not set, set to true and set to false
    NotificationManager *manager = NotificationManager::instance();
    QVariantHash hints1;
    QVariantHash hints2;
    QVariantHash hints3;
    QVariantHash hints6;
    hints2.insert(LipstickNotification::HINT_USER_REMOVABLE, true);
    hints3.insert(LipstickNotification::HINT_USER_REMOVABLE, false);
    hints6.insert(LipstickNotification::HINT_RESIDENT, true); // 'resident' hint also prevents automatic closure
    uint id1 = manager->Notify("app1", 0, QString(), QString(), QString(), QStringList(), hints1, 0);
    uint id2 = manager->Notify("app2", 0, QString(), QString(), QString(), QStringList(), hints2, 0);
    uint id3 = manager->Notify("app3", 0, QString(), QString(), QString(), QStringList(), hints3, 0);
    uint id6 = manager->Notify("app6", 0, QString(), QString(), QString(), QStringList(), hints6, 0);
    connect(this, SIGNAL(actionInvoked(QString)), manager->notification(id1), SIGNAL(actionInvoked(QString)));
    connect(this, SIGNAL(actionInvoked(QString)), manager->notification(id2), SIGNAL(actionInvoked(QString)));
    connect(this, SIGNAL(actionInvoked(QString)), manager->notification(id3), SIGNAL(actionInvoked(QString)));
    connect(this, SIGNAL(actionInvoked(QString)), manager->notification(id6), SIGNAL(actionInvoked(QString)));

    // Make all notifications emit the actionInvoked() signal for action "action"; removable notifications should get removed
    QSignalSpy removedSpy(manager, SIGNAL(notificationRemoved(uint)));
    QSignalSpy closedSpy(manager, SIGNAL(NotificationClosed(uint, uint)));
    emit actionInvoked("action");
    QCoreApplication::processEvents();
    QCOMPARE(removedSpy.count(), 2);
    QCOMPARE(removedSpy.at(0).at(0).toUInt(), id1);
    QCOMPARE(removedSpy.at(1).at(0).toUInt(), id2);
    QCOMPARE(closedSpy.count(), 2);
    QCOMPARE(closedSpy.at(0).at(0).toUInt(), id1);
    QCOMPARE(closedSpy.at(0).at(1).toInt(), (int)NotificationManager::NotificationDismissedByUser);
    QCOMPARE(closedSpy.at(1).at(0).toUInt(), id2);
    QCOMPARE(closedSpy.at(1).at(1).toInt(), (int)NotificationManager::NotificationDismissedByUser);
}

void Ut_NotificationManager::testListingNotifications()
{
    NotificationManager *manager = NotificationManager::instance();

    // Add three notifications, two for application appName1 and one for appName2
    QVariantHash hints1;
    QVariantHash hints2;
    QVariantHash hints3;
    hints1.insert(LipstickNotification::HINT_CATEGORY, "category1");
    hints2.insert(LipstickNotification::HINT_CATEGORY, "category2");
    hints3.insert(LipstickNotification::HINT_CATEGORY, "category3");
    uint id1 = manager->Notify("appName1", 0, "appIcon1", "summary1", "body1", QStringList() << "action1", hints1, 1);
    uint id2 = manager->Notify("appName1", 0, "appIcon2", "summary2", "body2", QStringList() << "action2", hints2, 2);
    uint id3 = manager->Notify("appName2", 0, "appIcon3", "summary3", "body3", QStringList() << "action3", hints3, 3);

    Q_UNUSED(id1)
    Q_UNUSED(id2)
    Q_UNUSED(id3)
#ifdef TODO
    // Check that only notifications for the given application are returned and that they contain all the information
    QDBusArgument notifications = manager->GetNotifications("appName1");
    QCOMPARE(notifications.count(), 2);
    QCOMPARE(notifications.at(0).appName(), QString("appName1"));
    QCOMPARE(notifications.at(1).appName(), QString("appName1"));
    QCOMPARE(notifications.at(0).id(), id1);
    QCOMPARE(notifications.at(1).id(), id2);
    QCOMPARE(notifications.at(0).appIcon(), QString("appIcon1"));
    QCOMPARE(notifications.at(1).appIcon(), QString("appIcon2"));
    QCOMPARE(notifications.at(0).summary(), QString("summary1"));
    QCOMPARE(notifications.at(1).summary(), QString("summary2"));
    QCOMPARE(notifications.at(0).body(), QString("body1"));
    QCOMPARE(notifications.at(1).body(), QString("body2"));
    QCOMPARE(notifications.at(0).actions(), QStringList() << "action1");
    QCOMPARE(notifications.at(1).actions(), QStringList() << "action2");
    QCOMPARE(notifications.at(0).category(), QVariant("category1"));
    QCOMPARE(notifications.at(1).category(), QVariant("category2"));
    QCOMPARE(notifications.at(0).expireTimeout(), 1);
    QCOMPARE(notifications.at(1).expireTimeout(), 2);

    notifications = manager->GetNotifications("appName2");
    QCOMPARE(notifications.count(), 1);
    QCOMPARE(notifications.at(0).appName(), QString("appName2"));
    QCOMPARE(notifications.at(0).id(), id3);
    QCOMPARE(notifications.at(0).appIcon(), QString("appIcon3"));
    QCOMPARE(notifications.at(0).summary(), QString("summary3"));
    QCOMPARE(notifications.at(0).body(), QString("body3"));
    QCOMPARE(notifications.at(0).actions(), QStringList() << "action3");
    QCOMPARE(notifications.at(0).category(), QVariant("category3"));
    QCOMPARE(notifications.at(0).expireTimeout(), 3);
#endif
}

void Ut_NotificationManager::testRemoveUserRemovableNotifications()
{
    NotificationManager *manager = NotificationManager::instance();
    QVariantHash hints1;
    QVariantHash hints2;
    QVariantHash hints3;
    QVariantHash hints6;
    hints2.insert(LipstickNotification::HINT_USER_REMOVABLE, true);
    hints3.insert(LipstickNotification::HINT_USER_REMOVABLE, false);
    hints6.insert(LipstickNotification::HINT_RESIDENT, true);
    uint id1 = manager->Notify("app1", 0, QString(), QString(), QString(), QStringList(), hints1, 0);
    uint id2 = manager->Notify("app2", 0, QString(), QString(), QString(), QStringList(), hints2, 0);
    manager->Notify("app3", 0, QString(), QString(), QString(), QStringList(), hints3, 0);
    uint id6 = manager->Notify("app6", 0, QString(), QString(), QString(), QStringList(), hints6, 0);

    QSignalSpy removedSpy(manager, SIGNAL(notificationRemoved(uint)));
    QSignalSpy closedSpy(manager, SIGNAL(NotificationClosed(uint, uint)));
    manager->removeUserRemovableNotifications();

    QSet<uint> removedIds;
    for (int i = 0; i < removedSpy.count(); i++) {
        removedIds.insert(removedSpy.at(i).at(0).toUInt());
    }
    QCOMPARE(removedIds.count(), 3);
    QCOMPARE(removedIds.contains(id1), true);
    QCOMPARE(removedIds.contains(id2), true);
    QCOMPARE(removedIds.contains(id6), true);

    QSet<uint> closedIds;
    for (int i = 0; i < closedSpy.count(); i++) {
        closedIds.insert(closedSpy.at(i).at(0).toUInt());
        QCOMPARE(closedSpy.at(i).at(1).toInt(), (int)NotificationManager::NotificationDismissedByUser);
    }
    QCOMPARE(closedIds.count(), 3);
    QCOMPARE(closedIds.contains(id1), true);
    QCOMPARE(closedIds.contains(id2), true);
    QCOMPARE(closedIds.contains(id6), true);
}

void Ut_NotificationManager::testRemoveRequested()
{
    NotificationManager *manager = NotificationManager::instance();
    uint id1 = manager->Notify("app1", 0, QString(), QString(), QString(), QStringList() << "action1" << "Action 1", QVariantHash(), 0);
    LipstickNotification *notification1 = manager->notification(id1);
    connect(this, SIGNAL(removeRequested()), notification1, SIGNAL(removeRequested()));

    QSignalSpy removedSpy(manager, SIGNAL(notificationRemoved(uint)));
    QSignalSpy closedSpy(manager, SIGNAL(NotificationClosed(uint, uint)));
    emit removeRequested();
    QCoreApplication::processEvents();
    QCOMPARE(removedSpy.count(), 1);
    QCOMPARE(removedSpy.last().at(0).toUInt(), id1);
    QCOMPARE(closedSpy.count(), 1);
    QCOMPARE(closedSpy.last().at(0).toUInt(), id1);
    QCOMPARE(closedSpy.last().at(1).toUInt(), static_cast<uint>(NotificationManager::NotificationDismissedByUser));
}

void Ut_NotificationManager::testImmediateExpiration()
{
    QVariantHash hints;
    hints.insert(LipstickNotification::HINT_TRANSIENT, "true");

    NotificationManager *manager = NotificationManager::instance();
    uint id1 = manager->Notify("app1", 0, QString(), QString(), QString(), QStringList(), hints, 0);

    QSignalSpy removedSpy(manager, SIGNAL(notificationRemoved(uint)));
    QSignalSpy closedSpy(manager, SIGNAL(NotificationClosed(uint, uint)));
    manager->markNotificationDisplayed(id1);
    QCoreApplication::processEvents();
    QCOMPARE(removedSpy.count(), 1);
    QCOMPARE(removedSpy.last().at(0).toUInt(), id1);
    QCOMPARE(closedSpy.count(), 1);
    QCOMPARE(closedSpy.last().at(0).toUInt(), id1);
    QCOMPARE(closedSpy.last().at(1).toUInt(), static_cast<uint>(NotificationManager::NotificationExpired));
}

QTEST_MAIN(Ut_NotificationManager)
