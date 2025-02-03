/***************************************************************************
**
** Copyright (c) 2012 - 2022 Jolla Ltd.
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

#include <dbus/dbus.h>
#include <policy/resource-set.h>
#include <linux/input.h>
#include <QGuiApplication>
#include "homewindow.h"
#include <QQmlContext>
#include <QScreen>
#include <QKeyEvent>
#include <MDConfItem>
#include "utilities/closeeventeater.h"
#include "pulseaudiocontrol.h"
#include "volumecontrol.h"
#include "lipstickqmlpath.h"
#include <QDBusConnection>
#include <QDBusPendingReply>
#include <QDBusPendingCallWatcher>
#include <mce/dbus-names.h>
#include <mce/mode-names.h>

static bool s_hwKeysCreated = false;

VolumeControl::VolumeControl(QObject *parent)
    : VolumeControl(false, parent)
{
}

VolumeControl::VolumeControl(bool hwKeysCapability, QObject *parent) :
    QObject(parent),
    m_window(nullptr),
    m_pulseAudioControl(new PulseAudioControl(this)),
    m_hwKeyResource(nullptr),
    m_hwKeysAcquired(false),
    m_hwKeysEnabled(false),
    m_hwKeysActive(false),
    m_volume(0),
    m_maximumVolume(0),
    m_audioWarning(new MDConfItem("/desktop/nemo/audiowarning", this)),
    m_safeVolume(0),
    m_callActive(false),
    m_upPressed(false),
    m_downPressed(false),
    m_mediaState(MediaStateUnknown)
{
    if (hwKeysCapability) {
        Q_ASSERT_X(!s_hwKeysCreated, Q_FUNC_INFO, "Hw key capable VolumeControl must be a singleton created from C++");

        m_hwKeyResource = new ResourcePolicy::ResourceSet("event");
        m_hwKeysEnabled = true;
        m_hwKeyResource->setAlwaysReply();
        m_hwKeyResource->addResourceObject(new ResourcePolicy::ScaleButtonResource);
        connect(m_hwKeyResource, SIGNAL(resourcesGranted(QList<ResourcePolicy::ResourceType>)),
                this, SLOT(hwKeyResourceAcquired()));
        connect(m_hwKeyResource, SIGNAL(lostResources()), this, SLOT(hwKeyResourceLost()));
        m_hwKeyResource->acquire();

        qApp->installEventFilter(this);
        QTimer::singleShot(0, this, SLOT(createWindow()));

        QDBusConnection systemBus = QDBusConnection::systemBus();

        systemBus.connect(MCE_SERVICE,
                          MCE_SIGNAL_PATH,
                          MCE_SIGNAL_IF,
                          MCE_VOLKEY_INPUT_POLICY_SIG,
                          this, SLOT(inputPolicyChanged(QString)));

        QDBusPendingReply<QString> inputPolicy = systemBus.asyncCall(QDBusMessage::createMethodCall(
                    MCE_SERVICE, MCE_REQUEST_PATH, MCE_REQUEST_IF, MCE_VOLKEY_INPUT_POLICY_GET));
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(inputPolicy, this);
        connect(watcher, &QDBusPendingCallWatcher::finished,
                this, &VolumeControl::inputPolicyReply);

        evaluateKeyState();

        s_hwKeysCreated = true;
    }

    setWarningAcknowledged(false);
    connect(m_audioWarning, SIGNAL(valueChanged()), this, SIGNAL(restrictedVolumeChanged()));
    connect(this, SIGNAL(maximumVolumeChanged()), this, SIGNAL(restrictedVolumeChanged()));
    connect(m_pulseAudioControl, SIGNAL(volumeChanged(int,int)), this, SLOT(setVolume(int,int)));
    connect(m_pulseAudioControl, SIGNAL(highVolume(int)), SLOT(handleHighVolume(int)));
    connect(m_pulseAudioControl, SIGNAL(longListeningTime(int)), SLOT(handleLongListeningTime(int)));
    connect(m_pulseAudioControl, SIGNAL(callActiveChanged(bool)), SLOT(handleCallActive(bool)));
    connect(m_pulseAudioControl, SIGNAL(mediaStateChanged(QString)), SLOT(handleMediaStateChanged(QString)));
    m_pulseAudioControl->update();
}

VolumeControl::~VolumeControl()
{
    if (m_hwKeyResource) {
        m_hwKeyResource->deleteResource(ResourcePolicy::ScaleButtonType);
    }
    delete m_window;
}

void VolumeControl::inputPolicyChanged(const QString &status)
{
    bool inputEnabled = (status != MCE_INPUT_POLICY_DISABLED);

    if (inputEnabled) {
        hwKeysEnabled();
    } else {
        hwKeysDisabled();
    }
}

void VolumeControl::inputPolicyReply(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QString> reply = *watcher;
    if (reply.isError()) {
        qWarning() << MCE_VOLKEY_INPUT_POLICY_GET"() failed!" << reply.error().name() << reply.error().message();
        /* Do not leave input disabled in error situations */
        inputPolicyChanged(QString(MCE_INPUT_POLICY_ENABLED));
    } else {
        inputPolicyChanged(reply.value());
    }
    watcher->deleteLater();
}

