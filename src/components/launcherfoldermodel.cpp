// This file is part of lipstick, a QML desktop library
//
// Copyright (c) 2014-2017 Jolla Ltd.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License version 2.1 as published by the Free Software Foundation
// and appearing in the file LICENSE.LGPL included in the packaging
// of this file.
//
// This code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.

#include "launcheritem.h"
#include "launcherfoldermodel.h"
#include "launchermodel.h"
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QStandardPaths>
#include <QDir>
#include <QStack>
#include <mdesktopentry.h>
#include <glib.h>
#include <QDebug>

static const int FOLDER_MODEL_SAVE_TIMER_MS = 1000;
static const QString CONFIG_FOLDER_SUBDIRECTORY("/lipstick/");
static const QString CONFIG_MENU_FILENAME("applications.menu");
static const QString DEFAULT_ICON_ID("icon-launcher-folder-01");

static QString absoluteConfigPath(const QString &fileName)
{
    return LauncherFolderModel::configDir() + fileName;
}


QString LauncherFolderModel::s_configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
        + CONFIG_FOLDER_SUBDIRECTORY;

static QString buildBlacklistValue(const QString &directoryId, int i)
{
    QString indexStr = QString::number(i);
    return QString(directoryId.isEmpty() ? indexStr : (directoryId + "-" + indexStr));
}

static void getDirAndIndex(const QString &positionId, QString &directoryId, int &index)
{
    QStringList parts = positionId.split("-");
    QString strIndex = parts.takeLast();
    if (!parts.isEmpty()) {
        directoryId = parts.join("");
    }
    index = strIndex.toInt();
}

struct FolderItem {
    FolderItem(LauncherFolderItem *folder, LauncherItem *item)
        : folder(folder)
        , item(item)
    {
    }

    LauncherFolderItem *folder;
    LauncherItem *item;
};

// This is modeled after the freedesktop.org menu files http://standards.freedesktop.org/menu-spec/latest/
// but handles only the basic elements, i.e. no merging, filtering, layout, etc. is supported.

LauncherFolderItem::LauncherFolderItem(QObject *parent)
    : QObjectListModel(parent), m_iconId(DEFAULT_ICON_ID)
{
    connect(this, &LauncherFolderItem::itemRemoved,
            this, &LauncherFolderItem::handleRemoved);
    connect(this, &LauncherFolderItem::itemAdded,
            this, &LauncherFolderItem::handleAdded);
    connect(this, &LauncherFolderItem::rowsMoved,
            this, &LauncherFolderItem::saveNeeded);
}

LauncherModel::ItemType LauncherFolderItem::type() const
{
    return LauncherModel::Folder;
}

const QString &LauncherFolderItem::title() const
{
    return m_title;
}

void LauncherFolderItem::setTitle(const QString &title)
{
    if (title == m_title)
        return;

    m_title = title;
    emit titleChanged();
    emit saveNeeded();
}

const QString &LauncherFolderItem::iconId() const
{
    return m_iconId;
}

void LauncherFolderItem::setIconId(const QString &icon)
{
    if (icon == m_iconId)
        return;

    m_iconId = icon;
    saveDirectoryFile();
    emit iconIdChanged();
}

bool LauncherFolderItem::isUpdating() const
{
    LauncherFolderItem *me = const_cast<LauncherFolderItem*>(this);
    for (int i = 0; i < rowCount(); ++i) {
        const LauncherItem *launcherItem = qobject_cast<const LauncherItem*>(me->get(i));
        if (launcherItem && launcherItem->isUpdating())
            return true;
    }

    return false;
}

int LauncherFolderItem::updatingProgress() const
{
    int updatingCount = 0;
    int updatingTotal = 0;
    LauncherFolderItem *me = const_cast<LauncherFolderItem*>(this);
    for (int i = 0; i < rowCount(); ++i) {
        const LauncherItem *launcherItem = qobject_cast<const LauncherItem*>(me->get(i));
        if (launcherItem && launcherItem->isUpdating()) {
            int progress = launcherItem->updatingProgress();
            if (progress < 0 || progress > 100)
                return progress;
            ++updatingCount;
            updatingTotal += progress;
        }
    }

    return updatingCount ? updatingTotal / updatingCount : 0;
}

