/***************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
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

#ifndef TOUCHSCREEN_H
#define TOUCHSCREEN_H

#include <QObject>
#include "lipstickglobal.h"

class TouchScreenPrivate;
class QTimerEvent;

class LIPSTICK_EXPORT TouchScreen : public QObject
{
    Q_OBJECT
    Q_ENUMS(DisplayState)
    Q_PROPERTY(bool touchBlocked READ touchBlocked NOTIFY touchBlockedChanged FINAL)

public:
    enum DisplayState {
        DisplayOff = -1,   // MeeGo::QmDisplayState::Off
        DisplayDimmed = 0, // MeeGo::QmDisplayState::Dimmed
        DisplayOn = 1,     // MeeGo::QmDisplayState::On
        DisplayUnknown     // MeeGo::QmDisplayState::Unknown
    };

    explicit TouchScreen(QObject *parent = 0);
    virtual ~TouchScreen();

    /*!
     * Returns touch blocking state.
     *
     * \return \c true when touch is blocked, \c false otherwise
     */
    bool touchBlocked() const;

    void setEnabled(bool);

    void setDisplayOff();

    DisplayState currentDisplayState() const;

signals:
    //! Emitted when touch blocking changes. Touch is blocked when display is off.
    void touchBlockedChanged();

    /*!
     * Emitted upon display state change.
     */
    void displayStateChanged(DisplayState oldDisplayState, DisplayState newDisplayState);

protected:
    bool eventFilter(QObject *, QEvent *);
    void timerEvent(QTimerEvent *);


private:
    TouchScreenPrivate *d_ptr;
    Q_DISABLE_COPY(TouchScreen)
    Q_DECLARE_PRIVATE(TouchScreen)

#ifdef UNIT_TEST
    friend class Ut_ScreenLock;
    friend class Ut_TouchScreen;
#endif
};

#endif // TOUCHSCREEN_H
