/***************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
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

#ifndef CONNECTIONSELECTOR_H
#define CONNECTIONSELECTOR_H

#include <QObject>
#include <lipstickglobal.h>
#include <lipstickwindow.h>

class LIPSTICK_EXPORT ConnectionSelector : public LipstickWindow
{
    Q_OBJECT
public:
    /*!
     * Creates a connection selector.
     *
     * \param parent the parent object
     */
    explicit ConnectionSelector(QObject *parent = 0);

    /*!
     * Destroys the connection selector.
     */
    virtual ~ConnectionSelector();

private slots:
    /*!
     * Creates the window.
     */
    void createWindow() override;
};

#endif // CONNECTIONSELECTOR_H