LauncherFolderItem *LauncherFolderItem::parentFolder() const
{
    return m_parentFolder;
}

void LauncherFolderItem::setParentFolder(LauncherFolderItem *parent)
{
    if (parent == m_parentFolder)
        return;

    m_parentFolder = parent;
    emit parentFolderChanged();
}

// Creates a folder and moves the item at that index into the folder
LauncherFolderItem *LauncherFolderItem::createFolder(int index, const QString &name)
{
    if (index < 0 || index > rowCount())
        return 0;

    LauncherFolderItem *folder = new LauncherFolderItem(this);
    folder->setTitle(name);
    folder->setParentFolder(this);
    QObject *item = get(index);
    insertItem(index, folder);
    if (item) {
        removeItem(item);
        folder->addItem(item);
    }

    emit saveNeeded();

    return folder;
}

void LauncherFolderItem::destroyFolder()
{
    if (itemCount() != 0)
        qWarning() << "Removing a folder that is not empty.";
    if (m_parentFolder)
        m_parentFolder->removeItem(this);
    if (!m_directoryFile.isEmpty()) {
        QFile file(m_directoryFile);
        file.remove();
    }

    emit saveNeeded();

    deleteLater();
}

LauncherFolderItem *LauncherFolderItem::findContainer(QObject *item)
{
    LauncherFolderItem *me = const_cast<LauncherFolderItem*>(this);
    for (int i = 0; i < rowCount(); ++i) {
        QObject *obj = me->get(i);
        if (obj == item) {
            return this;
        } else if (LauncherFolderItem *subFolder = qobject_cast<LauncherFolderItem*>(obj)) {
            LauncherFolderItem *folder = subFolder->findContainer(item);
            if (folder)
                return folder;
        }
    }

    return 0;
}

QString LauncherFolderItem::directoryFile() const
{
    return m_directoryFile;
}

void LauncherFolderItem::loadDirectoryFile(const QString &filename)
{
    m_directoryFile = filename;
    if (!m_directoryFile.startsWith('/')) {
        m_directoryFile = absoluteConfigPath(m_directoryFile);
    }

    GKeyFile *keyfile = g_key_file_new();
    GError *err = NULL;

    if (g_key_file_load_from_file(keyfile, m_directoryFile.toLatin1(), G_KEY_FILE_NONE, &err)) {
        m_iconId = QString::fromLatin1(g_key_file_get_string(keyfile, "Desktop Entry", "Icon", &err));
        emit iconIdChanged();
    }

    if (err != NULL) {
        qWarning() << "Failed to load .directory file" << err->message;
        g_error_free(err);
    }

    g_key_file_free(keyfile);
}

void LauncherFolderItem::saveDirectoryFile()
{
    QScopedPointer<QFile> dirFile;
    if (m_directoryFile.isEmpty()) {
        QTemporaryFile *tempFile = new QTemporaryFile(absoluteConfigPath("FolderXXXXXX.directory"));
        dirFile.reset(tempFile);
        tempFile->open();
        tempFile->setAutoRemove(false);
        m_directoryFile = tempFile->fileName();
        emit directoryFileChanged();
        emit saveNeeded();
    } else {
        dirFile.reset(new QFile(m_directoryFile));
        dirFile.data()->open(QIODevice::WriteOnly);
    }

    if (!dirFile.data()->isOpen()) {
        qWarning() << "Cannot open" << m_directoryFile;
        return;
    }

    GKeyFile *keyfile = g_key_file_new();
    GError *err = NULL;

    g_key_file_load_from_file(keyfile, m_directoryFile.toLatin1(), G_KEY_FILE_NONE, &err);
    g_key_file_set_string(keyfile, "Desktop Entry", "Icon", m_iconId.toLatin1());

    gchar *data = g_key_file_to_data(keyfile, NULL, &err);
    dirFile.data()->write(data);
    dirFile.data()->close();
    g_free(data);

    g_key_file_free(keyfile);
}

