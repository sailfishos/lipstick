/***************************************************************************
**
** Copyright (c) 2012 - 2021 Jolla Ltd.
** Copyright (c) 2019 - 2021 Open Mobile Platform LLC.
**
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

#include <QCoreApplication>
#include <QDataStream>
#include <QDBusArgument>
#include <QDebug>
#include <QImage>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlTableModel>
#include <QStandardPaths>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <aboutsettings.h>
#include <mremoteaction.h>
#include <mdesktopentry.h>
#include <sys/statfs.h>
#include <unistd.h>
#include <limits>
#include "androidprioritystore.h"
#include "categorydefinitionstore.h"
#include "notificationmanageradaptor.h"
#include "notificationmanager.h"

// Define this if you'd like to see debug messages from the notification manager
#ifdef DEBUG_NOTIFICATIONS
#define NOTIFICATIONS_DEBUG(things) qDebug() << Q_FUNC_INFO << things
#else
#define NOTIFICATIONS_DEBUG(things)
#endif

//! The android priority store path
static const char *ANDROID_PRIORITY_DEFINITION_PATH = "/usr/share/lipstick/androidnotificationpriorities";

//! The category definitions directory
static const char *CATEGORY_DEFINITION_FILE_DIRECTORY = "/usr/share/lipstick/notificationcategories";

//! The number configuration files to load into the event type store.
static const uint MAX_CATEGORY_DEFINITION_FILES = 100;

//! Path to probe for desktop entries
static const char *DESKTOP_ENTRY_PATH = "/usr/share/applications/";

//! Minimum amount of disk space needed for the notification database in kilobytes
static const uint MINIMUM_FREE_SPACE_NEEDED_IN_KB = 1024;

// Exported for unit test:
int MaxNotificationRestoreCount = 1000;

namespace {

const int DefaultNotificationPriority = 50;

const int CommitDelay = 10 * 1000;
const int PublicationDelay = 1000;

bool processIsPrivileged(int pid)
{
    bool isPrivileged = false;
    if (pid == getpid()) {
        // Internal operations are considered privileged
        isPrivileged = true;
    } else if (pid > 0) {
        QFileInfo info(QString("/proc/%1").arg(pid));
        isPrivileged = info.group() == QLatin1String("privileged") || info.owner() == QLatin1String("root");
    }
    NOTIFICATIONS_DEBUG("pid" << pid << "-> isPrivileged" << isPrivileged);
    return isPrivileged;
}

QString getProcessCmdline(int pid)
{
    QString cmdline;
    if (pid > 0) {
        const QString path(QString::fromLatin1("/proc/%1/cmdline").arg(QString::number(pid)));
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            const QByteArray data = file.readAll();
            cmdline = QString::fromUtf8(data.left(data.indexOf('\0')));
        }
    }
    NOTIFICATIONS_DEBUG("pid" << pid << "-> cmdline" << cmdline);
    return cmdline;
}

QString getProcessComm(int pid)
{
    QString comm;
    if (pid > 0) {
        const QString path(QString::fromLatin1("/proc/%1/comm").arg(QString::number(pid)));
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            const QByteArray data = file.readAll();
            comm = QFile::decodeName(data.left(data.indexOf('\n')));
        }
    }
    NOTIFICATIONS_DEBUG("pid" << pid << "-> comm" << comm);
    return comm;
}

QString getProcessExe(int pid)
{
    QString exe;
    if (pid > 0) {
        const QString path(QString::fromLatin1("/proc/%1/exe").arg(QString::number(pid)));
        exe = QFile::symLinkTarget(path);
    }
    NOTIFICATIONS_DEBUG("pid" << pid << "-> exe" << exe);
    return exe;
}

bool processIsDBusProxy(int pid)
{
    bool isDBusProxy = false;
    if (pid > 0) {
        /* Note: Applications can choose what shows up in /proc/pid/comm
         *       and/or cmdline. The exe symlink is under kernel control,
         *       but it might not be accessible by unprivileged processes.
         *       As figuring out originator of a notification is not security
         *       sensitive: prefer using exe info. But if it not accessible,
         *       falling back to using comm / cmdline is acceptable. */
        static const char proxyPath[] = "/usr/bin/xdg-dbus-proxy";
        static const char proxyName[] = "xdg-dbus-proxy";
        const QString exe(getProcessExe(pid));
        if (!exe.isEmpty())
            isDBusProxy = exe == proxyPath;
        else
            isDBusProxy = getProcessComm(pid) == proxyName || getProcessCmdline(pid) == proxyPath;
    }
    NOTIFICATIONS_DEBUG("pid" << pid << "-> isDBusProxy" << isDBusProxy);
    return isDBusProxy;
}

QString getProcessName(int pid)
{
    QString processName = QFileInfo(getProcessCmdline(pid)).fileName();
    NOTIFICATIONS_DEBUG("pid" << pid << "-> processName" << processName);
    return processName;
}

QPair<QString, QString> processProperties(int pid)
{
    // Cache resolution of process name to properties:
    static QHash<QString, QPair<QString, QString> > nameProperties;

    QPair<QString, QString> rv;

    if (pid == getpid()) {
        // This notification comes from our process
        rv.first = QCoreApplication::applicationName();
    } else {
        const QString processName = getProcessName(pid);
        if (!processName.isEmpty()) {
            QHash<QString, QPair<QString, QString> >::iterator it = nameProperties.find(processName);
            if (it == nameProperties.end()) {
                // Look up the desktop entry for this process name
                MDesktopEntry desktopEntry(DESKTOP_ENTRY_PATH + processName + ".desktop");
                if (desktopEntry.isValid()) {
                    it = nameProperties.insert(processName, qMakePair(desktopEntry.name(), desktopEntry.icon()));
                } else {
                    qWarning() << "No desktop entry for process name:" << processName;
                    // Fallback to the processName for application name
                    it = nameProperties.insert(processName, qMakePair(processName, QString()));
                }
            }
            if (it != nameProperties.end()) {
                rv.first = it->first;
                rv.second = it->second;
            }
        } else {
            qWarning() << "Unable to retrieve process name for pid:" << pid;
        }
    }

    return rv;
}

bool notificationReverseOrder(const LipstickNotification *lhs, const LipstickNotification *rhs)
{
    // Sort least significant notifications first
    return *rhs < *lhs;
}

}

