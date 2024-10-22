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

#ifndef NOTIFICATIONMANAGER_H
#define NOTIFICATIONMANAGER_H

#include "lipstickglobal.h"
#include "lipsticknotification.h"
#include <QObject>
#include <QTimer>
#include <QSet>
#include <QDBusContext>
#include <QDBusConnection>
#include <QDBusMessage>

class AndroidPriorityStore;
class CategoryDefinitionStore;
class QSqlDatabase;
class QDBusPendingCallWatcher;

/*!
 * \class ClientIdentifier
 *
 * \brief Asynchronous D-Bus client identification
 *
 * First makes standard GetConnectionUnixProcessID() query to D-Bus daemon.
 * Then, if the resulting pid looks like xdg-dbus-proxy process, makes
 * Sailfish OS specific Identify() query to the proxy in order to get
 * details of actual client behind the proxy.
 *
 * Emits finished() signal when done, at which state clientPid() will return
 * pid of the client process or -1 if client could not be identified.
 */
class ClientIdentifier : public QObject
{
    Q_OBJECT
public:
    ClientIdentifier(QObject *parent, const QDBusConnection &connection, const QDBusMessage &message);
    QDBusConnection &connection() { return m_connection; }
    QDBusMessage &message() { return m_message; }
    QString member() { return message().member(); }
    QString clientName() { return message().service(); }
    int clientPid() { return m_clientPid; }
Q_SIGNALS:
    void finished();
private Q_SLOTS:
    void getPidReply(QDBusPendingCallWatcher *watcher);
    void identifyReply(QDBusPendingCallWatcher *watcher);
private:
    void finish();
    QDBusConnection m_connection;
    QDBusMessage m_message;
    int m_clientPid;
};

/*!
 * \class NotificationManager
 *
 * \brief The notification manager allows applications to display notifications to the user.
 *
 * The notification manager implements a desktop notifications service based
 * on the <a href="http://www.galago-project.org/specs/notification/0.9/">Desktop Notifications Specification</a>.
 * The service is registered as org.freedesktop.Notifications on the D-Bus
 * session bus in the path /org/freedesktop/Notifications.
 */
class LIPSTICK_EXPORT NotificationManager : public QObject, public QDBusContext
{
    Q_OBJECT

public:
    //! Notification closing reasons used in the NotificationClosed signal
    enum NotificationClosedReason {
        //! The notification expired.
        NotificationExpired = 1,
        //! The notification was dismissed by the user.
        NotificationDismissedByUser,
        //! The notification was closed by a call to CloseNotification.
        CloseNotificationCalled
    };

    /*!
     * Returns a singleton instance of the notification manager.
     *
     * \param owner true if the calling application is taking ownership of the notifications data
     * \return an instance of the notification manager
     */
    static NotificationManager *instance(bool owner = true);

    /*!
     * Returns a notification with the given ID.
     *
     * \param id the ID of the notification to return
     * \return the notification with the given ID
     */
    LipstickNotification *notification(uint id) const;

    /*!
     * Returns a list of notification IDs.
     *
     * \return a list of notification IDs.
     */
    QList<uint> notificationIds() const;

    /*!
     * Returns an array of strings. Each string describes an optional capability
     * implemented by the server. Refer to the Desktop Notification Specifications for
     * the defined capabilities.
     *
     * \return an array of strings, each string describing an optional capability implemented by the server
     */
    QStringList GetCapabilities();