void LauncherFolderItem::clear()
{
    for (int i = 0; i < rowCount(); ++i) {
        QObject *item = get(i);
        LauncherItem *launcherItem = qobject_cast<LauncherItem*>(item);
        LauncherFolderItem *folder = qobject_cast<LauncherFolderItem*>(item);

        if (launcherItem) {
            disconnect(item, SIGNAL(isTemporaryChanged()), this, SIGNAL(saveNeeded()));
        } else if (folder) {
            disconnect(item, SIGNAL(saveNeeded()), this, SIGNAL(saveNeeded()));
        }

        if (launcherItem || folder) {
            disconnect(item, SIGNAL(isUpdatingChanged()), this, SIGNAL(isUpdatingChanged()));
            disconnect(item, SIGNAL(updatingProgressChanged()), this, SIGNAL(updatingProgressChanged()));
        }
        if (folder) {
            folder->clear();
            folder->deleteLater();
        }
    }
    reset();
}

void LauncherFolderItem::handleAdded(QObject *item)
{
    const LauncherItem *launcherItem = qobject_cast<const LauncherItem*>(item);
    const LauncherFolderItem *folder = qobject_cast<const LauncherFolderItem*>(item);

    if (launcherItem) {
        if (launcherItem->isUpdating()) {
            emit isUpdatingChanged();
            emit updatingProgressChanged();
        }
        connect(item, SIGNAL(isTemporaryChanged()), this, SIGNAL(saveNeeded()));
    } else if (folder) {
        if (folder->isUpdating()) {
            emit isUpdatingChanged();
            emit updatingProgressChanged();
        }
        connect(item, SIGNAL(saveNeeded()), this, SIGNAL(saveNeeded()));
    }

    if (launcherItem || folder) {
        connect(item, SIGNAL(isUpdatingChanged()), this, SIGNAL(isUpdatingChanged()));
        connect(item, SIGNAL(updatingProgressChanged()), this, SIGNAL(updatingProgressChanged()));
    }

    emit saveNeeded();
}

void LauncherFolderItem::handleRemoved(QObject *item)
{
    const LauncherItem *launcherItem = qobject_cast<const LauncherItem*>(item);
    const LauncherFolderItem *folder = qobject_cast<const LauncherFolderItem*>(item);

    if (launcherItem) {
        if (launcherItem->isUpdating()) {
            emit isUpdatingChanged();
            emit updatingProgressChanged();
        }
        disconnect(item, SIGNAL(isTemporaryChanged()), this, SIGNAL(saveNeeded()));
    } else if (folder) {
        if (folder->isUpdating()) {
            emit isUpdatingChanged();
            emit updatingProgressChanged();
        }
        disconnect(item, SIGNAL(saveNeeded()), this, SIGNAL(saveNeeded()));
    }

    if (launcherItem || folder) {
        disconnect(item, SIGNAL(isUpdatingChanged()), this, SIGNAL(isUpdatingChanged()));
        disconnect(item, SIGNAL(updatingProgressChanged()), this, SIGNAL(updatingProgressChanged()));
    }

    emit saveNeeded();
}

class DeferredLauncherModel : public LauncherModel
{
public:
    explicit DeferredLauncherModel(QObject *parent = 0)
        : LauncherModel(DeferInitialization, parent)
    {
    }

    using LauncherModel::initialize;
};

//============

