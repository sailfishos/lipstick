/***************************************************************************
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPL included in the packaging
** of this file.
**
****************************************************************************/

#ifndef HOMEAPPLICATION_H_
#define HOMEAPPLICATION_H_

#include <signal.h>
#include <QGuiApplication>
#include "lipstickglobal.h"
#include <touchscreen.h>

class QSocketNotifier;
class QQmlEngine;
class HomeWindow;
class ScreenLock;
class DeviceLock;
class VolumeControl;
class USBModeSelector;
class ShutdownScreen;
class ConnectionSelector;
class ScreenshotService;
class VpnAgent;
class ConnmanVpnProxy;
class ConnectivityMonitor;

/*!
 * Extends QApplication with features necessary to create a desktop.
 */
class LIPSTICK_EXPORT HomeApplication : public QGuiApplication
{
    Q_OBJECT

public:
    /*!
     * Constructs an application object.
     *
     * \param argc number of arguments passed to the application from the command line
     * \param argv argument strings passed to the application from the command line
     * \param qmlPath The path of the QML file to load for the main window
     */
    HomeApplication(int &argc, char **argv, const QString &qmlPath);

    /*!
     * Destroys the application object.
     */
    virtual ~HomeApplication();

    static HomeApplication *instance();

    /*!
      * Gets the main window instance associated to this application.
      * If it hasn't been created yet, this will create it.
      */
    HomeWindow *mainWindowInstance();

    /*!
     * Gets the QQmlEngine used for all the windows in this application.
     */
    QQmlEngine *engine() const;

    /*!
      * Gets the path to the QML file to display.
      */
    const QString &qmlPath() const;

    /*!
      * Sets the path to the QML file to display.
      */
    void setQmlPath(const QString &path);

    /*!
     * Gets the path to the compositor to load.
     */
    const QString &compositorPath() const;
    /*!
     * Sets the path to the compositor QML file to run.  This must be set before
     * the first window is created, or the mainWindowInstance() method is called.
     */
    void setCompositorPath(const QString &path);

    /*!
     * Gets the home active flag.
     */
    bool homeActive() const;

    TouchScreen *touchScreen() const;

    TouchScreen::DisplayState displayState();
    void setDisplayOff();
    bool takeScreenshot(const QString &path);

signals:
    /*!
     * Emitted whenever the home active flag changes.
     */
    void homeActiveChanged();

    /*!
     * Emitted when the home screen has been drawn on screen for the first time.
     */
    void homeReady();

    /*
     * Emitted before the HomeApplication commences destruction.
     */
    void aboutToDestroy();

    /*!
     * Emitted upon display state change.
     */
    void displayStateChanged(TouchScreen::DisplayState oldDisplayState, TouchScreen::DisplayState newDisplayState);

protected:
    virtual bool event(QEvent *);

private:
    void setUpSignalHandlers();

private slots:
    /*!
     * Emits the homeReady() signal unless it has already been sent
     */
    void sendHomeReadySignalIfNotAlreadySent();

    /*!
     * Sends a dbus-signal after UI is visible, stops the process if it has
     * been started by upstart
     */
    void sendStartupNotifications();

    /*!
     * Connects to the compositor's frame swapped signal for sending the
     * startup notifications.
     */
    void connectFrameSwappedSignal(bool mainWindowVisible);

private:
    friend class LipstickApi;

    //! A signal handler that quits the QApplication
    static void quitSignalHandler(int);
    //! An eventfd object for signal handling
    static int s_quitSignalFd;
    //! Socket notifier for signal handling
    QSocketNotifier *m_quitSignalNotifier;

    HomeWindow *m_mainWindowInstance;
    QString m_qmlPath;
    QString m_compositorPath;

    //! QML Engine instance
    QQmlEngine *m_qmlEngine;

    TouchScreen *m_touchScreen;

    //! Logic for locking and unlocking the screen
    ScreenLock *m_screenLock;

    //! Logic for setting the device volume
    VolumeControl *m_volumeControl;

    //! Logic for showing the USB mode selection dialog
    USBModeSelector *m_usbModeSelector;

    //! Logic for showing the shutdown screen and related notifications
    ShutdownScreen *m_shutdownScreen;

    //! Whether the home ready signal has been sent or not
    bool m_homeReadySent;

    VpnAgent *m_vpnAgent;
    ConnmanVpnProxy * m_connmanVpn;
    ConnectivityMonitor *m_connectivityMonitor;
    bool m_online;
};

#endif /* HOMEAPPLICATION_H_ */