    /*!
     * Sends a notification to the notification server.
     *
     * \param appName The optional name of the application sending the notification. Can be blank.
     * \param replacesId The optional notification ID that this notification replaces. The server must atomically
     *  (ie with no flicker or other visual cues) replace the given notification with this one. This allows clients
     *   to effectively modify the notification while it's active. A value of value of 0 means that this notification
     *   won't replace any existing notifications.
     * \param appIcon The optional program icon of the calling application. Can be an empty string, indicating no icon.
     * \param summary The summary text briefly describing the notification.
     * \param body The optional detailed body text. Can be empty.
     * \param actions Actions are sent over as a list of pairs. Each even element in the list (starting at index 0)
     *  represents the identifier for the action. Each odd element in the list is the localized string that will be
     *  displayed to the user.
     * \param hints Optional hints that can be passed to the server from the client program.
     *  Although clients and servers should never assume each other supports any specific hints, they can be used
     *  to pass along information, such as the process PID or window ID, that the server may be able to make use of.
     *  Can be empty.
     * \param expireTimeout he timeout time in milliseconds since the display of the notification at which
     * the notification should automatically close.  If -1, the notification's expiration time is dependent on
     * the notification server's settings, and may vary for the type of notification. If 0, never expire.
     */
    uint Notify(const QString &appName, uint replacesId, const QString &appIcon, const QString &summary,
                const QString &body, const QStringList &actions, const QVariantHash &hints, int expireTimeout);

    /*!
     * Causes a notification to be forcefully closed and removed from the user's view.
     * It can be used, for example, in the event that what the notification pertains
     * to is no longer relevant, or to cancel a notification with no expiration time.
     * The NotificationClosed signal is emitted by this method.
     *
     * \param id the ID of the notification to be closed
     * \param closeReason the reason for the closure of this notification
     */
    void CloseNotification(uint id, NotificationClosedReason closeReason = CloseNotificationCalled);

    /*!
     * Mark the notification as displayed.  If the notification has an expiry timeout
     * value defined, it will apply from when the notification is marked as displayed.
     *
     * \param id the ID of the notification to be closed
     */
    void markNotificationDisplayed(uint id);

    /*!
     * This message returns the information on the server. Specifically, the server name, vendor,
     * and version number.
     *
     * \param vendor The vendor name. For example, "KDE," "GNOME," "freedesktop.org," or "Microsoft."
     * \param version The server's version number.
     * \param spec_version The specification version the server is compliant with.
     * \return The product name of the server
     */
    QString GetServerInformation(QString &vendor, QString &version, QString &spec_version);

    /*!
     * Returns the notifications sent by a specified application.
     *
     * \param owner the identifier of the application to get notifications for
     * \return a list of notifications for the application
     */
    NotificationList GetNotifications(const QString &owner);

    /*!
     * Returns notifications that match to the specified category.
     * This requires privileged access rights.
     * \param category notification category
     * \return a list of notifications
     */
    NotificationList GetNotificationsByCategory(const QString &category);

    // App name for system notifications originating from Lipstick itself
    QString systemApplicationName() const;

signals:
    /*!
     * A completed notification is one that has timed out, or has been dismissed by the user.
     *
     * \param id The ID of the notification that was closed.
     * \param reason The reason the notification was closed.
     * 1 - The notification expired.
     * 2 - The notification was dismissed by the user.
     * 3 - The notification was closed by a call to CloseNotification.
     * 4 - Undefined/reserved reasons.
     */
    void NotificationClosed(uint id, uint reason);

    /*!
     * This signal is emitted when one of the following occurs:
     *   * The user performs some global "invoking" action upon a notification. For instance, clicking somewhere
     *   on the notification itself.
     *   * The user invokes a specific action as specified in the original Notify request. For example,
     *    clicking on an action button.
     *
     * \param id The ID of the notification emitting the ActionInvoked signal.
     * \param actionKey The key of the action invoked. These match the keys sent over in the list of actions.
     */
    void ActionInvoked(uint id, const QString &actionKey);

    /*!
     * Emitted when a notification is added.
     *
     * \param id the ID of the added notification
     */
    void notificationAdded(uint id);

    /*!
     * Emitted when a notification is modified.
     *
     * \param id the ID of the modified notification
     */
    void notificationModified(uint id);

    /*!
     * Batched group of modified notifications, emitted within a second from changes
     *
     * \param ids the IDs of the modified notifications
     */
    void notificationsModified(const QList<uint> &ids);

    /*!
     * Emitted when a notification is removed.
     *
     * \param id the ID of the removed notification
     */
    void notificationRemoved(uint id);

