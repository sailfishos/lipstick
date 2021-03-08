/***************************************************************************
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (c) 2012-2019 Jolla Ltd.
** Copyright (c) 2019 Open Mobile Platform LLC.
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
#include <QGuiApplication>
#include <QQmlContext>
#include <QScreen>
#include <qusbmoded.h>
#include "utilities/closeeventeater.h"
#include "notifications/notificationmanager.h"
#include "notifications/lipsticknotification.h"
#include "usbmodeselector.h"
#include "lipstickqmlpath.h"

#include <nemo-devicelock/devicelock.h>

USBModeSelector::USBModeSelector(NemoDeviceLock::DeviceLock *deviceLock, QObject *parent) :
    QObject(parent),
    m_usbMode(new QUsbModed(this)),
    m_deviceLock(deviceLock),
    m_windowVisible(false),
    m_preparingMode()
{
    connect(m_usbMode, &QUsbModed::eventReceived, this, &USBModeSelector::handleUSBEvent);
    connect(m_usbMode, &QUsbModed::currentModeChanged, this, &USBModeSelector::handleUSBState);
    connect(m_usbMode, &QUsbModed::targetModeChanged, this, &USBModeSelector::updateModePreparing);
    connect(m_usbMode, SIGNAL(usbStateError(QString)), this, SIGNAL(showError(QString)));
    connect(m_usbMode, SIGNAL(supportedModesChanged()), this, SIGNAL(supportedModesChanged()));
    connect(m_usbMode, &QUsbModed::availableModesChanged, this, &USBModeSelector::availableModesChanged);

    // Lazy initialize to improve startup time
    QTimer::singleShot(500, this, &USBModeSelector::handleUSBState);
}

void USBModeSelector::setWindowVisible(bool visible)
{
    if (visible) {
        emit dialogShown();
        if (!m_windowVisible) {
            m_windowVisible = true;
            emit windowVisibleChanged();
        }
    } else {
        if (m_windowVisible) {
            m_windowVisible = false;
            emit windowVisibleChanged();
        }
    }
}

bool USBModeSelector::windowVisible() const
{
    return m_windowVisible;
}

QStringList USBModeSelector::supportedModes() const
{
    return m_usbMode->supportedModes();
}

QStringList USBModeSelector::availableModes() const
{
    return m_usbMode->availableModes();
}

void USBModeSelector::handleUSBEvent(const QString &event)
{
    // Events (from usb_moded-dbus.h):
    //
    // Connected, DataInUse, Disconnected, ModeRequest, PreUnmount,
    // ReMountFailed, ModeSettingFailed, ChargerConnected, ChargerDisconnected

    if (event == QUsbModed::Mode::Connected) {
        if (m_deviceLock->state() >= NemoDeviceLock::DeviceLock::Locked) {
            // When the device lock is on and USB is connected, always pretend that the USB mode selection dialog is shown to unlock the touch screen lock
            emit dialogShown();
            emit showNotification(Notification::Locked);
        }
    } else if (event == QUsbModed::Mode::ModeRequest) {
        setWindowVisible(true);
    } else if (event == QUsbMode::Mode::ChargerConnected) {
        // Hide the mode selection dialog and show a mode notification
        setWindowVisible(false);
    }
}

void USBModeSelector::handleUSBState()
{
    // States (from usb_moded-modes.h):
    //
    // Undefined, Ask, MassStorage, Developer, MTP, Host, ConnectionSharing,
    // Diag, Adb, PCSuite, Charging, Charger, ChargingFallback, Busy

    QString mode = m_usbMode->currentMode();
    USBModeSelector::Notification type = Notification::Invalid;

    updateModePreparing();

    if (mode == QUsbModed::Mode::Ask) {
        // This probably isn't necessary, as it'll be handled by ModeRequest
        setWindowVisible(true);
    } else if (mode == QUsbMode::Mode::ChargingFallback) {
        // Do nothing
    } else if (mode == QUsbMode::Mode::Charging) {
        // Hide the mode selection dialog and show a mode notification
        setWindowVisible(false);
    } else if (QUsbMode::isFinalState(mode)) {
        // Hide the mode selection dialog and show a mode notification
        setWindowVisible(false);
        type = convertModeToNotification(mode);
    }

    if (type != Notification::Invalid && type != Notification::Charging)
        emit showNotification(type);
}

void USBModeSelector::setMode(const QString &mode)
{
    m_usbMode->setCurrentMode(mode);
}

bool USBModeSelector::modeRequiresInitialisation(const QString &mode) const
{
    //return (mode == QUsbModed::Mode::MTP);
    return ((mode != QUsbModed::Mode::Undefined)
            && (mode != QUsbModed::Mode::Ask)
            && (mode != QUsbModed::Mode::Charging)
            && (mode != QUsbModed::Mode::Charger)
            && (mode != QUsbModed::Mode::ChargingFallback)
            && (mode != QUsbModed::Mode::Busy));
}

QString USBModeSelector::preparingMode() const
{
    return m_preparingMode;
}

void USBModeSelector::updateModePreparing()
{
    bool preparing = ((m_usbMode->currentMode() == QUsbModed::Mode::Busy)
            && (modeRequiresInitialisation(m_usbMode->targetMode())));

    if (preparing) {
        setPreparingMode(m_usbMode->targetMode());
    } else {
        clearPreparingMode();
    }
}

void USBModeSelector::setPreparingMode(const QString &preparing)
{
    if (preparing != m_preparingMode) {
        m_preparingMode = preparing;
        emit preparingModeChanged(m_preparingMode);
    }
}

void USBModeSelector::clearPreparingMode()
{
    if (!m_preparingMode.isEmpty()) {
        m_preparingMode.clear();
        emit preparingModeChanged(m_preparingMode);
    }
}

QUsbModed * USBModeSelector::getUsbModed()
{
    return m_usbMode;
}

USBModeSelector::Notification USBModeSelector::convertModeToNotification(const QString &mode) const
{
    Notification type = Notification::Invalid;

    if (mode == QUsbModed::Mode::Disconnected) {
        type = Notification::Disconnected;
    } else if (mode == QUsbModed::Mode::ConnectionSharing) {
        type = Notification::ConnectionSharing;
    } else if (mode == QUsbModed::Mode::MTP) {
        type = Notification::MTP;
    } else if (mode == QUsbModed::Mode::MassStorage) {
        type = Notification::MassStorage;
    } else if (mode == QUsbModed::Mode::Developer) {
        type = Notification::Developer;
    } else if (mode == QUsbModed::Mode::PCSuite) {
        type = Notification::PCSuite;
    } else if (mode == QUsbModed::Mode::Adb) {
        type = Notification::Adb;
    } else if (mode == QUsbModed::Mode::Diag) {
        type = Notification::Diag;
    } else if (mode == QUsbModed::Mode::Host) {
        type = Notification::Host;
    }

    return type;
}
