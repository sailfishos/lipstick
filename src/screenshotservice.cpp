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
#include "lipstickcompositor.h"
#include "screenshotservice.h"

#include <QGuiApplication>
#include <QImage>
#include <QScreen>
#include <QTransform>
#include <QThreadPool>
#include <private/qquickwindow_p.h>
#include <QSharedPointer>
#include <QQuickItemGrabResult>

#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/select.h>

ScreenshotResult::ScreenshotResult(QObject *parent)
    : QObject(parent)
    , m_notifier(0, QSocketNotifier::Read)
    , m_notifierId(0)
    , m_status(Error)
{
}

ScreenshotResult::ScreenshotResult(int notifierId, const QUrl &path, QObject *parent)
    : QObject(parent)
    , m_notifier(notifierId, QSocketNotifier::Read)
    , m_path(path)
    , m_notifierId(notifierId)
{
    connect(&m_notifier, &QSocketNotifier::activated, this, [this] {
        if (m_status == Writing) {
            quint64 status = Writing;
            ssize_t unused = ::read(m_notifier.socket(), &status, sizeof(status));
            Q_UNUSED(unused);

            m_status = Status(status);
            emit statusChanged();

            if (m_status == Finished) {
                emit finished();
            } else if (m_status == Error) {
                emit error();
            }

            deleteLater();
        }
    });
}

ScreenshotResult::~ScreenshotResult()
{
    if (m_notifierId != 0) {
        ::close(m_notifierId);
    }
}

ScreenshotResult::Status ScreenshotResult::status() const
{
    return m_status;
}

QUrl ScreenshotResult::path() const
{
    return m_path;
}

void ScreenshotResult::waitForFinished()
{
    fd_set fdread;
    FD_ZERO(&fdread);
    FD_SET(m_notifierId, &fdread);

    for (;;) {
        const int ret = select(m_notifierId + 1, &fdread, nullptr, nullptr, nullptr);
        if (ret >= 0) {
            quint64 status = Writing;
            ssize_t unused = ::read(m_notifierId, &status, sizeof(status));
            Q_UNUSED(unused);

            m_status = Status(status);
            break;
        } else if (errno != EINTR) {
            m_status = Error;
            break;
        }
    }

    deleteLater();
}

class ScreenshotWriter : public QRunnable
{
public:
    ScreenshotWriter(int notifierId, const QImage &image, const QString &path, int rotation);
    ~ScreenshotWriter();

    void run() override;

private:
    QImage m_image;
    const QString m_path;
    const int m_notifierId;
    const int m_rotation;
};

ScreenshotWriter::ScreenshotWriter(int notifierId, const QImage &image, const QString &path, int rotation)
    : m_image(image)
    , m_path(path)
    , m_notifierId(::dup(notifierId))
    , m_rotation(rotation)
{
    setAutoDelete(true);
}

ScreenshotWriter::~ScreenshotWriter()
{
    ::close(m_notifierId);
}

void ScreenshotWriter::run()
{
    if (m_rotation != 0) {
        QTransform xform;
        xform.rotate(m_rotation);
        m_image = m_image.transformed(xform, Qt::SmoothTransformation);
    }

    const quint64 status = m_image.save(m_path)
            ? ScreenshotResult::Finished
            : ScreenshotResult::Error;
    const QByteArray path = m_path.toUtf8();
    int r = chown(path.constData(), getuid(), getgid());
    if (r != 0) {
        qWarning() << "Screenshot owner/group could not be set" << strerror(errno);
    }

    ssize_t unused = ::write(m_notifierId, &status, sizeof(status));
    Q_UNUSED(unused);
}

ScreenshotResult *ScreenshotService::saveScreenshot(const QString &path)
{
    LipstickCompositor *compositor = LipstickCompositor::instance();
    if (!compositor) {
        return nullptr;
    }

    const int notifierId = ::eventfd(0, 0);
    if (notifierId == -1) {
        return nullptr;
    }

    ScreenshotResult * const result = new ScreenshotResult(notifierId, path, compositor);

    if (path.isEmpty()) {
        qWarning() << "Screenshot path is empty.";

        const quint64 status = ScreenshotResult::Error;
        ssize_t unused = ::write(notifierId, &status, sizeof(status));
        Q_UNUSED(unused);

        return result;
    }

    auto grabResult = compositor->contentItem()->grabToImage();

    const int rotation(QGuiApplication::primaryScreen()->angleBetween(
                Qt::PrimaryOrientation, compositor->topmostWindowOrientation()));

    connect(grabResult.data(), &QQuickItemGrabResult::ready, result, [=]() {
        QImage grab = grabResult->image();
        QThreadPool::globalInstance()->start(new ScreenshotWriter(notifierId, grab, path, rotation));
    });

    return result;
}