    /*!
     * Emitted when a group of notifications is collectively removed. notificationRemoved() is still
     * called for each instance too.
     *
     * \param ids the IDs of the removed notifications
     */
    void notificationsRemoved(const QList<uint> &ids);

    void remoteActionActivated(const QString &remoteAction, bool trusted);
    void remoteTextActionActivated(const QString &remoteAction, const QString &text, bool trusted);

public slots:
    /*!
     * Removes all notifications which are user removable.
     */
    void removeUserRemovableNotifications();

private slots:
    /*!
     * D-Bus client that made Notify() call has been identified
     */
    void identifiedNotify();

    /*!
     * D-Bus client that made CloseNotification() call has been identified
     */
    void identifiedCloseNotification();

    /*!
     * D-Bus client that made GetNotifications() call has been identified
     */
    void identifiedGetNotifications();

    /*!
     * D-Bus client that made GetNotificationsByCategory() call has been identified
     */
    void identifiedGetNotificationsByCategory();

    /*!
     * Removes all notifications with the specified category.
     *
     * \param category the category of the notifications to remove
     */
    void removeNotificationsWithCategory(const QString &category);

    /*!
     * Update category data of all notifications with the
     * specified category.
     *
     * \param category the category of the notifications to update
     */
    void updateNotificationsWithCategory(const QString &category);

    /*!
     * Commits the current database transaction, if any.
     * Also destroys any removed notifications.
     */
    void commit();

    /*!
     * Invokes the given action if it is has been defined. The
     * sender is expected to be a Notification.
     */
    void invokeAction(const QString &action, const QString &actionText);

    /*!
     * Removes a notification if it is removable by the user.
     *
     * \param id the ID of the notification to be removed
     */
    void removeNotificationIfUserRemovable(uint id = 0);

    /*!
     * Expires any notifications whose expiration time has been reached.
     */
    void expire();

    /*!
     * Reports any notifications that have been modified since the last report.
     */
    void reportModifications();

private:
    bool isInternalOperation() const;
    /*!
     * Actual Notify() work. In case of D-Bus ipc, called after client identification.
     */
    uint handleNotify(int clientPid, const QString &appName, uint replacesId, const QString &appIcon,
                      const QString &summary, const QString &body, const QStringList &actions,
                      const QVariantHash &hints, int expireTimeout);

    /*!
     * Actual CloseNotification() work. In case of D-Bus ipc, called after client identification.
     */
    void handleCloseNotification(int clientPid, uint id, NotificationClosedReason closeReason);

    /*!
     * Actual GetNotifications() work. In case of D-Bus ipc, called after client identification.
     */
    NotificationList handleGetNotifications(int clientPid, const QString &owner);

    /*!
     * Actual GetNotificationsByCategory() work. In case of D-Bus ipc, called after client identification.
     */
    NotificationList handleGetNotificationsByCategory(int clientPid, const QString &category);

    /*!
     * Creates a new notification manager.
     *
     * \param parent the parent object
     * \param owner true if the manager is taking ownership of the notifications data
     */
    NotificationManager(QObject *parent, bool owner);

    //! Destroys the notification manager.
    virtual ~NotificationManager();

    /*!
     * Returns the next available notification ID
     *
     * \return The next available notification ID
     */
    uint nextAvailableNotificationID();

    /*!
     * Returns all key-value pairs in the requested category definition.
     *
     * \param hints the notification hints from which to determine the category definition to report
     */
    QHash<QString, QString> categoryDefinitionParameters(const QVariantHash &hints) const;

    /*!
     * Update a notification by applying the changes implied by the catgeory definition.
     */
    void applyCategoryDefinition(LipstickNotification *notification) const;

    /*!
     * Makes a notification known to the system, or updates its properties if already published.
     */
    void publish(const LipstickNotification *notification, uint replacesId);

    //! Restores the notifications from a database on the disk
    void restoreNotifications(bool update);

