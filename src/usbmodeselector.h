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
#ifndef USBMODESELECTOR_H
#define USBMODESELECTOR_H

#include <QObject>
#include <QStringList>
#include "lipstickglobal.h"

class QUsbModed;

namespace NemoDeviceLock {
class DeviceLock;
}

class LIPSTICK_EXPORT USBModeSelector : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool windowVisible READ windowVisible WRITE setWindowVisible NOTIFY windowVisibleChanged)
    Q_PROPERTY(QStringList supportedModes READ supportedModes NOTIFY supportedModesChanged)
    Q_PROPERTY(QStringList availableModes READ availableModes NOTIFY availableModesChanged)
    Q_PROPERTY(QString preparingMode READ preparingMode NOTIFY preparingModeChanged)

public:
    /*! Notification enum used to indicate which notification should be
     * presented to the user.
     */
    enum Notification {
        Invalid = -1,
        Disconnected,
        Charging,
        ConnectionSharing,
        MTP,
        MassStorage,
        Developer,
        PCSuite,
        Adb,
        Diag,
        Host,
        Locked
    };
    Q_ENUM(Notification)

    explicit USBModeSelector(NemoDeviceLock::DeviceLock *deviceLock, QObject *parent = 0);

    /*!
     * Returns whether the window should be visible or not.
     *
     * \return \c true if the window should be visible, \c false otherwise
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
    QStringList supportedModes() const;

    /*!
     * Returns the available USB modes.
     *
     * \return a list of numbers of the available USB modes
     */
    QStringList availableModes() const;

    /*!
     * Sets the USB mode to the given mode.
     *
     * \param mode the mode to set
     */
    Q_INVOKABLE void setMode(const QString &mode);

    /*!
     * Returns an empty string if no node is being prepared, or the target mode
     * otherwise.
     *
     * \return an empty string if no mode is being prespared, target mode o/w.
     */
    QString preparingMode() const;

    /*!
     * Returns the \c USBModeSelector object's \c QUsbModed instance, providing
     * access to the class constants and object methods.
     *
     * \return The \c USBModeSelector object's \c QUsbModed instance
     */
    QUsbModed * getUsbModed();

signals:
    //! Signaled when the USB mode dialog is shown.
    void dialogShown();

    //! Sent when the visibility of the window should change.
    void windowVisibleChanged();

    //! Sent when the supported USB modes have changed.
    void supportedModesChanged();

    //! Sent when the available USB modes have changed.
    void availableModesChanged();

    //! Sent to request the unlock screen to be unlocked by the user.
    void showUnlockScreen();

    //! Sent when the preparing state changes.
    void preparingModeChanged(const QString &preparing);

    //! Sent to request a mode notification be shown to the user.
    void showNotification(USBModeSelector::Notification type);

    //! Sent to request an error be displayed to the user.
    void showError(const QString &errorCode);

private:
    /*!
     * Given a mode, returns whether or not the mode could potentially
     * spend a user-relevant delay before the mode becomes avialable.
     * For example, MTP must scan some elements of the filesystem, which
     * can take some minutes depending on the number of files.
     *
     * \param mode the USB mode to check
     * \return \c true if starting the mode may be delayed, \c false otherwise
     */
    bool modeRequiresInitialisation(const QString &mode) const;

    /*!
     * Set the target state that the USB mode is preparing for.
     *
     * \param the target mode being prepared for.
     */
    void setPreparingMode(const QString &preparing);

    /*!
     * Clear the target state that the USB mode is preparing for. This must
     * be called when there are no longer ongoing preparations.
     */
    void clearPreparingMode();

    /*!
     * Handle a USB event arriving from QUsbModed. Triggers notification
     * and dialogue signals.
     *
     * \param mode the USB mode to show UI elements for
     */
    void handleUSBEvent(const QString &event);

    /*!
     * Handle a USB state change in QUsbModed. Triggers notification
     * and dialogue signals.
     *
     * \param mode the USB mode to show UI elements for
     */
    void handleUSBState();

    /*!
     * Convert the USB mode into an appropriate notification enum value. The
     * notification enum value is used to choose an appropriate notirication
     * to display to the user.
     *
     * \param mode the USB mode to convert.
     * \return The notification enum value associated with the mode.
     */
    Notification convertModeToNotification(const QString &mode) const;

    /*!
     * Updates the modePreparing property based on the current USB mode
     * and the target USB mode. The values for the mode are taken from
     * from QUsbModed.
     */
    void updateModePreparing();

private:
    //! For getting and setting the USB mode
    QUsbModed *m_usbMode;

    //! For getting information about the device lock state
    NemoDeviceLock::DeviceLock *m_deviceLock;

    //! Whether or not the USB mode selection dialog is currently visible
    bool m_windowVisible;

    //! State indicating whether the device is preparing a USB mode or not
    QString m_preparingMode;

#ifdef UNIT_TEST
    friend class Ut_USBModeSelector;
#endif
};

#endif // USBMODESELECTOR_H
