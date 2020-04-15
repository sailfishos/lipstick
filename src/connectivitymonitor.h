/***************************************************************************
**
** Copyright (c) 2016 Jolla Ltd.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPL included in the packaging
** of this file.
**
****************************************************************************/

#ifndef CONNECTIVITYMONITOR_H_
#define CONNECTIVITYMONITOR_H_

#include "lipstickglobal.h"

#include <QObject>
#include <QList>
#include <QMap>

class ConnmanManagerProxy;
class ConnmanServiceProxy;

class LIPSTICK_EXPORT ConnectivityMonitor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<QString> activeConnectionTypes READ activeConnectionTypes NOTIFY connectivityChanged)

public:
    ConnectivityMonitor(QObject *parent = 0);

    virtual ~ConnectivityMonitor();

    QList<QString> activeConnectionTypes() const;

signals:
    void connectivityChanged(const QList<QString> &activeTypes);

private:
    void fetchServiceList();

    void monitorService(const QString &type, const QString &path);
    void forgetService(const QString &path);

    void serviceActive(const QString &type, const QString &path);
    void serviceInactive(const QString &path);

    ConnmanManagerProxy *connmanManager_;
    QMap<QString, QStringList> activeServices_;
    QMap<QString, ConnmanServiceProxy *> connmanServices_;
};

#endif /* CONNECTIVITYMONITOR_H_ */