LauncherFolderModel::LauncherFolderModel(QObject *parent)
    : LauncherFolderItem(parent)
    , m_launcherModel(new DeferredLauncherModel(this))
    , m_loading(false)
    , m_initialized(false)
{
    connect(m_launcherModel, &LauncherModel::directoriesChanged, this, &LauncherFolderModel::directoriesChanged);
    connect(m_launcherModel, &LauncherModel::blacklistedApplicationsChanged, this, &LauncherFolderModel::blacklistedApplicationsChanged);
    connect(m_launcherModel, &LauncherModel::blacklistedApplicationsChanged, this, &LauncherFolderModel::updateblacklistedApplications);
    connect(m_launcherModel, &LauncherModel::iconDirectoriesChanged, this, &LauncherFolderModel::iconDirectoriesChanged);
    connect(m_launcherModel, &LauncherModel::categoriesChanged, this, &LauncherFolderModel::categoriesChanged);

    initialize();
}

LauncherFolderModel::LauncherFolderModel(InitializationMode, QObject *parent)
    : LauncherFolderItem(parent)
    , m_launcherModel(new DeferredLauncherModel(this))
    , m_loading(false)
    , m_initialized(false)
{
    connect(m_launcherModel, &LauncherModel::directoriesChanged, this, &LauncherFolderModel::directoriesChanged);
    connect(m_launcherModel, &LauncherModel::blacklistedApplicationsChanged, this, &LauncherFolderModel::blacklistedApplicationsChanged);
    connect(m_launcherModel, &LauncherModel::blacklistedApplicationsChanged, this, &LauncherFolderModel::updateblacklistedApplications);
    connect(m_launcherModel, &LauncherModel::iconDirectoriesChanged, this, &LauncherFolderModel::iconDirectoriesChanged);
    connect(m_launcherModel, &LauncherModel::categoriesChanged, this, &LauncherFolderModel::categoriesChanged);
}

void LauncherFolderModel::initialize()
{
    if (m_initialized)
        return;
    m_initialized = true;

    m_launcherModel->initialize();

    m_saveTimer.setSingleShot(true);
    connect(m_launcherModel, SIGNAL(itemRemoved(QObject*)),
            this, SLOT(appRemoved(QObject*)));
    connect(m_launcherModel, SIGNAL(itemAdded(QObject*)),
            this, SLOT(appAdded(QObject*)));
    connect(m_launcherModel, (void (LauncherModel::*)(LauncherItem *))&LauncherModel::notifyLaunching,
            this, &LauncherFolderModel::notifyLaunching);
    connect(m_launcherModel, (void (LauncherModel::*)(LauncherItem *))&LauncherModel::canceledNotifyLaunching,
            this, &LauncherFolderModel::canceledNotifyLaunching);
    connect(&m_saveTimer, SIGNAL(timeout()), this, SLOT(save()));

    QDir config;
    config.mkpath(configDir());

    load();

    connect(this, SIGNAL(saveNeeded()), this, SLOT(scheduleSave()));
}

LauncherModel *LauncherFolderModel::allItems() const
{
    return m_launcherModel;
}

QString LauncherFolderModel::scope() const
{
    return m_launcherModel->scope();
}

void LauncherFolderModel::setScope(const QString &scope)
{
    if (m_launcherModel->scope() != scope) {
        m_launcherModel->setScope(scope);
        emit scopeChanged();

        if (m_initialized) {
            load();
        }
    }
}

QStringList LauncherFolderModel::directories() const
{
    return m_launcherModel->directories();
}

void LauncherFolderModel::setDirectories(QStringList dirs)
{
    m_launcherModel->setDirectories(dirs);
}

QStringList LauncherFolderModel::iconDirectories() const
{
    return m_launcherModel->iconDirectories();
}

void LauncherFolderModel::setIconDirectories(QStringList dirs)
{
    m_launcherModel->setIconDirectories(dirs);
}

QStringList LauncherFolderModel::categories() const
{
    return m_launcherModel->categories();
}

void LauncherFolderModel::setCategories(const QStringList &categories)
{
    m_launcherModel->setCategories(categories);
}

