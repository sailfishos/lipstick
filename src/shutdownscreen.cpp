/***************************************************************************
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (c) 2012 - 2020 Jolla Ltd.
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
#include <QGuiApplication>
#include <QDBusContext>
#include <QDBusConnectionInterface>
#include <QFileInfo>
#include "homewindow.h"
#include <QQmlContext>
#include <QScreen>
#include "utilities/closeeventeater.h"
#include "notifications/notificationmanager.h"
#include "notifications/lipsticknotification.h"
#include "notifications/thermalnotifier.h"
#include "homeapplication.h"
#include "shutdownscreen.h"
#include "lipstickqmlpath.h"
#include <unistd.h>

ShutdownScreen::ShutdownScreen(QObject *parent) :
    QObject(parent),
    QDBusContext(),
    m_window(0),
    m_systemState(new DeviceState::DeviceState(this)),
    m_user(getuid())
{
    connect(m_systemState, &DeviceState::DeviceState::systemStateChanged, this, &ShutdownScreen::applySystemState);
    connect(m_systemState, &DeviceState::DeviceState::nextUserChanged, this, &ShutdownScreen::setUser);
}

void ShutdownScreen::setWindowVisible(bool visible)
{
    if (visible) {
        if (m_window == 0) {
            m_window = new HomeWindow();
            m_window->setGeometry(QRect(QPoint(), QGuiApplication::primaryScreen()->size()));
            m_window->setCategory(QLatin1String("notification"));
            m_window->setWindowTitle("Shutdown");
            m_window->setContextProperty("initialSize", QGuiApplication::primaryScreen()->size());
            m_window->setContextProperty("shutdownScreen", this);
            m_window->setContextProperty("shutdownMode", m_shutdownMode);
            m_window->setContextProperty("user", m_user);
            m_window->setSource(QmlPath::to("system/ShutdownScreen.qml"));
            m_window->installEventFilter(new CloseEventEater(this));
        }

        if (!m_window->isVisible()) {
            m_window->setContextProperty("shutdownMode", m_shutdownMode);
            m_window->show();
            emit windowVisibleChanged();
        }
    } else if (m_window != 0 && m_window->isVisible()) {
        m_window->hide();
        emit windowVisibleChanged();
    }
}

bool ShutdownScreen::windowVisible() const
{
    return m_window != 0 && m_window->isVisible();
}

void ShutdownScreen::applySystemState(DeviceState::DeviceState::StateIndication what)
{
    switch (what) {
        case DeviceState::DeviceState::Shutdown:
            setWindowVisible(true);
            break;

        case DeviceState::DeviceState::ThermalStateFatal:
            //% "Temperature too high. Device shutting down."
            ThermalNotifier::publishTemperatureNotification(qtTrId("qtn_shut_high_temp"));
            break;

        case DeviceState::DeviceState::ShutdownDeniedUSB:
            //% "USB cable plugged in. Unplug the USB cable to shutdown."
            publishNotification("icon-system-usb", "accessory_connected", qtTrId("qtn_shut_unplug_usb"));
            break;

        case DeviceState::DeviceState::BatteryStateEmpty:
            //% "Battery empty. Device shutting down."
            publishNotification("icon-system-battery", "battery_empty", qtTrId("qtn_shut_batt_empty"));
            break;

        case DeviceState::DeviceState::Reboot:
            // Set shutdown mode unless already set explicitly
            if (m_shutdownMode.isEmpty()) {
                m_shutdownMode = "reboot";
                m_window->setContextProperty("shutdownMode", m_shutdownMode);
            }
            break;

        case DeviceState::DeviceState::UserSwitching:
            m_shutdownMode = "userswitch";
            applySystemState(DeviceState::DeviceState::Shutdown);
            break;

        case DeviceState::DeviceState::UserSwitchingFailed:
            m_shutdownMode = "userswitchFailed";
            m_window->setContextProperty("shutdownMode", m_shutdownMode);
            emit userSwitchFailed();
            QTimer::singleShot(10000, this, [this] {
                // Allow for some time for the notification to be visible
                setWindowVisible(false);
                m_shutdownMode.clear();
            });
            break;

        default:
            break;
    }
}

void ShutdownScreen::setUser(uint uid)
{
    m_user = uid;
}

void ShutdownScreen::publishNotification(const QString &icon, const QString &feedback, const QString &body)
{
    QVariantHash hints;
    hints.insert(LipstickNotification::HINT_URGENCY, LipstickNotification::Critical);
    hints.insert(LipstickNotification::HINT_TRANSIENT, true);
    hints.insert(LipstickNotification::HINT_FEEDBACK, feedback);

    NotificationManager *manager = NotificationManager::instance();
    manager->Notify(manager->systemApplicationName(), 0, icon, QString(), body, QStringList(), hints, -1);
}

void ShutdownScreen::setShutdownMode(const QString &mode)
{
    if (!isPrivileged())
        return;

    m_shutdownMode = mode;
    applySystemState(DeviceState::DeviceState::Shutdown);
}

bool ShutdownScreen::isPrivileged()
{
    if (!calledFromDBus()) {
        // Local function calls are always privileged
        return true;
    }

    // Get the PID of the calling process
    pid_t pid = connection().interface()->servicePid(message().service());

    // The /proc/<pid> directory is owned by EUID:EGID of the process
    QFileInfo info(QString("/proc/%1").arg(pid));
    if (info.group() != "privileged" && info.owner() != "root") {
        sendErrorReply(QDBusError::AccessDenied,
                QString("PID %1 is not in privileged group").arg(pid));
        return false;
    }

    return true;
}