ClientIdentifier::ClientIdentifier(QObject *parent, const QDBusConnection &connection, const QDBusMessage &message)
    : QObject(parent)
    , m_connection(connection)
    , m_message(message)
    , m_clientPid(-1)
{
    QDBusMessage request = QDBusMessage::createMethodCall("org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "GetConnectionUnixProcessID");
    request << clientName();
    NOTIFICATIONS_DEBUG("identify" << member() << "from" << clientName() << "...");
    QDBusPendingReply<quint32> call = this->connection().asyncCall(request);
    QDBusPendingCallWatcher *getPidWatcher = new QDBusPendingCallWatcher(call, this);
    connect(getPidWatcher, &QDBusPendingCallWatcher::finished, this, &ClientIdentifier::getPidReply);
}

void ClientIdentifier::getPidReply(QDBusPendingCallWatcher *getPidWatcher)
{
    bool waiting = false;
    QDBusPendingReply<quint32> reply = *getPidWatcher;
    if (reply.isError()) {
        qWarning() << "identify" << member() << "from" << clientName() << "- GetConnectionUnixProcessID:" << reply.error().name() << reply.error().message();
    } else {
        quint32 pid = reply.value();
        if (pid > 0) {
            m_clientPid = pid;
            if (processIsDBusProxy(pid)) {
                QDBusMessage request = QDBusMessage::createMethodCall(clientName(), "/", "org.sailfishos.sailjailed", "Identify");
                QDBusPendingReply<QVariantMap> call = connection().asyncCall(request);
                QDBusPendingCallWatcher *identifyWatcher = new QDBusPendingCallWatcher(call, this);
                connect(identifyWatcher, &QDBusPendingCallWatcher::finished, this, &ClientIdentifier::identifyReply);
                waiting = true;
            }
        }
    }
    if (!waiting)
        finish();
    getPidWatcher->deleteLater();
}

void ClientIdentifier::identifyReply(QDBusPendingCallWatcher *identifyWatcher)
{
    QDBusPendingReply<QVariantMap> reply = *identifyWatcher;
    if (reply.isError()) {
        qWarning() << "identify" << member() << "from" << clientName() << " - Identify:" << reply.error().name() << reply.error().message();
    } else {
        QVariantMap map(reply.value());
        if (map.contains("pid")) {
            bool ack = false;
            int pid = map["pid"].toInt(&ack);
            if (ack && pid > 0) {
                m_clientPid = pid;
            }
        }
    }
    finish();
    identifyWatcher->deleteLater();
}

void ClientIdentifier::finish()
{
    NOTIFICATIONS_DEBUG("identify" << member() << "from" << clientName() << "-> using pid" << clientPid());
    Q_EMIT finished();
}

NotificationManager *NotificationManager::s_instance = 0;

NotificationManager *NotificationManager::instance(bool owner)
{
    if (s_instance == 0) {
        s_instance = new NotificationManager(qApp, owner);
    }
    return s_instance;
}

NotificationManager::NotificationManager(QObject *parent, bool owner)
    : QObject(parent),
      QDBusContext(),
      m_previousNotificationID(0),
      m_categoryDefinitionStore(new CategoryDefinitionStore(CATEGORY_DEFINITION_FILE_DIRECTORY,
                                                            MAX_CATEGORY_DEFINITION_FILES, this)),
      m_androidPriorityStore(new AndroidPriorityStore(ANDROID_PRIORITY_DEFINITION_PATH, this)),
      m_database(new QSqlDatabase),
      m_committed(true),
      m_nextExpirationTime(0)
{
    if (owner) {
        qDBusRegisterMetaType<QVariantHash>();
        qDBusRegisterMetaType<LipstickNotification>();
        qDBusRegisterMetaType<NotificationList>();

        new NotificationManagerAdaptor(this);
        QDBusConnection::sessionBus().registerObject("/org/freedesktop/Notifications", this);
        QDBusConnection::sessionBus().registerService("org.freedesktop.Notifications");

        connect(m_categoryDefinitionStore, SIGNAL(categoryDefinitionUninstalled(QString)),
                this, SLOT(removeNotificationsWithCategory(QString)));
        connect(m_categoryDefinitionStore, SIGNAL(categoryDefinitionModified(QString)),
                this, SLOT(updateNotificationsWithCategory(QString)));

        // Commit the modifications to the database 10 seconds after the last modification so that writing
        // to disk doesn't affect user experience
        m_databaseCommitTimer.setInterval(CommitDelay);
        m_databaseCommitTimer.setSingleShot(true);
        connect(&m_databaseCommitTimer, SIGNAL(timeout()), this, SLOT(commit()));

        m_expirationTimer.setSingleShot(true);
        connect(&m_expirationTimer, SIGNAL(timeout()), this, SLOT(expire()));

        m_modificationTimer.setInterval(PublicationDelay);
        m_modificationTimer.setSingleShot(true);
        connect(&m_modificationTimer, SIGNAL(timeout()), this, SLOT(reportModifications()));
    }

    restoreNotifications(owner);
}

NotificationManager::~NotificationManager()
{
    m_database->commit();
    QString connectionName = m_database->connectionName();
    delete m_database;
    QSqlDatabase::removeDatabase(connectionName);
}

LipstickNotification *NotificationManager::notification(uint id) const
{
    return m_notifications.value(id);
}

QList<uint> NotificationManager::notificationIds() const
{
    return m_notifications.keys();
}

QStringList NotificationManager::GetCapabilities()
{
    return QStringList() << "body"
                         << "actions"
                         << "persistence"
                         << "sound"
                         << LipstickNotification::HINT_ITEM_COUNT
                         << LipstickNotification::HINT_TIMESTAMP
                         << LipstickNotification::HINT_PREVIEW_BODY
                         << LipstickNotification::HINT_PREVIEW_SUMMARY
                         << "x-nemo-remote-actions"
                         << LipstickNotification::HINT_USER_REMOVABLE
                         << "x-nemo-get-notifications";
}

bool NotificationManager::isInternalOperation() const
{
    if (!calledFromDBus())
        return true;

    /* D-Bus method calls from self special case: we know that caller
     * is this process, and additionally the message might have been
     * dispatched internally without involving D-Bus daemon - in which
     * case we must provide an immediate synchronous reply, or an
     * autogenerated error reply is returned to the caller. */
    if (message().service() == connection().baseService())
        return true;

    return false;
}

uint NotificationManager::Notify(const QString &appName, uint replacesId, const QString &appIcon,
                                 const QString &summary, const QString &body, const QStringList &actions,
                                 const QVariantHash &hints, int expireTimeout)
{
    uint id = 0;
    if (isInternalOperation()) {
        id = handleNotify(getpid(), appName, replacesId, appIcon, summary, body, actions, hints, expireTimeout);
    } else {
        setDelayedReply(true);
        ClientIdentifier *identifier = new ClientIdentifier(this, connection(), message());
        connect(identifier, &ClientIdentifier::finished, this, &NotificationManager::identifiedNotify,
                Qt::QueuedConnection);
    }
    return id;
}

void NotificationManager::identifiedNotify()
{
    ClientIdentifier *identifier = qobject_cast<ClientIdentifier *>(sender());
    QVariantList arguments(identifier->message().arguments());
    QString appName = arguments.at(0).toString();
    uint replacesId = arguments.at(1).toUInt();
    QString appIcon = arguments.at(2).toString();
    QString summary = arguments.at(3).toString();
    QString body = arguments.at(4).toString();
    QStringList actions = arguments.at(5).toStringList();
    const QDBusArgument hintsArg(arguments.at(6).value<QDBusArgument>());
    QVariantHash hints;
    hintsArg >> hints;
    int expireTimeout = arguments.at(7).toInt();
    uint id = handleNotify(identifier->clientPid(), appName, replacesId, appIcon, summary, body,
                           actions, hints, expireTimeout);
    if (identifier->message().isReplyRequired()) {
        QDBusMessage reply;
        if (id == 0) {
            QString errorString = QString("PID %1 is not in privileged group").arg(identifier->clientPid());
            reply = identifier->message().createErrorReply(QDBusError::AccessDenied, errorString);
        } else {
            reply = identifier->message().createReply();
            reply << id;
        }
        identifier->connection().send(reply);
    }
    identifier->deleteLater();
}

uint NotificationManager::handleNotify(int clientPid, const QString &appName, uint replacesId, const QString &appIcon,
                                       const QString &summary, const QString &body, const QStringList &actions,
                                       const QVariantHash &hints, int expireTimeout)
{
    NOTIFICATIONS_DEBUG("clientPid:" << clientPid << "appName:" << appName << "replacesId:" << replacesId
                        << "appIcon:" << appIcon << "summary:" << summary << "body:" << body << "actions:"
                        << actions << "hints:" << hints << "expireTimeout:" << expireTimeout);
    if (replacesId != 0 && !m_notifications.contains(replacesId)) {
        replacesId = 0;
    }

    uint id = replacesId != 0 ? replacesId : nextAvailableNotificationID();

    QVariantHash hints_(hints);

    // Ensure the hints contain a timestamp, and convert to UTC if required
    QString timestamp(hints_.value(LipstickNotification::HINT_TIMESTAMP).toString());
    if (!timestamp.isEmpty()) {
        QDateTime tsValue(QDateTime::fromString(timestamp, Qt::ISODate));
        if (tsValue.isValid()) {
            if (tsValue.timeSpec() != Qt::UTC) {
                tsValue = tsValue.toUTC();
            }
            timestamp = tsValue.toString(Qt::ISODate);
        } else {
            timestamp = QString();
        }
    }
    if (timestamp.isEmpty()) {
        timestamp = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    }
    hints_.insert(LipstickNotification::HINT_TIMESTAMP, timestamp);

    auto it = hints_.find(LipstickNotification::HINT_IMAGE_DATA);
    if (it != hints_.end()) {
        const QDBusArgument argument = it->value<QDBusArgument>();

        hints_.erase(it);

        int width = 0;
        int height = 0;
        int stride = 0;
        bool alpha = false;
        int bitsPerSample = 0;
        int channels = 0;
        QByteArray data;

        argument.beginStructure();
        argument >> width;
        argument >> height;
        argument >> stride;
        argument >> alpha;
        argument >> bitsPerSample;
        argument >> channels;
        argument >> data;
        argument.endStructure();

        if (bitsPerSample == 8 && channels == 4 && data.size() >= stride * height) {
            const QImage image(
                        reinterpret_cast<const uchar *>(data.constData()),
                        width,
                        height,
                        stride,
                        alpha ? QImage::Format_ARGB32 : QImage::Format_RGB32);

            QBuffer buffer;
            buffer.open(QIODevice::WriteOnly);
            image.save(&buffer, "PNG");
            buffer.close();

            const QUrl url
                    = QLatin1String("data:image/png;base64,")
                    + QString::fromUtf8(buffer.data().toBase64());

            hints_.insert(LipstickNotification::HINT_IMAGE_PATH, url.toString());
        }
    }

    QPair<QString, QString> pidProperties;
    bool androidOrigin(false);
    if (clientPid > 0) {
        // Look up the properties of the originating process
        pidProperties = processProperties(clientPid);
        // Only Alien4 has special notification handling
        androidOrigin = pidProperties.first == QLatin1String("alien_bridge_server");
    }

    // Allow notifications originating from android to be differentiated from native app notifications
    QString disambiguatedAppName(appName);
    if (androidOrigin) {
        disambiguatedAppName.append("-android");
    }

    LipstickNotification notificationData(
                appName, appName, disambiguatedAppName, id, appIcon, summary, body, actions, hints_, expireTimeout);
    applyCategoryDefinition(&notificationData);
    hints_ = notificationData.hints();

    bool clientIsPrivileged = processIsPrivileged(clientPid);

    if (!notificationData.isUserRemovableByHint() && !clientIsPrivileged) {
        qWarning() << "Persistent notification from"
                   << qPrintable(pidProperties.first)
                   << "dropped because of insufficent permissions";
        return 0; // AccessDenied error reply will be sent if called from D-Bus
    }

    LipstickNotification *notification = replacesId != 0
            ? m_notifications.value(replacesId)
            : nullptr;

    if (notification) {
        if (!notification->isUserRemovableByHint() && !clientIsPrivileged) {
            qWarning() << "An alteration to a persistent notification by"
                       << qPrintable(pidProperties.first)
                       << "was ignored because of insufficent permissions";
            return 0; // AccessDenied error reply will be sent if called from D-Bus
        }

        notification->setAppName(notificationData.appName());
        notification->setExplicitAppName(notificationData.explicitAppName());
        notification->setDisambiguatedAppName(notificationData.disambiguatedAppName());
        notification->setSummary(notificationData.summary());
        notification->setBody(notificationData.body());
        notification->setActions(notificationData.actions());
        notification->setHints(notificationData.hints());
        notification->setExpireTimeout(notificationData.expireTimeout());
        notification->setRestored(false);
    } else {
        notification = new LipstickNotification(notificationData);
        notification->setParent(this);

        connect(notification, &LipstickNotification::actionInvoked,
                this, &NotificationManager::invokeAction, Qt::QueuedConnection);
        connect(notification, &LipstickNotification::removeRequested, // default prevents direction connection.
                this, [this]() { removeNotificationIfUserRemovable(); }, Qt::QueuedConnection);

        m_notifications.insert(id, notification);
    }

    notification->restartProgressTimer();

    const QString previewSummary(hints_.value(LipstickNotification::HINT_PREVIEW_SUMMARY).toString());
    const QString previewBody(hints_.value(LipstickNotification::HINT_PREVIEW_BODY).toString());

    if (androidOrigin) {
        // If this notification includes a preview, ensure it has a non-empty body and summary
        if (!previewSummary.isEmpty()) {
            if (previewBody.isEmpty()) {
                hints_.insert(LipstickNotification::HINT_PREVIEW_BODY, QStringLiteral(" "));
            }
        }
        if (!previewBody.isEmpty()) {
            if (previewSummary.isEmpty()) {
                hints_.insert(LipstickNotification::HINT_PREVIEW_SUMMARY, QStringLiteral(" "));
            }
        }

        // See if this notification has elevated priority and feedback
        AndroidPriorityStore::PriorityDetails priority;
        const QString packageName(hints_.value(LipstickNotification::HINT_ORIGIN_PACKAGE).toString());
        if (!packageName.isEmpty()) {
            priority = m_androidPriorityStore->packageDetails(packageName);
        } else {
            priority = m_androidPriorityStore->appDetails(appName);
        }
        hints_.insert(LipstickNotification::HINT_PRIORITY, priority.first);

        if (!priority.second.isEmpty()) {
            hints_.insert(LipstickNotification::HINT_FEEDBACK, priority.second);
            // Also turn the display on if required, but only if it's really playing vibra or sound feedback
            if (hints_.value(LipstickNotification::HINT_VIBRA, false).toBool()
                    || !hints_.value(LipstickNotification::HINT_SUPPRESS_SOUND, false).toBool()) {
                hints_.insert(LipstickNotification::HINT_DISPLAY_ON, true);
            }
        }
    } else {
        if (notification->appName().isEmpty() && !pidProperties.first.isEmpty()) {
            notification->setAppName(pidProperties.first);
        }
        if (notification->appIcon().isEmpty() && !pidProperties.second.isEmpty()) {
            notification->setAppIcon(pidProperties.second, LipstickNotification::InferredValue);
        }

        // Use the summary and body as fallback values for previewSummary and previewBody.
        // (This can be avoided by setting them explicitly to empty strings.)
        if (!hints_.contains(LipstickNotification::HINT_PREVIEW_SUMMARY)) {
            hints_.insert(LipstickNotification::HINT_PREVIEW_SUMMARY, summary);
        }
        if (!hints_.contains(LipstickNotification::HINT_PREVIEW_BODY)) {
            hints_.insert(LipstickNotification::HINT_PREVIEW_BODY, body);
        }

        // Unspecified priority should result in medium priority to permit low priorities
        if (!hints_.contains(LipstickNotification::HINT_PRIORITY)) {
            hints_.insert(LipstickNotification::HINT_PRIORITY, DefaultNotificationPriority);
        }
    }

    notification->setHints(hints_);
    notification->setPrivilegedSource(clientIsPrivileged);

    publish(notification, replacesId);

    return id;
}

void NotificationManager::deleteNotification(uint id)
{
    // Remove the notification, its actions and its hints from database
    const QVariantList params(QVariantList() << id);
    execSQL(QString("DELETE FROM notifications WHERE id=?"), params);
    execSQL(QString("DELETE FROM actions WHERE id=?"), params);
    execSQL(QString("DELETE FROM hints WHERE id=?"), params);
    execSQL(QString("DELETE FROM internal_hints WHERE id=?"), params);
    execSQL(QString("DELETE FROM expiration WHERE id=?"), params);
}

void NotificationManager::CloseNotification(uint id, NotificationClosedReason closeReason)
{
    if (isInternalOperation()) {
        handleCloseNotification(getpid(), id, closeReason);
    } else {
        setDelayedReply(true);
        ClientIdentifier *identifier = new ClientIdentifier(this, connection(), message());
        connect(identifier, &ClientIdentifier::finished,
                this, &NotificationManager::identifiedCloseNotification, Qt::QueuedConnection);
    }
}

void NotificationManager::identifiedCloseNotification()
{
    ClientIdentifier *identifier = qobject_cast<ClientIdentifier *>(sender());
    QVariantList arguments(identifier->message().arguments());
    uint id = arguments.at(0).toUInt();
    // Note: apply closeReason that is/was implicitly provided to CloseNotification()
    //       C++ method but is not present in D-Bus method call message arguments
    NotificationClosedReason closeReason = CloseNotificationCalled;
    handleCloseNotification(identifier->clientPid(), id, closeReason);
    if (identifier->message().isReplyRequired()) {
        QDBusMessage reply = identifier->message().createReply();
        identifier->connection().send(reply);
    }
    identifier->deleteLater();
}

void NotificationManager::handleCloseNotification(int clientPid, uint id, NotificationClosedReason closeReason)
{
    NOTIFICATIONS_DEBUG("clientPid:" << clientPid << "id:" << id << "closeReason:" << closeReason);
    if (LipstickNotification *notification = m_notifications.value(id)) {
        if (!notification->isUserRemovableByHint() && !processIsPrivileged(clientPid)) {
            qWarning() << "An application was not allowed to close a notification due to insufficient permissions";
            return;
        }

        emit NotificationClosed(id, closeReason);

        deleteNotification(id);

        NOTIFICATIONS_DEBUG("REMOVE:" << id);
        emit notificationRemoved(id);

        // Mark the notification to be destroyed
        m_removedNotifications.insert(m_notifications.take(id));
    }
}

void NotificationManager::closeNotifications(const QList<uint> &ids, NotificationClosedReason closeReason)
{
    QSet<uint> uniqueIds = QSet<uint>::fromList(ids);
    QList<uint> removedIds;

    foreach (uint id, uniqueIds) {
        if (m_notifications.contains(id)) {
            removedIds.append(id);
            emit NotificationClosed(id, closeReason);

            deleteNotification(id);
        }
    }

    if (!removedIds.isEmpty()) {
        NOTIFICATIONS_DEBUG("REMOVE:" << removedIds);
        emit notificationsRemoved(removedIds);

        foreach (uint id, removedIds) {
            emit notificationRemoved(id);

            // Mark the notification to be destroyed
            m_removedNotifications.insert(m_notifications.take(id));
        }
    }
}

void NotificationManager::markNotificationDisplayed(uint id)
{
    if (m_notifications.contains(id)) {
        const LipstickNotification *notification = m_notifications.value(id);
        if (notification->hints().value(LipstickNotification::HINT_TRANSIENT).toBool()) {
            // Remove this notification immediately
            CloseNotification(id, NotificationExpired);
            NOTIFICATIONS_DEBUG("REMOVED transient:" << id);
        } else {
            const int timeout(notification->expireTimeout());
            if (timeout > 0) {
                // Insert the timeout into the expiration table, or leave the existing value if already present
                const qint64 currentTime(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
                const qint64 expireAt(currentTime + timeout);
                execSQL(QString("INSERT OR IGNORE INTO expiration(id, expire_at) VALUES(?, ?)"),
                        QVariantList() << id << expireAt);

                if (m_nextExpirationTime == 0 || (expireAt < m_nextExpirationTime)) {
                    // This will be the next notification to expire - update the timer
                    m_nextExpirationTime = expireAt;
                    m_expirationTimer.start(timeout);
                }

                NOTIFICATIONS_DEBUG("DISPLAYED:" << id << "expiring in:" << timeout);
            }
        }
    }
}

QString NotificationManager::GetServerInformation(QString &vendor, QString &version, QString &spec_version)
{
    QString name = qApp->applicationName();
    AboutSettings settings;
    vendor = settings.operatingSystemName();
    version = qApp->applicationVersion();
    spec_version = "1.2";
    return name;
}

NotificationList NotificationManager::GetNotifications(const QString &owner)
{
    NotificationList notificationList;
    if (isInternalOperation()) {
        notificationList = handleGetNotifications(getpid(), owner);
    } else {
        setDelayedReply(true);
        ClientIdentifier *identifier = new ClientIdentifier(this, connection(), message());
        connect(identifier, &ClientIdentifier::finished,
                this, &NotificationManager::identifiedGetNotifications, Qt::QueuedConnection);
    }
    return notificationList;
}

void NotificationManager::identifiedGetNotifications()
{
    ClientIdentifier *identifier = qobject_cast<ClientIdentifier *>(sender());
    QVariantList arguments(identifier->message().arguments());
    const QString owner = arguments.at(0).toString();
    NotificationList notificationList = handleGetNotifications(identifier->clientPid(), owner);
    if (identifier->message().isReplyRequired()) {
        QDBusMessage reply = identifier->message().createReply();
        reply << QVariant::fromValue(notificationList);
        identifier->connection().send(reply);
    }
    identifier->deleteLater();
}

NotificationList NotificationManager::handleGetNotifications(int clientPid, const QString &owner)
{
    NOTIFICATIONS_DEBUG("clientPid:" << clientPid << "owner:" << owner);
    QString callerProcessName = getProcessName(clientPid);
    QList<LipstickNotification *> notificationList;
    QHash<uint, LipstickNotification *>::const_iterator it = m_notifications.constBegin(), end = m_notifications.constEnd();
    for ( ; it != end; ++it) {
        LipstickNotification *notification = it.value();
        if (notification->owner() == owner
                || (!callerProcessName.isEmpty() && (notification->owner() == callerProcessName))) {
            notificationList.append(notification);
        }
    }

    return NotificationList(notificationList);
}

NotificationList NotificationManager::GetNotificationsByCategory(const QString &category)
{
    NotificationList notificationList;
    if (isInternalOperation()) {
        notificationList = handleGetNotificationsByCategory(getpid(), category);
    } else {
        setDelayedReply(true);
        ClientIdentifier *identifier = new ClientIdentifier(this, connection(), message());
        connect(identifier, &ClientIdentifier::finished,
                this, &NotificationManager::identifiedGetNotificationsByCategory, Qt::QueuedConnection);
    }
    return notificationList;
}

void NotificationManager::identifiedGetNotificationsByCategory()
{
    ClientIdentifier *identifier = qobject_cast<ClientIdentifier *>(sender());
    QVariantList arguments(identifier->message().arguments());
    const QString category = arguments.at(0).toString();
    NotificationList notificationList = handleGetNotificationsByCategory(identifier->clientPid(), category);
    if (identifier->message().isReplyRequired()) {
        QDBusMessage reply = identifier->message().createReply();
        reply << QVariant::fromValue(notificationList);
        identifier->connection().send(reply);
    }
    identifier->deleteLater();
}

NotificationList NotificationManager::handleGetNotificationsByCategory(int clientPid, const QString &category)
{
    NOTIFICATIONS_DEBUG("clientPid:" << clientPid << "category:" << category);
    QList<LipstickNotification *> notificationList;
    if (processIsPrivileged(clientPid)) {
        QHash<uint, LipstickNotification *>::const_iterator it = m_notifications.constBegin(),
                end = m_notifications.constEnd();
        for ( ; it != end; ++it) {
            LipstickNotification *notification = it.value();
            if (notification->category() == category) {
                notificationList.append(notification);
            }
        }
    }
    return NotificationList(notificationList);
}

QString NotificationManager::systemApplicationName() const
{
    //% "System"
    return qtTrId("qtn_ap_lipstick");
}

uint NotificationManager::nextAvailableNotificationID()
{
    /* Find an unused ID. It is assumed that we will never end up
     * in a situation where even close to significant portion of
     * all possible ID numbers would be in use. If that ever happens
     * this will turn into forever loop.
     */
    for (;;) {
        uint id = ++m_previousNotificationID;
        // 0 is not a valid ID so skip it
        if (!id)
            continue;
        if (!m_notifications.contains(id))
            return id;
    }
}

void NotificationManager::removeNotificationsWithCategory(const QString &category)
{
    QList<uint> ids;
    QHash<uint, LipstickNotification *>::const_iterator it = m_notifications.constBegin(), end = m_notifications.constEnd();
    for ( ; it != end; ++it) {
        LipstickNotification *notification(it.value());
        if (notification->category() == category) {
            ids.append(it.key());
        }
    }
    closeNotifications(ids);
}

void NotificationManager::updateNotificationsWithCategory(const QString &category)
{
    QList<LipstickNotification *> categoryNotifications;

    QHash<uint, LipstickNotification *>::const_iterator it = m_notifications.constBegin(), end = m_notifications.constEnd();
    for ( ; it != end; ++it) {
        LipstickNotification *notification(it.value());
        if (notification->category() == category) {
            categoryNotifications.append(notification);
        }
    }

    foreach (LipstickNotification *notification, categoryNotifications) {
        // Mark the notification as restored to avoid showing the preview banner again
        notification->setRestored(true);

        // Update the category properties and re-publish
        applyCategoryDefinition(notification);
        publish(notification, notification->id());
    }
}

QHash<QString, QString> NotificationManager::categoryDefinitionParameters(const QVariantHash &hints) const
{
    return m_categoryDefinitionStore->categoryParameters(hints.value(LipstickNotification::HINT_CATEGORY).toString());
}

void NotificationManager::applyCategoryDefinition(LipstickNotification *notification) const
{
    QVariantHash hints = notification->hints();

    // Apply a category definition, if any
    const QHash<QString, QString> categoryParameters(categoryDefinitionParameters(hints));
    QHash<QString, QString>::const_iterator it = categoryParameters.constBegin(), end = categoryParameters.constEnd();
    for ( ; it != end; ++it) {
        const QString &key(it.key());
        const QString &value(it.value());

        // TODO: this is wrong - in some cases we need to overwrite any existing value...
        // What would get broken by doing this?
        if (key == QString("appName")) {
            if (notification->appName().isEmpty()) {
                notification->setAppName(value);
            }
        } else if (key == QString("app_icon")) {
            if (notification->appIcon().isEmpty()) {
                notification->setAppIcon(value, LipstickNotification::CategoryValue);
            }
        } else if (key == QString("summary")) {
            if (notification->summary().isEmpty()) {
                notification->setSummary(value);
            }
        } else if (key == QString("body")) {
            if (notification->body().isEmpty()) {
                notification->setBody(value);
            }
        } else if (key == QString("expireTimeout")) {
            if (notification->expireTimeout() == -1) {
                notification->setExpireTimeout(value.toInt());
            }
        } else if (!hints.contains(key)) {
            hints.insert(key, value);
        }
    }

    notification->setHints(hints);
}

void NotificationManager::publish(const LipstickNotification *notification, uint replacesId)
{
    const uint id(notification->id());
    if (id == 0) {
        qWarning() << "Cannot publish notification without ID!";
        return;
    } else if (replacesId != 0 && replacesId != id) {
        qWarning() << "Cannot publish notification replacing independent ID!";
        return;
    }

    if (replacesId != 0) {
        // Delete the existing notification from the database
        deleteNotification(id);
    }

    // Add the notification, its actions and its hints to the database
    execSQL("INSERT INTO notifications VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
            QVariantList() << id << notification->appName() << notification->appIcon() << notification->summary()
            << notification->body() << notification->expireTimeout() << notification->disambiguatedAppName()
            << notification->explicitAppName() << notification->appIconOrigin());

    // every other is identifier and every other the localized name for it
    bool everySecond = false;
    QString action;
    foreach (const QString &actionItem, notification->actions()) {
        if (everySecond) {
            if (!action.isEmpty()) {
                execSQL("INSERT INTO actions VALUES (?, ?, ?)", QVariantList() << id << action << actionItem);
            }
        } else {
            action = actionItem;
        }
        everySecond = !everySecond;
    }

    const QVariantHash hints(notification->hints());
    QVariantHash::const_iterator hit = hints.constBegin(), hend = hints.constEnd();
    for ( ; hit != hend; ++hit) {
        execSQL("INSERT INTO hints VALUES (?, ?, ?)", QVariantList() << id << hit.key() << hit.value());
    }

    const QVariantHash internalHints(notification->internalHints());
    hit = internalHints.constBegin(), hend = internalHints.constEnd();
    for ( ; hit != hend; ++hit) {
        execSQL("INSERT INTO internal_hints VALUES (?, ?, ?)", QVariantList() << id << hit.key() << hit.value());
    }

    NOTIFICATIONS_DEBUG("PUBLISH:" << notification->appName() << notification->appIcon() << notification->summary()
                        << notification->body() << notification->actions() << notification->hints()
                        << notification->expireTimeout() << "->" << id);
    m_modifiedIds.insert(id);
    if (!m_modificationTimer.isActive()) {
        m_modificationTimer.start();
    }
    if (replacesId == 0) {
        emit notificationAdded(id);
    } else {
        emit notificationModified(id);
    }
}

void NotificationManager::restoreNotifications(bool update)
{
    if (connectToDatabase()) {
        if (checkTableValidity()) {
            fetchData(update);
        } else {
            m_database->close();
        }
    }
}

bool NotificationManager::connectToDatabase()
{
    QString databasePath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
            + QStringLiteral("/system/privileged/Notifications");
    if (!QDir::root().exists(databasePath)) {
        QDir::root().mkpath(databasePath);
    }
    QString databaseName = databasePath + "/notifications.db";

    *m_database = QSqlDatabase::addDatabase("QSQLITE", metaObject()->className());
    m_database->setDatabaseName(databaseName);
    bool success = checkForDiskSpace(databasePath, MINIMUM_FREE_SPACE_NEEDED_IN_KB);
    if (success) {
        success = m_database->open();
        if (!success) {
            NOTIFICATIONS_DEBUG(m_database->lastError().driverText() << databaseName << m_database->lastError().databaseText());

            // If opening the database fails, try to recreate the database
            removeDatabaseFile(databaseName);
            success = m_database->open();
            NOTIFICATIONS_DEBUG("Unable to open database file. Recreating. Success: " << success);
        }
    } else {
        NOTIFICATIONS_DEBUG("Not enough free disk space available. Unable to open database.");
    }

    if (success) {
        // Set up the database mode to write-ahead locking to improve performance
        QSqlQuery(*m_database).exec("PRAGMA journal_mode=WAL");
    }

    return success;
}

bool NotificationManager::checkForDiskSpace(const QString &path, unsigned long freeSpaceNeeded)
{
    struct statfs st;
    bool spaceAvailable = false;
    if (statfs(path.toUtf8().data(), &st) != -1) {
        unsigned long freeSpaceInKb = (st.f_bsize * st.f_bavail) / 1024;
        if (freeSpaceInKb > freeSpaceNeeded) {
            spaceAvailable = true;
        }
    }
    return spaceAvailable;
}

void NotificationManager::removeDatabaseFile(const QString &path)
{
    // Remove also -shm and -wal files created when journal-mode=WAL is being used
    QDir::root().remove(path + "-shm");
    QDir::root().remove(path + "-wal");
    QDir::root().remove(path);
}

bool NotificationManager::checkTableValidity()
{
    bool result = true;
    bool recreateNotificationsTable = false;
    bool recreateActionsTable = false;
    bool recreateHintsTable = false;
    bool recreateInternalHintsTable = false;
    bool recreateExpirationTable = false;

    const int databaseVersion(schemaVersion());

    if (databaseVersion < 3) {
        // All databases this old should have been migrated already.
        qWarning() << "Removing obsolete notifications";
        recreateNotificationsTable = true;
        recreateActionsTable = true;
        recreateHintsTable = true;
        recreateInternalHintsTable = true;
        recreateExpirationTable = true;
    } else {
        if (databaseVersion == 3) {
            QSqlQuery query(*m_database);
            if (query.exec("ALTER TABLE notifications ADD COLUMN explicit_app_name TEXT")
                    && query.exec("ALTER TABLE notifications ADD COLUMN app_icon_origin INTEGER")) {
                qWarning() << "Extended notifications table";
            } else {
                qWarning() << "Failed to extend notifications table!" << query.lastError();
                recreateNotificationsTable = true;
            }

        } else {
            recreateNotificationsTable = !verifyTableColumns("notifications",
                                                             QStringList() << "id" << "app_name" << "app_icon" << "summary"
                                                             << "body" << "expire_timeout" << "disambiguated_app_name"
                                                             << "explicit_app_name" << "app_icon_origin");
            recreateActionsTable = !verifyTableColumns("actions", QStringList() << "id" << "action" << "display_name");
        }

        recreateHintsTable = !verifyTableColumns("hints", QStringList() << "id" << "hint" << "value");
        recreateInternalHintsTable = !verifyTableColumns("internal_hints", QStringList() << "id" << "hint" << "value");
        recreateExpirationTable = !verifyTableColumns("expiration", QStringList() << "id" << "expire_at");
    }

    if (recreateNotificationsTable) {
        qWarning() << "Recreating notifications table";
        result &= recreateTable("notifications", "id INTEGER PRIMARY KEY, app_name TEXT, app_icon TEXT, summary TEXT, "
                                                 "body TEXT, expire_timeout INTEGER, disambiguated_app_name TEXT, "
                                                 "explicit_app_name TEXT, app_icon_origin INTEGER");
    }
    if (recreateActionsTable) {
        qWarning() << "Recreating actions table";
        result &= recreateTable("actions", "id INTEGER, action TEXT, display_name TEXT, PRIMARY KEY(id, action)");
    }
    if (recreateHintsTable) {
        qWarning() << "Recreating hints table";
        result &= recreateTable("hints", "id INTEGER, hint TEXT, value TEXT, PRIMARY KEY(id, hint)");
    }
    if (recreateInternalHintsTable) {
        qWarning() << "Recreating internal hints table";
        result &= recreateTable("internal_hints", "id INTEGER, hint TEXT, value TEXT, PRIMARY KEY(id, hint)");
    }
    if (recreateExpirationTable) {
        qWarning() << "Recreating expiration table";
        result &= recreateTable("expiration", "id INTEGER PRIMARY KEY, expire_at INTEGER");
    }

    if (result && databaseVersion != 4) {
        if (!setSchemaVersion(4)) {
            qWarning() << "Unable to set database schema version!";
        }
    }
    return result;
}

int NotificationManager::schemaVersion()
{
    int result = -1;

    if (m_database->isOpen()) {
        QSqlQuery query(*m_database);
        if (query.exec("PRAGMA user_version") && query.next()) {
            result = query.value(0).toInt();
        }
    }

    return result;
}

bool NotificationManager::setSchemaVersion(int version)
{
    bool result = false;

    if (m_database->isOpen()) {
        QSqlQuery query(*m_database);
        if (query.exec(QString::fromLatin1("PRAGMA user_version=%1").arg(version))) {
            result = true;
        }
    }

    return result;
}

bool NotificationManager::verifyTableColumns(const QString &tableName, const QStringList &columnNames)
{
    QSqlTableModel tableModel(0, *m_database);
    tableModel.setTable(tableName);

    // The order of the columns must be correct
    int index = 0;
    foreach (const QString &columnName, columnNames) {
        if (tableModel.fieldIndex(columnName) != index)
            return false;
        ++index;
    }

    return true;
}

bool NotificationManager::recreateTable(const QString &tableName, const QString &definition)
{
    bool result = false;

    if (m_database->isOpen()) {
        QSqlQuery(*m_database).exec("DROP TABLE " + tableName);
        result = QSqlQuery(*m_database).exec("CREATE TABLE " + tableName + " (" + definition + ")");
    }

    return result;
}

void NotificationManager::fetchData(bool update)
{
    // Gather actions for each notification
    QSqlQuery actionsQuery("SELECT * FROM actions", *m_database);
    QSqlRecord actionsRecord = actionsQuery.record();
    int actionsTableIdIndex = actionsRecord.indexOf("id");
    int actionsTableActionIndex = actionsRecord.indexOf("action");
    int actionsTableNameIndex = actionsRecord.indexOf("display_name");

    QHash<uint, QStringList> actions;
    while (actionsQuery.next()) {
        const uint id = actionsQuery.value(actionsTableIdIndex).toUInt();
        actions[id].append(actionsQuery.value(actionsTableActionIndex).toString());
        actions[id].append(actionsQuery.value(actionsTableNameIndex).toString());
    }

    // Gather hints for each notification
    QSqlQuery hintsQuery("SELECT * FROM hints", *m_database);
    QSqlRecord hintsRecord = hintsQuery.record();
    int hintsTableIdIndex = hintsRecord.indexOf("id");
    int hintsTableHintIndex = hintsRecord.indexOf("hint");
    int hintsTableValueIndex = hintsRecord.indexOf("value");
    QHash<uint, QVariantHash> hints;

    while (hintsQuery.next()) {
        const uint id = hintsQuery.value(hintsTableIdIndex).toUInt();
        const QString hintName(hintsQuery.value(hintsTableHintIndex).toString());
        const QVariant hintValue(hintsQuery.value(hintsTableValueIndex));

        QVariant value;
        if (hintName == LipstickNotification::HINT_TIMESTAMP) {
            // Timestamps in the DB are already UTC but not marked as such, so they will
            // be converted again unless specified to be UTC
            QDateTime timestamp(QDateTime::fromString(hintValue.toString(), Qt::ISODate));
            timestamp.setTimeSpec(Qt::UTC);
            value = timestamp.toString(Qt::ISODate);
        } else {
            value = hintValue;
        }
        hints[id].insert(hintName, value);
    }

    // Gather the internal hints
    QSqlQuery internalHintsQuery("SELECT * FROM internal_hints", *m_database);
    QSqlRecord internalHintsRecord = internalHintsQuery.record();
    int internalHintsTableIdIndex = internalHintsRecord.indexOf("id");
    int internalHintsTableHintIndex = internalHintsRecord.indexOf("hint");
    int internalHintsTableValueIndex = internalHintsRecord.indexOf("value");
    QHash<uint, QVariantHash> internalHints;

    while (internalHintsQuery.next()) {
        const uint id = internalHintsQuery.value(internalHintsTableIdIndex).toUInt();
        const QString hintName(internalHintsQuery.value(internalHintsTableHintIndex).toString());
        const QVariant hintValue(internalHintsQuery.value(internalHintsTableValueIndex));

        internalHints[id].insert(hintName, hintValue);
    }

    // Gather expiration times for displayed notifications
    QSqlQuery expirationQuery("SELECT * FROM expiration", *m_database);
    QSqlRecord expirationRecord = expirationQuery.record();
    int expirationTableIdIndex = expirationRecord.indexOf("id");
    int expirationTableExpireAtIndex = expirationRecord.indexOf("expire_at");
    QHash<uint, qint64> expireAt;
    while (expirationQuery.next()) {
        const uint id = expirationQuery.value(expirationTableIdIndex).toUInt();
        expireAt.insert(id, expirationQuery.value(expirationTableExpireAtIndex).value<qint64>());
    }

    const qint64 currentTime(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    QList<LipstickNotification *> activeNotifications;
    QList<uint> transientIds;
    QList<uint> expiredIds;
    qint64 nextTimeout = std::numeric_limits<qint64>::max();
    bool unexpiredRemaining = false;

    // Create the notifications
    QSqlQuery notificationsQuery("SELECT * FROM notifications", *m_database);
    QSqlRecord notificationsRecord = notificationsQuery.record();
    int notificationsTableIdIndex = notificationsRecord.indexOf("id");
    int notificationsTableAppNameIndex = notificationsRecord.indexOf("app_name");
    int notificationsTableExplicitAppNameIndex = notificationsRecord.indexOf("explicit_app_name");
    int notificationsTableDisambiguatedAppNameIndex = notificationsRecord.indexOf("disambiguated_app_name");
    int notificationsTableAppIconIndex = notificationsRecord.indexOf("app_icon");
    int notificationsTableAppIconOriginIndex = notificationsRecord.indexOf("app_icon_origin");
    int notificationsTableSummaryIndex = notificationsRecord.indexOf("summary");
    int notificationsTableBodyIndex = notificationsRecord.indexOf("body");
    int notificationsTableExpireTimeoutIndex = notificationsRecord.indexOf("expire_timeout");

    while (notificationsQuery.next()) {
        const uint id = notificationsQuery.value(notificationsTableIdIndex).toUInt();
        QString appName = notificationsQuery.value(notificationsTableAppNameIndex).toString();
        QString explicitAppName = notificationsQuery.value(notificationsTableExplicitAppNameIndex).toString();
        QString disambiguatedAppName = notificationsQuery.value(notificationsTableDisambiguatedAppNameIndex).toString();
        QString appIcon = notificationsQuery.value(notificationsTableAppIconIndex).toString();
        int appIconOrigin = notificationsQuery.value(notificationsTableAppIconOriginIndex).toInt();
        QString summary = notificationsQuery.value(notificationsTableSummaryIndex).toString();
        QString body = notificationsQuery.value(notificationsTableBodyIndex).toString();
        int expireTimeout = notificationsQuery.value(notificationsTableExpireTimeoutIndex).toInt();

        const QStringList &notificationActions = actions[id];

        QVariantHash &notificationHints = hints[id];
        if (notificationHints.value(LipstickNotification::HINT_TRANSIENT).toBool()) {
            // This notification was transient, it should not be restored
            NOTIFICATIONS_DEBUG("TRANSIENT AT RESTORE:" << appName << appIcon << summary << body << notificationActions
                                << notificationHints << expireTimeout << "->" << id);
            transientIds.append(id);
            continue;
        }

        bool expired = false;
        if (update && expireAt.contains(id)) {
            const qint64 expiry(expireAt.value(id));
            if (expiry <= currentTime) {
                expired = true;
            } else {
                nextTimeout = qMin(expiry, nextTimeout);
                unexpiredRemaining = true;
            }
        }

        LipstickNotification *notification = new LipstickNotification(appName, explicitAppName, disambiguatedAppName,
                                                                      id, QString(), summary, body, notificationActions,
                                                                      notificationHints, expireTimeout, this);
        notification->setAppIcon(appIcon, appIconOrigin);
        notification->setInternalHints(internalHints[id]);
        notification->setRestored(true);
        m_notifications.insert(id, notification);

        if (id > m_previousNotificationID) {
            // Use the highest notification ID found as the previous notification ID
            m_previousNotificationID = id;
        }

        if (!expired) {
            activeNotifications.append(notification);
        } else {
            NOTIFICATIONS_DEBUG("EXPIRED AT RESTORE:" << appName << appIcon << summary << body << notificationActions
                                << notificationHints << expireTimeout << "->" << id);
            expiredIds.append(id);
        }
    }

    if (update) {
        // Remove notifications no longer required
        foreach (uint id, transientIds) {
            deleteNotification(id);
        }
    }

    int cullCount(activeNotifications.count() - MaxNotificationRestoreCount);
    if (update && cullCount > 0) {
        // Cull the least relevant notifications from this set
        std::sort(activeNotifications.begin(), activeNotifications.end(), notificationReverseOrder);

        foreach (LipstickNotification *n, activeNotifications) {
            const QVariant userRemovable = n->hints().value(LipstickNotification::HINT_USER_REMOVABLE);
            if (!userRemovable.isValid() || userRemovable.toBool()) {
                const uint id = n->id();
                NOTIFICATIONS_DEBUG("CULLED AT RESTORE:" << n->appName() << n->appIcon() << n->summary() << n->body()
                                    << actions[id] << hints[id] << n->expireTimeout() << "->" << id);
                expiredIds.append(id);

                if (--cullCount == 0) {
                    break;
                }
            }
        }
    }

    if (update) {
        closeNotifications(expiredIds, NotificationExpired);

        m_nextExpirationTime = unexpiredRemaining ? nextTimeout : 0;
        if (m_nextExpirationTime) {
            const qint64 nextTriggerInterval(m_nextExpirationTime - currentTime);
            m_expirationTimer.start(static_cast<int>(std::min<qint64>(nextTriggerInterval, std::numeric_limits<int>::max())));
        }
    }

    foreach (LipstickNotification *n, m_notifications) {
        connect(n, &LipstickNotification::actionInvoked,
                this, &NotificationManager::invokeAction, Qt::QueuedConnection);
        connect(n, SIGNAL(removeRequested()), this, SLOT(removeNotificationIfUserRemovable()), Qt::QueuedConnection);
#ifdef DEBUG_NOTIFICATIONS
        const uint id = n->id();
        NOTIFICATIONS_DEBUG("RESTORED:" << n->appName() << n->appIcon() << n->summary() << n->body() << actions[id]
                            << hints[id] << n->expireTimeout() << "->" << id);
#endif
    }

    if (update) {
        qWarning() << "Notifications restored:" << m_notifications.count();
    }
}

void NotificationManager::commit()
{
    // Any aditional rules about when database commits are allowed can be added here
    if (!m_committed) {
        m_database->commit();
        m_committed = true;
    }

    qDeleteAll(m_removedNotifications);
    m_removedNotifications.clear();
}

void NotificationManager::execSQL(const QString &command, const QVariantList &args)
{
    if (!m_database->isOpen()) {
        return;
    }

    if (m_committed) {
        m_committed = false;
        m_database->transaction();
    }

    QSqlQuery query(*m_database);
    query.prepare(command);

    foreach(const QVariant &arg, args) {
        query.addBindValue(arg);
    }

    query.exec();

    if (query.lastError().isValid()) {
        NOTIFICATIONS_DEBUG(command << args << query.lastError());
    }

    m_databaseCommitTimer.start();
}

void NotificationManager::invokeAction(const QString &action, const QString &actionText)
{
    LipstickNotification *notification = qobject_cast<LipstickNotification *>(sender());
    if (notification != 0) {
        uint id = m_notifications.key(notification, 0);
        if (id > 0) {
            QString actionHint = LipstickNotification::HINT_REMOTE_ACTION_PREFIX + action;
            QString remoteAction = notification->hints().value(actionHint).toString();

            if (!remoteAction.isEmpty()) {
                NOTIFICATIONS_DEBUG("INVOKE REMOTE ACTION:" << action << id);

                // If we need to support text actions with empty parameter, we need to fetch the action type from
                // the notification, but likely this is enough for the action type
                if (actionText.isEmpty()) {
                    emit remoteActionActivated(remoteAction, notification->privilegedSource());
                } else {
                    emit remoteTextActionActivated(remoteAction, actionText, notification->privilegedSource());
                }
            }

            for (int actionIndex = 0; actionIndex < notification->actions().count() / 2; actionIndex++) {
                // Actions are sent over as a list of pairs. Each even element in the list (starting at index 0) represents
                // the identifier for the action. Each odd element in the list is the localized string that will
                // be displayed to the user.
                if (notification->actions().at(actionIndex * 2) == action) {
                    NOTIFICATIONS_DEBUG("INVOKE ACTION:" << action << id);

                    emit ActionInvoked(id, action);
                }
            }

            // Unless marked as resident, we should remove the notification now.
            // progress notifications expected to update real soon so there is no point of automatically removing.
            const QVariant resident(notification->hints().value(LipstickNotification::HINT_RESIDENT));
            if ((resident.isValid() && resident.toBool() == false)
                    || (!resident.isValid() && !notification->hasProgress())) {
                removeNotificationIfUserRemovable(id);
            }
        }
    }
}

void NotificationManager::removeNotificationIfUserRemovable(uint id)
{
    if (id == 0) {
        LipstickNotification *notification = qobject_cast<LipstickNotification *>(sender());
        if (notification != 0) {
            id = m_notifications.key(notification, 0);
        }
    }

    LipstickNotification *notification = m_notifications.value(id);
    if (!notification) {
        return;
    }

    QVariant userRemovable = notification->hints().value(LipstickNotification::HINT_USER_REMOVABLE);
    if (!userRemovable.isValid() || userRemovable.toBool()) {
        // The notification should be removed if user removability is not defined (defaults to true) or is set to true
        CloseNotification(id, NotificationDismissedByUser);
    }
}

void NotificationManager::expire()
{
    const qint64 currentTime(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    QList<uint> expiredIds;
    qint64 nextTimeout = std::numeric_limits<qint64>::max();
    bool unexpiredRemaining = false;

    QSqlQuery expirationQuery("SELECT * FROM expiration", *m_database);
    QSqlRecord expirationRecord = expirationQuery.record();
    int expirationTableIdIndex = expirationRecord.indexOf("id");
    int expirationTableExpireAtIndex = expirationRecord.indexOf("expire_at");
    while (expirationQuery.next()) {
        const uint id = expirationQuery.value(expirationTableIdIndex).toUInt();
        const qint64 expiry = expirationQuery.value(expirationTableExpireAtIndex).value<qint64>();

        if (expiry <= currentTime) {
            expiredIds.append(id);
        } else {
            nextTimeout = qMin(expiry, nextTimeout);
            unexpiredRemaining = true;
        }
    }

    closeNotifications(expiredIds, NotificationExpired);

    m_nextExpirationTime = unexpiredRemaining ? nextTimeout : 0;
    if (m_nextExpirationTime) {
        const qint64 nextTriggerInterval(m_nextExpirationTime - currentTime);
        m_expirationTimer.start(static_cast<int>(std::min<qint64>(nextTriggerInterval, std::numeric_limits<int>::max())));
    }
}

void NotificationManager::reportModifications()
{
    if (!m_modifiedIds.isEmpty()) {
        emit notificationsModified(m_modifiedIds.toList());
        m_modifiedIds.clear();
    }
}

void NotificationManager::removeUserRemovableNotifications()
{
    QList<uint> closableNotifications;

    // Find any closable notifications we can close as a batch
    QHash<uint, LipstickNotification *>::const_iterator it = m_notifications.constBegin(), end = m_notifications.constEnd();
    for ( ; it != end; ++it) {
        LipstickNotification *notification(it.value());
        QVariant userRemovable = notification->hints().value(LipstickNotification::HINT_USER_REMOVABLE);
        if (!userRemovable.isValid() || userRemovable.toBool()) {
            closableNotifications.append(it.key());
        }
    }

    closeNotifications(closableNotifications, NotificationDismissedByUser);

    // Remove any remaining notifications
    foreach(uint id, m_notifications.keys()) {
        removeNotificationIfUserRemovable(id);
    }
}