void LauncherFolderModel::blacklistApps(LauncherFolderItem *folder, const QString &directoryId)
{
    for (int i = 0; i < folder->rowCount(); ++i) {
        LauncherItem *item = qobject_cast<LauncherItem*>(folder->get(i));
        if (item && m_launcherModel->isBlacklisted(item)) {
            item->setIsBlacklisted(true);
            if (!m_loading) {
                m_blacklistedApplications.insert(item->filePath(), buildBlacklistValue(directoryId, i));
            }
        } else if (LauncherFolderItem *subFolder = qobject_cast<LauncherFolderItem*>(folder->get(i))) {
            blacklistApps(subFolder, subFolder->directoryFile());
        }
    }
}

void LauncherFolderModel::removeAppsFromBlacklist()
{
    QMap<QString, QString>::iterator i = m_blacklistedApplications.begin();

    // Same index can exist in subfolders. Thus, multimap.
    QMultiMap<int, FolderItem*> unblacklistedItems;

    while (i != m_blacklistedApplications.end()) {
        LauncherItem* item = m_launcherModel->itemInModel(i.key());
        if (!item) {
            i = m_blacklistedApplications.erase(i);
        } else if (!m_launcherModel->isBlacklisted(item)) {
            QString positionId = i.value();
            QString directory;
            int index = -1;
            getDirAndIndex(positionId, directory, index);

            LauncherFolderItem *folder = nullptr;
            if (!directory.isEmpty()) {
                folder = findContainerFolder(directory);
            }

            unblacklistedItems.insert(index, new FolderItem(folder, item));
            item->setIsBlacklisted(false);
            i = m_blacklistedApplications.erase(i);
        } else {
            ++i;
        }
    }

    QMultiMap<int, FolderItem*>::iterator unblacklistIterator = unblacklistedItems.begin();
    while (unblacklistIterator != unblacklistedItems.end()) {
        int index = unblacklistIterator.key();
        FolderItem *folderItem = unblacklistIterator.value();
        if (folderItem) {
            if (folderItem->folder) {
                folderItem->folder->insertItem(index, folderItem->item);
            } else {
                insertItem(index, folderItem->item);
            }
        }

        delete folderItem;
        unblacklistIterator = unblacklistedItems.erase(unblacklistIterator);
    }
}

void LauncherFolderModel::updateAppsInBlacklistedFolders()
{
    QMap<QString, QString> tmpDesktopFiles;
    QMap<QString, QString>::iterator i = m_blacklistedApplications.begin();
    while (i != m_blacklistedApplications.end()) {
        QString positionId = i.value();
        QString directory;
        int index = -1;
        getDirAndIndex(positionId, directory, index);
        if (!findContainerFolder(directory) && positionId.startsWith(directory)) {
            positionId.remove(directory + "-");
            tmpDesktopFiles.insert(i.key(), positionId);
            i = m_blacklistedApplications.erase(i);
        } else {
            ++i;
        }
    }

    i = tmpDesktopFiles.begin();
    while (i != tmpDesktopFiles.end()) {
        m_blacklistedApplications.insert(i.key(), i.value());
        ++i;
    }
}

LauncherFolderItem *LauncherFolderModel::findContainerFolder(const QString &directoryId) const
{
    LauncherFolderModel *me = const_cast<LauncherFolderModel*>(this);
    for (int i = 0; i < rowCount(); ++i) {
        QObject *obj = me->get(i);
        LauncherFolderItem *folder = qobject_cast<LauncherFolderItem*>(obj);
        if (folder && (folder->directoryFile() == directoryId)) {
            return folder;
        }
    }

    return 0;
}

void LauncherFolderModel::setBlacklistedApplications(const QStringList &applications)
{
    m_launcherModel->setBlacklistedApplications(applications);
}

QStringList LauncherFolderModel::blacklistedApplications() const
{
    return m_launcherModel->blacklistedApplications();
}

