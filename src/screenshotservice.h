/***************************************************************************
**
** Copyright (c) 2013 Jolla Ltd.
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
#ifndef SCREENSHOTSERVICE_H
#define SCREENSHOTSERVICE_H

#include <QObject>
#include <QSocketNotifier>
#include <QUrl>

#include "lipstickglobal.h"

class LIPSTICK_EXPORT ScreenshotResult : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QUrl path READ path CONSTANT)
public:
    enum Status {
        Writing,
        Finished,
        Error
    };
    Q_ENUM(Status)

    explicit ScreenshotResult(QObject *parent = nullptr); // For the benefit of qmlRegisterUncreatableType().
    ScreenshotResult(int notifierId, const QUrl &path, QObject *parent = nullptr);
    ~ScreenshotResult();

    Status status() const;
    QUrl path() const;

    void waitForFinished();

signals:
    void finished();
    void statusChanged();

    void error();

private:
    QSocketNotifier m_notifier;
    const QUrl m_path;
    const int m_notifierId;
    Status m_status = Writing;
};

class ScreenshotService : public QObject
{
    Q_OBJECT
public:
    static ScreenshotResult *saveScreenshot(const QString &path);
};

#endif // SCREENSHOTSERVICE_H
