/***************************************************************************
**
** Copyright (c) 2026 Jolla Mobile Ltd
**
****************************************************************************/

#ifndef FILESERVICEADAPTOR_H
#define FILESERVICEADAPTOR_H

#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusMessage>
#include <QtCore/QString>

class LipstickCompositor;

class FileServiceAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.sailfishos.fileservice")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.sailfishos.fileservice\">\n"
"    <method name=\"checkMimeSupported\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"mimeType\"/>\n"
"      <arg direction=\"out\" type=\"b\" name=\"supported\"/>\n"
"    </method>\n"
"    <method name=\"checkUrlSupported\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"url\"/>\n"
"      <arg direction=\"out\" type=\"b\" name=\"supported\"/>\n"
"    </method>\n"
"    <method name=\"openUrl\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"url\"/>\n"
"    </method>\n"
"  </interface>\n"
        "")

public:
    explicit FileServiceAdaptor(LipstickCompositor *compositor);

public slots:
    Q_NOREPLY void checkMimeSupported(const QString &mimeType, const QDBusMessage &message);
    Q_NOREPLY void checkUrlSupported(const QString &url, const QDBusMessage &message);
    void openUrl(const QString &url, const QDBusMessage &message);

private:
    LipstickCompositor *m_compositor;
};

#endif