// Move item to folder at index. If index < 0 the item will be appended.
bool LauncherFolderModel::moveToFolder(QObject *item, LauncherFolderItem *folder, int index)
{
    if (!item || !folder)
        return false;

    LauncherFolderItem *source = findContainer(item);
    if (!source)
        return false;

    source->removeItem(item);
    if (index >= 0)
        folder->insertItem(index, item);
    else
        folder->addItem(item);
    if (LauncherFolderItem *f = qobject_cast<LauncherFolderItem*>(item))
        f->setParentFolder(folder);

    scheduleSave();

    return true;
}

// An app removed from system
void LauncherFolderModel::appRemoved(QObject *item)
{
    if (LauncherItem *launcherItem = qobject_cast<LauncherItem*>(item)) {
        emit applicationRemoved(launcherItem);
    }

    LauncherFolderItem *folder = findContainer(item);
    if (folder) {
        folder->removeItem(item);
        scheduleSave();
    }
}

// An app added to system
void LauncherFolderModel::appAdded(QObject *item)
{
    addItem(item);
    scheduleSave();
}

void LauncherFolderModel::updateblacklistedApplications()
{
    // Remove apps from blacklist that are no longer blacklisted.
    removeAppsFromBlacklist();

    QString positionId;
    blacklistApps(this, positionId);

    QMap<QString, QString>::iterator i = m_blacklistedApplications.begin();
    while (i != m_blacklistedApplications.end()) {
        LauncherItem *item = m_launcherModel->itemInModel(i.key());
        LauncherFolderItem *folder = findContainer(item);
        if (folder) {
            folder->removeItem(item);
        } else {
            removeItem(item);
        }
        ++i;
    }
}

void LauncherFolderModel::import()
{
    for (int i = 0; i < m_launcherModel->rowCount(); ++i) {
        addItem(m_launcherModel->get(i));
    }
}

void LauncherFolderModel::scheduleSave()
{
    if (!m_loading)
        m_saveTimer.start(FOLDER_MODEL_SAVE_TIMER_MS);
}

QString LauncherFolderModel::configFile()
{
    return configDir() + CONFIG_MENU_FILENAME;
}

void LauncherFolderModel::setConfigDir(const QString &dirPath)
{
    s_configDir = dirPath;
}

QString LauncherFolderModel::configDir()
{
    return s_configDir;
}

static QString configurationFileForScope(const QString &scope)
{
    return !scope.isEmpty()
            ? LauncherFolderModel::configDir() + scope + QStringLiteral(".menu")
            : LauncherFolderModel::configFile();
}

void LauncherFolderModel::save()
{
    m_saveTimer.stop();
    QFile file(configurationFileForScope(m_launcherModel->scope()));
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to save apps menu" << configFile();
        return;
    }

    // Folder might have been removed. Let's update blacklisted apps and their folders.
    // When folder is removed, an app is shifted back to main level.
    updateAppsInBlacklistedFolders();

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();

    QString directoryId;
    saveFolder(xml, this, directoryId);
    xml.writeEndDocument();
}

