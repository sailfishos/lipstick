/***************************************************************************
**
** Copyright (c) 2012 - 2019 Jolla Ltd.
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

#ifndef LIPSTICKNOTIFICATION_H
#define LIPSTICKNOTIFICATION_H

#include "lipstickglobal.h"
#include <QStringList>
#include <QDateTime>
#include <QVariantHash>
#include <QTimer>

class QDBusArgument;

/*!
 * An object for storing information about a single notification.
 */
class LIPSTICK_EXPORT LipstickNotification : public QObject
{
    Q_OBJECT
    Q_ENUMS(InformationOrigin)
    Q_PROPERTY(QString appName READ appName CONSTANT)
    Q_PROPERTY(QString explicitAppName READ explicitAppName CONSTANT)
    Q_PROPERTY(QString disambiguatedAppName READ disambiguatedAppName CONSTANT)
    Q_PROPERTY(uint id READ id CONSTANT)
    Q_PROPERTY(QString appIcon READ appIcon NOTIFY appIconChanged)
    Q_PROPERTY(int appIconOrigin READ appIconOrigin NOTIFY appIconOriginChanged)
    Q_PROPERTY(QString summary READ summary NOTIFY summaryChanged)
    Q_PROPERTY(QString body READ body NOTIFY bodyChanged)
    Q_PROPERTY(QStringList actions READ actions CONSTANT)
    Q_PROPERTY(QVariantMap hints READ hintValues NOTIFY hintsChanged)
    Q_PROPERTY(int expireTimeout READ expireTimeout CONSTANT)
    Q_PROPERTY(QDateTime timestamp READ timestamp NOTIFY timestampChanged)
    Q_PROPERTY(QString previewSummary READ previewSummary NOTIFY previewSummaryChanged)
    Q_PROPERTY(QString previewBody READ previewBody NOTIFY previewBodyChanged)
    Q_PROPERTY(QString subText READ subText NOTIFY subTextChanged)
    Q_PROPERTY(int urgency READ urgency NOTIFY urgencyChanged)
    Q_PROPERTY(int itemCount READ itemCount NOTIFY itemCountChanged)
    Q_PROPERTY(int priority READ priority NOTIFY priorityChanged)
    Q_PROPERTY(QString category READ category NOTIFY categoryChanged)
    Q_PROPERTY(bool userRemovable READ isUserRemovable NOTIFY userRemovableChanged)
    Q_PROPERTY(QVariantList remoteActions READ remoteActions NOTIFY remoteActionsChanged)
    Q_PROPERTY(QString owner READ owner CONSTANT)
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(bool hasProgress READ hasProgress NOTIFY hasProgressChanged)
    Q_PROPERTY(bool isTransient READ isTransient NOTIFY isTransientChanged)
    Q_PROPERTY(QString color READ color NOTIFY colorChanged)

public:
    enum Urgency { Low = 0, Normal = 1, Critical = 2 };
    enum InformationOrigin { ExplicitValue, CategoryValue, InferredValue };

    //! Standard hint: The urgency level.
    static const char *HINT_URGENCY;

    //! Standard hint: The type of notification this is.
    static const char *HINT_CATEGORY;

    //! Standard hint: If true, the notification should be removed after display.
    static const char *HINT_TRANSIENT;

    //! Standard hint: If true, the notification should not be removed after activation.
    static const char *HINT_RESIDENT;

    //! Standard hint: Icon of the notification: either a file:// URL, an absolute path, or a token to be satisfied by the 'theme' image provider.
    static const char *HINT_IMAGE_PATH;

    //! Standard hint: Icon of the notification: image data.
    static const char *HINT_IMAGE_DATA;

    //! Standard hint: If true, audible feedback should be should be suppressed during notification feedback.
    static const char *HINT_SUPPRESS_SOUND;

    //! Standard hint: If set, override possible audible feedback sound.
    static const char *HINT_SOUND_FILE;

    //! Standard hint: If set, override possible audible feedback sound.
    static const char *HINT_SOUND_NAME;

    //! Nemo hint: Item count represented by the notification.
    static const char *HINT_ITEM_COUNT;

    //! Nemo hint: Priority level of the notification.
    static const char *HINT_PRIORITY;

    //! Nemo hint: Timestamp of the notification.
    static const char *HINT_TIMESTAMP;

    //! Nemo hint: Body text of the preview of the notification.
    static const char *HINT_PREVIEW_BODY;

    //! Nemo hint: Summary text of the preview of the notification.
    static const char *HINT_PREVIEW_SUMMARY;

