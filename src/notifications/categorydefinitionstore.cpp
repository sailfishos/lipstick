/***************************************************************************
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (c) 2012 Jolla Ltd.
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

#include "categorydefinitionstore.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>

//! The file extension for the category definition files
static const char *FILE_EXTENSION = ".conf";

//! The maximum size of the category definition file
static const uint FILE_MAX_SIZE = 32768;

CategoryDefinitionStore::CategoryDefinitionStore(const QString &categoryDefinitionsPath,
                                                 uint maxStoredCategoryDefinitions, QObject *parent)
    : QObject(parent),
      m_categoryDefinitionsPath(categoryDefinitionsPath),
      m_maxStoredCategoryDefinitions(maxStoredCategoryDefinitions)
{
    if (m_categoryDefinitionsPath.isEmpty()) {
        qWarning() << "CategoryDefinitionStore instantiated without a path";
        return;
    }

    if (!this->m_categoryDefinitionsPath.endsWith('/')) {
        this->m_categoryDefinitionsPath.append('/');
    }

    // Watch for changes in category definition files
    m_categoryDefinitionPathWatcher.addPath(this->m_categoryDefinitionsPath);
    connect(&m_categoryDefinitionPathWatcher, SIGNAL(directoryChanged(QString)),
            this, SLOT(updateCategoryDefinitionFileList()));
    connect(&m_categoryDefinitionPathWatcher, SIGNAL(fileChanged(QString)),
            this, SLOT(updateCategoryDefinitionFile(QString)));
    updateCategoryDefinitionFileList();
}

void CategoryDefinitionStore::updateCategoryDefinitionFileList()
{
    QDir categoryDefinitionsDir(m_categoryDefinitionsPath);

    if (categoryDefinitionsDir.exists()) {
        QStringList filter("*" + QString(FILE_EXTENSION));

        QSet<QString> files = categoryDefinitionsDir.entryList(filter, QDir::Files).toSet();
        QSet<QString> removedFiles = m_categoryDefinitionFiles - files;

        foreach(const QString &removedCategory, removedFiles) {
            QString category = QFileInfo(removedCategory).completeBaseName();
            QString categoryDefinitionPath = m_categoryDefinitionsPath + removedCategory;
            m_categoryDefinitionPathWatcher.removePath(categoryDefinitionPath);
            m_categoryDefinitions.remove(category);
            emit categoryDefinitionUninstalled(category);
        }

        m_categoryDefinitionFiles = files;

        // Add category definition files to watcher
        foreach(QString file, m_categoryDefinitionFiles){
            QString categoryDefinitionFilePath = m_categoryDefinitionsPath + file;
            if (!m_categoryDefinitionPathWatcher.files().contains(categoryDefinitionFilePath)) {
                m_categoryDefinitionPathWatcher.addPath(categoryDefinitionFilePath);
            }
        }
    }
}

void CategoryDefinitionStore::updateCategoryDefinitionFile(const QString &path)
{
    QFileInfo fileInfo(path);
    if (fileInfo.exists()) {
       QString category = fileInfo.completeBaseName();
       loadSettings(category);
       emit categoryDefinitionModified(category);
    }
}

bool CategoryDefinitionStore::categoryDefinitionExists(const QString &category) const
{
    bool categoryFound = false;

    if (!m_categoryDefinitions.contains(category)) {
        // If the category definition has not been loaded yet load it
        loadSettings(category);
    }

    if (m_categoryDefinitions.contains(category)) {
        categoryDefinitionAccessed(category);
        categoryFound = true;
    }

    return categoryFound;
}

QList<QString> CategoryDefinitionStore::allKeys(const QString &category) const
{
    if (categoryDefinitionExists(category)) {
        return m_categoryDefinitions.value(category)->allKeys();
    }

    return QList<QString>();
}

bool CategoryDefinitionStore::contains(const QString &category, const QString &key) const
{
    if (categoryDefinitionExists(category)) {
        return m_categoryDefinitions.value(category)->contains(key);
    }

    return false;
}

QString CategoryDefinitionStore::value(const QString &category, const QString &key) const
{
    if (contains(category, key)) {
        const QVariant &value(m_categoryDefinitions.value(category)->value(key));
        if (value.canConvert<QStringList>()) {
            return value.toStringList().join(QStringLiteral(","));
        } else {
            return value.toString();
        }
    }

    return QString();
}

QHash<QString, QString> CategoryDefinitionStore::categoryParameters(const QString &category) const
{
    QHash<QString, QString> rv;

    if (categoryDefinitionExists(category)) {
        const QSettings &categoryDefinitionSettings(*(m_categoryDefinitions.value(category)));
        foreach (const QString &key, categoryDefinitionSettings.allKeys()) {
            const QVariant &value(categoryDefinitionSettings.value(key));
            if (value.canConvert<QStringList>()) {
                rv.insert(key, value.toStringList().join(QStringLiteral(",")));
            } else {
                rv.insert(key, value.toString());
            }
        }
    }

    return rv;
}

void CategoryDefinitionStore::loadSettings(const QString &category) const
{
    QFileInfo file(QString(m_categoryDefinitionsPath).append(category).append(FILE_EXTENSION));
    if (file.exists() && file.size() != 0 && file.size() <= FILE_MAX_SIZE) {
        QSharedPointer<QSettings> categoryDefinitionSettings(new QSettings(file.filePath(), QSettings::IniFormat));
        if (categoryDefinitionSettings->status() == QSettings::NoError) {
            m_categoryDefinitions.insert(category, categoryDefinitionSettings);
        }
    }
}

void CategoryDefinitionStore::categoryDefinitionAccessed(const QString &category) const
{
    // Mark the category definition as recently used by moving it to the beginning of the usage list
    m_categoryDefinitionUsage.removeAll(category);
    m_categoryDefinitionUsage.insert(0, category);

    // If there are too many category definitions in memory get rid of the extra ones
    while (m_categoryDefinitionUsage.count() > (int)m_maxStoredCategoryDefinitions) {
        m_categoryDefinitions.remove(m_categoryDefinitionUsage.takeLast());
    }
}
