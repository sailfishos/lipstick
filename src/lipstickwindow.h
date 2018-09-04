/***************************************************************************
**
** Copyright (C) 2018 Jolla Ltd.
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

#ifndef LIPSTICK_WINDOW_H
#define LIPSTICK_WINDOW_H

#include <QObject>
#include "lipstickglobal.h"

class HomeWindow;

class LIPSTICK_EXPORT LipstickWindow : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool windowVisible READ windowVisible WRITE setWindowVisible NOTIFY windowVisibleChanged)

public:
    /*!
     * Creates a lipstick window.
     *
     * \param parent the parent object
     */
    explicit LipstickWindow(QObject *parent = 0);

    /*!
     * Destroys the lipstick window.
     */
    virtual ~LipstickWindow();

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

Q_SIGNALS:
    //! Sent when the visibility of the window has changed.
    void windowVisibleChanged();

protected Q_SLOTS:
    /*!
     * Creates the window.
     */
    virtual void createWindow() = 0;

protected:
    HomeWindow *m_window;
};

#endif