int VolumeControl::volume() const
{
    return m_volume;
}

void VolumeControl::setVolume(int volume)
{
    int newVolume = qBound(0, volume, maximumVolume());

    // This must always clamp the volume.
    if (!warningAcknowledged() && m_safeVolume != 0 && newVolume > m_safeVolume) {
        emit showAudioWarning(false);
        newVolume = safeVolume();
    }

    if (newVolume != m_volume) {
        m_volume = volume;
        m_pulseAudioControl->setVolume(m_volume);
        emit volumeChanged();
    }

    setWindowVisible(true);
}

int VolumeControl::maximumVolume() const
{
    return m_maximumVolume;
}

int VolumeControl::safeVolume() const
{
    return m_safeVolume == 0 ? maximumVolume() : m_safeVolume;
}

int VolumeControl::restrictedVolume() const
{
    return !warningAcknowledged() ? safeVolume() : maximumVolume();
}

void VolumeControl::setWindowVisible(bool visible)
{
    if (visible) {
        if (m_window && !m_window->isVisible()) {
            m_window->show();
            emit windowVisibleChanged();
        }
    } else if (m_window != 0 && m_window->isVisible()) {
        m_window->hide();
        emit windowVisibleChanged();
    }
}

bool VolumeControl::windowVisible() const
{
    return m_window != 0 && m_window->isVisible();
}

bool VolumeControl::warningAcknowledged() const
{
    return m_audioWarning->value(false).toBool();
}

void VolumeControl::setWarningAcknowledged(bool acknowledged)
{
    if (m_audioWarning->value(false).toBool() != acknowledged) {
        m_audioWarning->set(acknowledged);
    }
}

bool VolumeControl::callActive() const
{
    return m_callActive;
}

int VolumeControl::mediaState() const
{
    return m_mediaState;
}

void VolumeControl::setVolume(int volume, int maximumVolume)
{
    int clampedMaxVolume = qMax(0, maximumVolume);
    int clampedVolume = qBound(0, volume, maximumVolume);

    bool volumeUpdated = false;
    bool maxVolumeUpdated = false;

    if (m_hwKeyResource && !warningAcknowledged() && m_safeVolume != 0 && clampedVolume+1 > m_safeVolume) {
        emit showAudioWarning(false);
        clampedVolume = safeVolume();
    }

    if (m_maximumVolume != clampedMaxVolume) {
        m_maximumVolume = clampedMaxVolume;
        maxVolumeUpdated = true;
    }

    if (m_volume != clampedVolume) {
        m_volume = clampedVolume;
        volumeUpdated = true;
    }

    if (maxVolumeUpdated) {
        emit maximumVolumeChanged();
    }

    if (volumeUpdated) {
        emit volumeChanged();
    }
}

void VolumeControl::setVolumeUpKeyState(bool pressed)
{
    if (m_upPressed != pressed) {
        m_upPressed = pressed;
        if (m_upPressed) {
            emit volumeKeyPressed(Qt::Key_VolumeUp);
        } else {
            emit volumeKeyReleased(Qt::Key_VolumeUp);
        }
    }
}