void LauncherFolderModel::saveFolder(QXmlStreamWriter &xml, LauncherFolderItem *folder, const QString &directoryId)
{
    xml.writeStartElement("Menu");
    xml.writeTextElement("Name", folder->title());
    if (!folder->directoryFile().isEmpty())
        xml.writeTextElement("Directory", folder->directoryFile());

    for (int i = 0; i < folder->rowCount(); ++i) {
        LauncherItem *item = qobject_cast<LauncherItem*>(folder->get(i));
        LauncherFolderItem *subFolder = qobject_cast<LauncherFolderItem*>(folder->get(i));

        QString currentPosId = buildBlacklistValue(directoryId, i);

        // Blacklisted apps have been removed from the LauncherFolderModel already over here.
        // Populate the menu still so that it contains also blacklisted apps.
        QStringList desktopFiles;
        if (!m_blacklistedApplications.key(currentPosId).isEmpty()) {
            desktopFiles = m_blacklistedApplications.keys(currentPosId);
        } else if (folder && !folder->directoryFile().isEmpty()) {
            // If app is in out of bounds indexes of the current folder.
            QMap<QString, QString>::const_iterator iter = m_blacklistedApplications.constBegin();
            while (iter != m_blacklistedApplications.constEnd()) {
                QString positionId = iter.value();
                QString directory;
                int folderIndex;
                getDirAndIndex(positionId, directory, folderIndex);
                if (positionId.startsWith(directoryId) && folderIndex >= i && (i == (folder->rowCount() - 1))) {
                    desktopFiles.append(iter.key());
                }
                ++iter;
            }
        }

        for (const QString &desktopFile : desktopFiles) {
            if (LauncherItem * item = m_launcherModel->itemInModel(desktopFile)) {
                xml.writeTextElement("Filename", item->filename());
            }
        }

        if (item) {
            if (!item->isTemporary()) {
                xml.writeTextElement("Filename", item->filename());
            }
        } else if (subFolder) {
            saveFolder(xml, subFolder, subFolder->directoryFile());
        }
    }
    xml.writeEndElement();
}

void LauncherFolderModel::load()
{
    m_loading = true;
    clear();

    QFile file(configurationFileForScope(m_launcherModel->scope()));
    if (!file.open(QIODevice::ReadOnly)) {
        // We haven't saved a folder model yet - import all apps.
        import();
        m_loading = false;
        return;
    }

    QVector<bool> loadedItems(m_launcherModel->itemCount());
    loadedItems.fill(false);
    QStack<LauncherFolderItem*> menus;
    QString textData;

    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == QLatin1String("Menu")) {
                LauncherFolderItem *folder = 0;
                if (menus.isEmpty())
                    folder = this;
                else
                    folder = new LauncherFolderItem(this);
                if (!menus.isEmpty()) {
                    folder->setParentFolder(menus.top());
                    menus.top()->addItem(folder);
                }
                menus.push(folder);
            }
        } else if (xml.isEndElement()) {
            if (xml.name() == QLatin1String("Menu")) {
                menus.pop();
            } else if (xml.name() == QLatin1String("Name")) {
                if (!menus.isEmpty()) {
                    menus.top()->setTitle(textData);
                }
            } else if (xml.name() == QLatin1String("Directory")) {
                if (!menus.isEmpty()) {
                    LauncherFolderItem *folder = menus.top();
                    folder->loadDirectoryFile(textData);
                }
            } else if (xml.name() == QLatin1String("Filename")) {
                if (!menus.isEmpty()) {
                    int idx = m_launcherModel->indexInModel(textData);
                    if (idx >= 0) {
                        loadedItems[idx] = true;
                        LauncherItem *item = qobject_cast<LauncherItem*>(m_launcherModel->get(idx));
                        if (item) {
                            LauncherFolderItem *folder = menus.top();
                            if (!item->isBlacklisted()) {
                                folder->addItem(item);
                            } else {
                                QString positionId;
                                // Current count == normal index.
                                int lastIndex = folder->itemCount();
                                if (!folder->parentFolder()) {
                                    positionId = QString::number(lastIndex);
                                } else {
                                    positionId = QString("%1-%2").arg(folder->directoryFile()).arg(lastIndex);
                                }

                                m_blacklistedApplications.insert(item->filePath(), positionId);
                            }
                        }
                    }
                }
            }
            textData.clear();
        } else if (xml.isCharacters()) {
            textData = xml.text().toString();
        }
    }

    for (int i = 0; i < loadedItems.count(); ++i) {
        if (!loadedItems.at(i)) {
            LauncherItem *item = qobject_cast<LauncherItem*>(m_launcherModel->get(i));
            if (item) {
                if (!item->isBlacklisted()) {
                    addItem(item);
                } else {
                    m_blacklistedApplications.insert(item->filePath(), QString::number(itemCount()));
                }
            }
        }
    }

    m_loading = false;
}