    /*!
     * Creates a connection to the Sqlite database.
     *
     * \return \c true if the connection was successfully established, \c false otherwise
     */
    bool connectToDatabase();

    /*!
     * Deletes a notification from the system, without any reporting.
     */
    void deleteNotification(uint id);

    /*!
     * Causes all listed notifications to be forcefully closed and removed from the user's view.
     * The NotificationClosed signal is emitted by this method for each closed notification.
     *
     * \param ids the IDs of the notifications to be closed
     * \param closeReason the reason for the closure of these notifications
     */
    void closeNotifications(const QList<uint> &ids, NotificationClosedReason closeReason = CloseNotificationCalled);

    /*!
     * Checks whether there is enough free disk space available.
     *
     * \param path any path to the file system from which the space should be checked
     * \param freeSpaceNeeded free space needed in kilobytes
     * \return \c true if there is enough free space in given file system, \c false otherwise
     */
    static bool checkForDiskSpace(const QString &path, unsigned long freeSpaceNeeded);

    /*!
     * Removes a database file from the filesystem. Removes related -wal and -shm files as well.
     *
     * \param path the path of the database file to be removed
     */
    static void removeDatabaseFile(const QString &path);

    /*!
     * Ensures that all database tables have the requires fields.
     * Recreates the tables if needed.
     *
     * \return \c true if the database can be used, \c false otherwise
     */
    bool checkTableValidity();

    /*!
     * Returns the schema version of the database.
     *
     * \return the version number the database schema is currently set to.
     */
    int schemaVersion();

    /*!
     * Sets the schema version of the database.
     *
     * \param version the version number to set the database schema to.
     * \return \c true if the database is updated.
     */
    bool setSchemaVersion(int version);

    /*!
     * Returns true if all listed columns are present in the table in the database.
     *
     * \param tableName the name of the table to be verified
     * \param columnNames the list of columns that should be present in the table
     * \return \c true if the columns are all present, \c false otherwise
     */
    bool verifyTableColumns(const QString &tableName, const QStringList &columnNames);

    /*!
     * Recreates a table in the database.
     *
     * \param tableName the name of the table to be created
     * \param definition SQL definition for the table
     * \return \c true if the table was created, \c false otherwise
     */
    bool recreateTable(const QString &tableName, const QString &definition);

    //! Fills the notifications hash table with data from the database
    void fetchData(bool update);

    /*!
     * Executes a SQL command in the database. Starts a new transaction if none is active currently, otherwise
     * the command goes to the active transaction. Restarts the transaction commit timer.
     * \param command the SQL command
     * \param args list of values to be bound to the positional placeholders ('?' -character) in the command.
     */
    void execSQL(const QString &command, const QVariantList &args = QVariantList());

    //! The singleton notification manager instance
    static NotificationManager *s_instance;

    //! Hash of all notifications keyed by notification IDs
    QHash<uint, LipstickNotification*> m_notifications;

    //! Notifications waiting to be destroyed
    QSet<LipstickNotification *> m_removedNotifications;

    //! Previous notification ID used
    uint m_previousNotificationID;

    //! The category definition store
    CategoryDefinitionStore *m_categoryDefinitionStore;

    //! The Android application priority store
    AndroidPriorityStore *m_androidPriorityStore;

    //! Database for the notifications
    QSqlDatabase *m_database;

    //! Whether the current database transaction has been committed to the database
    bool m_committed;

    //! Timer for triggering the commit of the current database transaction
    QTimer m_databaseCommitTimer;

    //! Timer for triggering the expiration of displayed notifications
    QTimer m_expirationTimer;

    //! Next trigger time for the expirationTimer, relative to epoch
    qint64 m_nextExpirationTime;

    //! IDs of notifications modified since the last report
    QSet<uint> m_modifiedIds;

    //! Timer for triggering the reporting of modified notifications
    QTimer m_modificationTimer;

#ifdef UNIT_TEST
    friend class Ut_NotificationManager;
#endif
};

#endif // NOTIFICATIONMANAGER_H