void VolumeControl::setVolumeDownKeyState(bool pressed)
{
    if (m_downPressed != pressed) {
        m_downPressed = pressed;
        if (m_downPressed) {
            emit volumeKeyPressed(Qt::Key_VolumeDown);
        } else {
            emit volumeKeyReleased(Qt::Key_VolumeDown);
        }
    }
}

void VolumeControl::evaluateKeyState()
{
    bool hwKeysActive = m_hwKeysAcquired && m_hwKeysEnabled;

    if (m_hwKeysActive != hwKeysActive) {
        m_hwKeysActive = hwKeysActive;
        if (!m_hwKeysActive) {
            setVolumeUpKeyState(false);
            setVolumeDownKeyState(false);
        }
    }
}

void VolumeControl::hwKeysEnabled()
{
    if (!m_hwKeysEnabled) {
        m_hwKeysEnabled = true;
        evaluateKeyState();
    }
}

void VolumeControl::hwKeysDisabled()
{
    if (m_hwKeysEnabled) {
        m_hwKeysEnabled = false;
        evaluateKeyState();
    }
}

void VolumeControl::hwKeyResourceAcquired()
{
    if (!m_hwKeysAcquired ) {
        m_hwKeysAcquired = true;
        evaluateKeyState();
    }
}

void VolumeControl::hwKeyResourceLost()
{
    if (m_hwKeysAcquired) {
        m_hwKeysAcquired = false;
        evaluateKeyState();
    }
}

void VolumeControl::handleHighVolume(int safeLevel)
{
    if (m_safeVolume != safeLevel) {
        m_safeVolume = safeLevel;
        emit safeVolumeChanged();
    }
}

void VolumeControl::handleLongListeningTime(int listeningTime)
{
    // Ignore initial listening time signal, which is sent the first time
    // media audio starts playing after reboot.
    if (listeningTime == 0)
        return;

    setWarningAcknowledged(false);
    setWindowVisible(true);

    // If safe volume step is not defined for this route (safeVolume() returns 0), use
    // maximum volume as the upper bound, otherwise use safe volume step as the upper
    // bound.
    int newVolume = qBound(0, m_volume, safeVolume() == 0 ? maximumVolume() : safeVolume());
    if (newVolume != m_volume) {
        m_volume = newVolume;
        m_pulseAudioControl->setVolume(m_volume);
        emit volumeChanged();
    }

    emit showAudioWarning(false);
}

void VolumeControl::handleCallActive(bool callActive)
{
    if (m_callActive != callActive) {
        m_callActive = callActive;
        emit callActiveChanged();
    }
}

void VolumeControl::handleMediaStateChanged(const QString &state)
{
    int newValue = MediaStateUnknown;

    if (state == "inactive") {
        newValue = MediaStateInactive;
    } else if (state == "foreground") {
        newValue = MediaStateForeground;
    } else if (state == "background") {
        newValue = MediaStateBackground;
    } else if (state == "active") {
        newValue = MediaStateActive;
    }

    if (newValue != m_mediaState) {
        m_mediaState = newValue;
        emit mediaStateChanged();
    }
}

void VolumeControl::createWindow()
{
    m_window = new HomeWindow();
    m_window->setGeometry(QRect(QPoint(), QGuiApplication::primaryScreen()->size()));
    m_window->setCategory(QLatin1String("notification"));
    m_window->setWindowTitle("Volume");
    m_window->setContextProperty("initialSize", QGuiApplication::primaryScreen()->size());
    m_window->setSource(QmlPath::to("volumecontrol/VolumeControl.qml"));
    m_window->installEventFilter(new CloseEventEater(this));
}

bool VolumeControl::eventFilter(QObject *, QEvent *event)
{
    bool handled = false;

    if (m_hwKeysActive && (event->type() == QEvent::KeyPress
                           || event->type() == QEvent::KeyRelease)) {
        bool pressed = (event->type() == QEvent::KeyPress);

        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

        switch (keyEvent->key()) {
        case Qt::Key_VolumeUp:
            setVolumeUpKeyState(pressed);
            handled = true;
            break;

        case Qt::Key_VolumeDown:
            setVolumeDownKeyState(pressed);
            handled = true;
            break;

        default:
            break;
        }
    }

    return handled;
}
