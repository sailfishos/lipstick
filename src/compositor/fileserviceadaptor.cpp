/***************************************************************************
**
** Copyright (c) 2026 Jolla Mobile Ltd
**
****************************************************************************/

#include "fileserviceadaptor.h"

#include "lipstickcompositor.h"

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>

FileServiceAdaptor::FileServiceAdaptor(LipstickCompositor *compositor)
    : QDBusAbstractAdaptor(compositor)
    , m_compositor(compositor)
{
    setAutoRelaySignals(true);
}

void FileServiceAdaptor::checkMimeSupported(const QString &mimeType, const QDBusMessage &message)
{
    message.setDelayedReply(true);
    m_compositor->checkMimeSupported(mimeType, message, QDBusConnection::sessionBus());
}

void FileServiceAdaptor::checkUrlSupported(const QString &url, const QDBusMessage &message)
{
    message.setDelayedReply(true);
    m_compositor->checkUrlSupported(url, message, QDBusConnection::sessionBus());
}

void FileServiceAdaptor::openUrl(const QString &url, const QDBusMessage &)
{
    m_compositor->openUrl(url);
}