    //! Nemo hint: Sub-text of the notification.
    static const char *HINT_SUB_TEXT;

    //! Nemo hint: Remote action of the notification. Prefix only: the action identifier is to be appended.
    static const char *HINT_REMOTE_ACTION_PREFIX;

    //! Nemo hint: Icon for the remote action of the notification. Prefix only: the action identifier is to be appended.
    static const char *HINT_REMOTE_ACTION_ICON_PREFIX;

    //! Nemo hint: Optional type for the remote action of the notification. Prefix only: the action identifier is to be appended.
    static const char *HINT_REMOTE_ACTION_TYPE_PREFIX;

    //! Nemo hint: User removability of the notification.
    static const char *HINT_USER_REMOVABLE;

    //! Nemo hint: Feedback of the notification.
    static const char *HINT_FEEDBACK;

    //! Nemo hint: Whether to turn the screen on when displaying preview
    static const char *HINT_DISPLAY_ON;

    //! Nemo hint: Indicates the Android package name from which this notification originates
    static const char *HINT_ORIGIN_PACKAGE;

    //! Nemo hint: Indicates the identifer of the owner for notification
    static const char *HINT_OWNER;

    //! Nemo hint: progress percentage between 0 and 1, negative for indeterminate
    static const char *HINT_PROGRESS;

    //! Nemo hint: play vibra feedback
    static const char *HINT_VIBRA;

    //! Nemo hint: Indicates the confidentiality of the notification
    static const char *HINT_VISIBILITY;

    /*!
     * Creates an object for storing information about a single notification.
     *
     * \param appName name of the application sending the notification
     * \param explicitAppName name explicitly set on the received notification
     * \param disambiguatedAppName name of the application, decorated to disambiguate names from android and native applications
     * \param id the ID of the notification
     * \param appIcon icon ID of the application sending the notification
     * \param summary summary text for the notification
     * \param body body text for the notification
     * \param actions actions for the notification as a list of identifier/string pairs
     * \param hints hints for the notification
     * \param expireTimeout expiration timeout for the notification
     * \param parent the parent QObject
     */
    LipstickNotification(const QString &appName, const QString &explicitAppName, const QString &disambiguatedAppName,
                         uint id, const QString &appIcon, const QString &summary, const QString &body,
                         const QStringList &actions, const QVariantHash &hints,
                         int expireTimeout, QObject *parent = 0);

    /*!
     * Creates a new uninitialized representation of a notification.
     *
     * \param parent the parent QObject
     */
    LipstickNotification(QObject *parent = 0);

    //! Returns the name of the application sending the notification
    QString appName() const;
    QString explicitAppName() const;
    QString disambiguatedAppName() const;

    //! Sets the name of the application sending the notification
    void setAppName(const QString &appName);
    void setExplicitAppName(const QString &appName);
    void setDisambiguatedAppName(const QString &disambiguatedAppName);

    //! Returns the ID of the notification
    uint id() const;

    //! Returns the icon ID of the application sending the notification
    QString appIcon() const;
    void setAppIcon(const QString &appIcon, int source = ExplicitValue);

    int appIconOrigin() const;

    //! Returns the summary text for the notification
    QString summary() const;

    //! Sets the summary text for the notification
    void setSummary(const QString &summary);

    //! Returns the body text for the notification
    QString body() const;

    //! Sets the body text for the notification
    void setBody(const QString &body);

    //! Returns the actions for the notification as a list of identifier/string pairs
    QStringList actions() const;

    //! Sets the actions for the notification as a list of identifier/string pairs
    void setActions(const QStringList &actions);

    //! Returns the hints for the notification
    QVariantHash hints() const;
    QVariantMap hintValues() const;

    //! Sets the hints for the notification
    void setHints(const QVariantHash &hints);

    //! Returns the expiration timeout for the notification
    int expireTimeout() const;

    //! Sets the expiration timeout for the notification
    void setExpireTimeout(int expireTimeout);

    //! Returns the timestamp for the notification
    QDateTime timestamp() const;

    //! Returns the summary text for the preview of the notification
    QString previewSummary() const;

    //! Returns the body text for the preview of the notification
    QString previewBody() const;

    //! Returns the sub-text for the notification
    QString subText() const;

    //! Returns the urgency of the notification
    int urgency() const;

    //! Returns the item count of the notification
    int itemCount() const;

    //! Returns the priority of the notification
    int priority() const;

    //! Returns the category of the notification
    QString category() const;

