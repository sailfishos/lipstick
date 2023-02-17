/***************************************************************************
**
** Copyright (c) 2012 - 2023 Jolla Ltd.
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
#ifndef THERMALNOTIFIER_H
#define THERMALNOTIFIER_H

#include <QObject>
#include <QString>

class ThermalNotifier : public QObject
{
    Q_OBJECT
public:
    explicit ThermalNotifier(QObject *parent = 0);
    ~ThermalNotifier();

    /*!
     * Shows a system notification.
     *
     * \param body the body text of the notification
     */
    static void publishTemperatureNotification(const QString &body);

private slots:
    /*!
     * Reacts to thermal state changes by showing the
     * related notification.
     *
     * \param state the new thermal state
     */
    void applyThermalState(const QString &state);

private:
#ifdef UNIT_TEST
    friend class Ut_ThermalNotifier;
#endif
};

#endif // THERMALNOTIFIER_H
