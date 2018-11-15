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
#ifndef USBMODESELECTOR_H
#define USBMODESELECTOR_H

#include <QObject>
#include <QMap>
#include <QStringList>
#include "lipstickglobal.h"

class HomeWindow;
class QUsbModed;

namespace NemoDeviceLock {
class DeviceLock;
}

class LIPSTICK_EXPORT USBModeSelector : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool windowVisible READ windowVisible WRITE setWindowVisible NOTIFY windowVisibleChanged)
    Q_PROPERTY(QStringList supportedUSBModes READ supportedUSBModes NOTIFY supportedUSBModesChanged)
    Q_PROPERTY(QStringList availableUSBModes READ availableUSBModes NOTIFY availableUSBModesChanged)

public:

    explicit USBModeSelector(NemoDeviceLock::DeviceLock *deviceLock, QObject *parent = 0);

    /*!
     * Returns whether the window is visible or not.
     *
     * \return \c true if the window is visible, \c false otherwise
     */
    bool windowVisible() const;

    /*!
     * Sets the visibility of the window.
     *
     * \param visible \c true if the window should be visible, \c false otherwise
     */
    void setWindowVisible(bool visible);

    /*!
     * Returns the supported USB modes.
     *
     * \return a list of numbers of the supported USB modes
     */
    QStringList supportedUSBModes() const;

    /*!
     * Returns the available USB modes.
     *
     * \return a list of numbers of the available USB modes
     */
    QStringList availableUSBModes() const;

    /*!
     * Sets the USB mode to the given mode.
     *
     * \param mode the mode to set
     */
    Q_INVOKABLE void setUSBMode(QString mode);

signals:
    //! Signaled when the USB mode dialog is shown.
    void dialogShown();

    //! Sent when the visibility of the window has changed.
    void windowVisibleChanged();

    //! Sent when the supported USB modes have changed.
    void supportedUSBModesChanged();

    //! Sent when the available USB modes have changed.
    void availableUSBModesChanged();

    void showUnlockScreen();

private slots:
    /*!
     * Shows the USB dialog/banners based on the given USB mode.
     *
     * \param mode the USB mode to show UI elements for
     */
    void handleUSBEvent(QString mode);

    /*!
     * Shows an error string matching the given error code, if any.
     *
     * \param errorCode the error code of the error to be shown
     */
    void showError(const QString &errorCode);

    /*!
     * Shows the USB dialog/banners based on the current USB mode.
     */
    void applyCurrentUSBMode();

private:

    /*!
     * Shows a notification.
     *
     * \param mode the USB mode for the notification
     */
    void showNotification(QString mode);

    void notificationClosed(uint id);

private:

    //! Error code to translation ID mapping
    static QMap<QString, QString> s_errorCodeToTranslationID;

    //! The volume control window
    HomeWindow *m_window;

    //! For getting and setting the USB mode
    QUsbModed *m_usbMode;

    //! For getting information about the device lock state
    NemoDeviceLock::DeviceLock *m_deviceLock;

    uint m_previousNotificationId;

#ifdef UNIT_TEST
    friend class Ut_USBModeSelector;
#endif
};

#endif // USBMODESELECTOR_H