    //! Returns whether the notification is transient
    bool isTransient() const;

    //! Returns the color set for notification, commonly just for Android compatibility
    QString color() const;

    //! Returns the user removability of the notification
    bool isUserRemovable() const;

    //! Returns the user removability hint state
    bool isUserRemovableByHint() const;

    //! Returns the remote actions invokable by the notification
    QVariantList remoteActions() const;

    //! Returns an indicator for the notification owner
    QString owner() const;

    //! Returns true if the notification has been restored since it was last modified
    bool restored() const;
    void setRestored(bool restored);

    qreal progress() const;

    bool hasProgress() const;

    //! \internal
    quint64 internalTimestamp() const;

    //! \internal
    void restartProgressTimer();

    // Properties internal to lipstick, not visible over D-Bus
    QVariantHash internalHints() const;
    void setInternalHints(const QVariantHash &hints);

    // Notification came from process with privileged group
    void setPrivilegedSource(bool privileged);
    bool privilegedSource() const;

    /*!
     * Creates a copy of an existing representation of a notification.
     * This constructor should only be used for populating the notification
     * list from D-Bus structures.
     *
     * \param notification the notification representation to a create copy of
     */
    explicit LipstickNotification(const LipstickNotification &notification);

    friend QDBusArgument &operator<<(QDBusArgument &, const LipstickNotification &);
    friend const QDBusArgument &operator>>(const QDBusArgument &, LipstickNotification &);
    //! \internal_end

signals:
    /*!
     * Sent when an action defined for the notification is invoked.
     *
     * \param action the action that was invoked
     * \param actionText parameter for the action
     */
    void actionInvoked(QString action, QString actionText = QString());

    //! Sent when the removal of this notification was requested.
    void removeRequested();

    //! Sent when the summary has been modified
    void summaryChanged();

    //! Sent when the body has been modified
    void bodyChanged();

    //! Sent when the hints have been modified
    void hintsChanged();

    //! Sent when the app icon has been modified
    void appIconChanged();
    void appIconOriginChanged();

    //! Sent when the timestamp has changed
    void timestampChanged();

    //! Sent when the preview summary has been modified
    void previewSummaryChanged();

    //! Sent when the preview body has been modified
    void previewBodyChanged();

    //! Sent when the sub text has been modified
    void subTextChanged();

    //! Sent when the urgency has been modified
    void urgencyChanged();

    //! Sent when the item count has been modified
    void itemCountChanged();

    //! Sent when the priority has been modified
    void priorityChanged();

    //! Sent when the category has been modified
    void categoryChanged();

    //! Sent when the user removability has been modified
    void userRemovableChanged();

    //! Sent when the remote actions have been modified
    void remoteActionsChanged();

    void hasProgressChanged();
    void progressChanged();

    void isTransientChanged();
    void colorChanged();

private:
    void updateHintValues();

    //! Name of the application sending the notification
    QString m_appName;
    QString m_explicitAppName;
    QString m_disambiguatedAppName;

    //! The ID of the notification
    uint m_id;

    QString m_appIcon;
    int m_appIconOrigin = ExplicitValue;

    //! Summary text for the notification
    QString m_summary;

    //! Body text for the notification
    QString m_body;

    //! Actions for the notification as a list of identifier/string pairs
    QStringList m_actions;

    //! Hints for the notification
    QVariantHash m_hints;
    QVariantMap m_hintValues;
    QVariantHash m_internalHints;

    bool m_restored = false;

    //! Expiration timeout for the notification
    int m_expireTimeout;

    // Cached values for speeding up comparisons:
    int m_priority;
    quint64 m_timestamp;
    QTimer *m_activeProgressTimer;
};

// Order notifications by descending priority then timestamp:
bool operator<(const LipstickNotification &lhs, const LipstickNotification &rhs);

Q_DECLARE_METATYPE(LipstickNotification)

class LIPSTICK_EXPORT NotificationList
{
public:
    NotificationList();
    NotificationList(const QList<LipstickNotification *> &notificationList);
    NotificationList(const NotificationList &notificationList);
    NotificationList &operator=(const NotificationList &) = default;
    QList<LipstickNotification *> notifications() const;
    friend QDBusArgument &operator<<(QDBusArgument &, const NotificationList &);
    friend const QDBusArgument &operator>>(const QDBusArgument &, NotificationList &);

private:
    QList<LipstickNotification *> m_notificationList;
};

Q_DECLARE_METATYPE(NotificationList)

#endif // LIPSTICKNOTIFICATION_H
