/***************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (C) 2012-2015 Jolla Ltd.
** Contact: Robin Burchell <robin.burchell@jollamobile.com>
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
#include "homewindow.h"
#include <QQmlContext>
#include <QScreen>
#include "utilities/closeeventeater.h"
#include <qmlocks.h>
#include <qusbmoded.h>
#include "notifications/notificationmanager.h"
#include "usbmodeselector.h"
#include "lipstickqmlpath.h"

#include <nemo-devicelock/devicelock.h>

QMap<QString, QString> USBModeSelector::s_errorCodeToTranslationID;

USBModeSelector::USBModeSelector(NemoDeviceLock::DeviceLock *deviceLock, QObject *parent) :
    QObject(parent),
    m_window(0),
    m_usbMode(new QUsbModed(this)),
    m_deviceLock(deviceLock),
    m_previousNotificationId(0)
{
    if (s_errorCodeToTranslationID.isEmpty()) {
        s_errorCodeToTranslationID.insert("qtn_usb_filessystem_inuse", "qtn_usb_filessystem_inuse");
        s_errorCodeToTranslationID.insert("mount_failed", "qtn_usb_mount_failed");
    }

    connect(m_usbMode, SIGNAL(currentModeChanged()), this, SLOT(applyCurrentUSBMode()));
    connect(m_usbMode, SIGNAL(usbStateError(QString)), this, SLOT(showError(QString)));
    connect(m_usbMode, SIGNAL(supportedModesChanged()), this, SIGNAL(supportedUSBModesChanged()));
    connect(NotificationManager::instance(), &NotificationManager::NotificationClosed,
            this, &USBModeSelector::notificationClosed);

    // Lazy initialize to improve startup time
    QTimer::singleShot(500, this, SLOT(applyCurrentUSBMode()));
}

void USBModeSelector::applyCurrentUSBMode()
{
    applyUSBMode(m_usbMode->currentMode());
}

void USBModeSelector::setWindowVisible(bool visible)
{
    if (visible) {
        emit dialogShown();

        if (m_window == 0) {
            m_window = new HomeWindow();
            m_window->setGeometry(QRect(QPoint(), QGuiApplication::primaryScreen()->size()));
            m_window->setCategory(QLatin1String("dialog"));
            m_window->setWindowTitle("USB Mode");
            m_window->setContextProperty("initialSize", QGuiApplication::primaryScreen()->size());
            m_window->setContextProperty("usbModeSelector", this);
            m_window->setContextProperty("USBMode", m_usbMode);
            m_window->setSource(QmlPath::to("connectivity/USBModeSelector.qml"));
            m_window->installEventFilter(new CloseEventEater(this));
        }

        if (!m_window->isVisible()) {
            m_window->show();
            emit windowVisibleChanged();
        }
    } else if (m_window != 0 && m_window->isVisible()) {
        m_window->hide();
        emit windowVisibleChanged();
    }
}

bool USBModeSelector::windowVisible() const
{
    return m_window != 0 && m_window->isVisible();
}

QStringList USBModeSelector::supportedUSBModes() const
{
    return m_usbMode->supportedModes();
}

void USBModeSelector::applyUSBMode(QString mode)
{
    if (mode == QUsbModed::Mode::Connected) {
        if (m_deviceLock->state() >= NemoDeviceLock::DeviceLock::Locked) {
            // When the device lock is on and USB is connected, always pretend that the USB mode selection dialog is shown to unlock the touch screen lock
            emit dialogShown();

            if (m_usbMode->configMode() != QUsbModed::Mode::Charging) {
                // Show a notification instead if configured USB mode is not charging only.
                NotificationManager *manager = NotificationManager::instance();
                QVariantHash hints;
                hints.insert(NotificationManager::HINT_CATEGORY, "x-nemo.device.locked");
                //% "Unlock device first"
                hints.insert(NotificationManager::HINT_PREVIEW_BODY, qtTrId("qtn_usb_device_locked"));
                manager->Notify(qApp->applicationName(), 0, QString(), QString(), QString(), QStringList(), hints, -1);
                emit showUnlockScreen();
            }
        }
    } else if (mode == QUsbModed::Mode::Ask ||
               mode == QUsbModed::Mode::ModeRequest) {
        setWindowVisible(true);
    } else if (mode != QUsbModed::Mode::Charging &&
               mode != QUsbModed::Mode::Undefined) {
        // Hide the mode selection dialog and show a mode notification
        setWindowVisible(false);
        showNotification(mode);
    }
}

void USBModeSelector::showNotification(QString mode)
{
    QString category;
    QString body;
    if (mode == QUsbModed::Mode::Disconnected) {
        category = "device.removed";
        //% "USB cable disconnected"
        body = qtTrId("qtn_usb_disconnected");
    } else {
        category = "device.added";
        if (mode == QUsbModed::Mode::ConnectionSharing) {
            //% "USB tethering in use"
            body = qtTrId("qtn_usb_connection_sharing_active");
        } else if (mode == QUsbModed::Mode::MTP) {
            //% "MTP mode in use"
            body = qtTrId("qtn_usb_mtp_active");
        } else if (mode == QUsbModed::Mode::MassStorage) {
            //% "Mass storage in use"
            body = qtTrId("qtn_usb_storage_active");
        } else if (mode == QUsbModed::Mode::Developer) {
            //% "SDK mode in use"
            body = qtTrId("qtn_usb_sdk_active");
        } else if (mode == QUsbModed::Mode::PCSuite) {
            //% "Sync and connect in use"
            body = qtTrId("qtn_usb_sync_active");
        } else if (mode == QUsbModed::Mode::Adb) {
            //% "Adb mode in use"
            body = qtTrId("qtn_usb_adb_active");
        } else if (mode == QUsbModed::Mode::Diag) {
            //% "Diag mode in use"
            body = qtTrId("qtn_usb_diag_active");
        } else if (mode == QUsbModed::Mode::Host) {
            //% "USB switched to host mode (OTG)"
            body = qtTrId("qtn_usb_host_mode_active");
        } else {
            return;
        }
    }

    NotificationManager *manager = NotificationManager::instance();
    QVariantHash hints;
    hints.insert(NotificationManager::HINT_CATEGORY, category);
    hints.insert(NotificationManager::HINT_PREVIEW_BODY, body);
    if (m_previousNotificationId != 0) {
        manager->CloseNotification(m_previousNotificationId);
    }
    m_previousNotificationId = manager->Notify(qApp->applicationName(), 0, QString(), QString(), QString(), QStringList(), hints, -1);
}

void USBModeSelector::showError(const QString &errorCode)
{
    if (s_errorCodeToTranslationID.contains(errorCode)) {
        NotificationManager *manager = NotificationManager::instance();
        QVariantHash hints;
        hints.insert(NotificationManager::HINT_CATEGORY, "device.error");
        //% "USB connection error occurred"
        hints.insert(NotificationManager::HINT_PREVIEW_BODY, qtTrId(s_errorCodeToTranslationID.value(errorCode).toUtf8().constData()));
        manager->Notify(qApp->applicationName(), 0, QString(), QString(), QString(), QStringList(), hints, -1);
    }
}

void USBModeSelector::setUSBMode(QString mode)
{
    m_usbMode->setCurrentMode(mode);
}

void USBModeSelector::notificationClosed(uint id)
{
    if (m_previousNotificationId == id) {
        m_previousNotificationId = 0;
    }
}
