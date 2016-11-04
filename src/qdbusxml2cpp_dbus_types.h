#ifndef QDBUSXML2CPP_DBUS_TYPES
#define QDBUSXML2CPP_DBUS_TYPES

#include <QPair>
#include <QList>
#include <QDBusMetaType>

typedef QPair<QDBusObjectPath, QVariantMap> PathProperties;
Q_DECLARE_METATYPE(PathProperties);

typedef QList<PathProperties> PathPropertiesArray;
Q_DECLARE_METATYPE(PathPropertiesArray);

#endif
