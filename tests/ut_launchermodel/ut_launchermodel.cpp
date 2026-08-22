/***************************************************************************
**
** Copyright (c) 2014 Jolla Ltd.
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

#include <QtTest/QtTest>

#include "launcheritem.h"
#include "launchermodel.h"
#include "ut_launchermodel.h"
#include "mdesktopentry.h"

class MDesktopEntryPrivate
{
public:
    MDesktopEntryPrivate(const QString &fileName)
        : m_fileName(fileName)
    {
    }

    QString m_fileName;
};

static QHash<QString, QHash<QString, QString> > desktopEntryValues;
static QHash<QString, QStringList> desktopEntryMimeTypes;

MDesktopEntry::MDesktopEntry(const QString &fileName)
    : d_ptr(new MDesktopEntryPrivate(fileName))
{
}

MDesktopEntry::~MDesktopEntry()
{
    delete d_ptr;
}

QString
MDesktopEntry::fileName() const
{
    return d_ptr->m_fileName;
}

QString
MDesktopEntry::exec() const
{
    return "";
}

QString
MDesktopEntry::name() const
{
    return "";
}

QString
MDesktopEntry::icon() const
{
    return "";
}

QString
MDesktopEntry::type() const
{
    return "Application";
}

QStringList
MDesktopEntry::categories() const
{
    return QStringList();
}

QStringList
MDesktopEntry::mimeType() const
{
    return desktopEntryMimeTypes.value(d_ptr->m_fileName);
}

QString
MDesktopEntry::nameUnlocalized() const
{
    return "";
}

bool
MDesktopEntry::noDisplay() const
{
    return false;
}

QStringList
MDesktopEntry::notShowIn() const
{
    return QStringList();
}

bool
MDesktopEntry::isValid() const
{
    return true;
}

uint
MDesktopEntry::hash() const
{
    return 1234;
}

QString
MDesktopEntry::value(const QString &group, const QString &key) const
{
    return desktopEntryValues.value(d_ptr->m_fileName).value(group + QLatin1Char('/') + key);
}

void QTimer::singleShot(int, const QObject *receiver, const char *member)
{
    // The "member" string is of form "1member()", so remove the trailing 1 and the ()
    int memberLength = strlen(member) - 3;
    char modifiedMember[memberLength + 1];
    strncpy(modifiedMember, member + 1, memberLength);
    modifiedMember[memberLength] = 0;
    QMetaObject::invokeMethod(const_cast<QObject *>(receiver), modifiedMember, Qt::DirectConnection);
}

bool QFile::exists() const
{
    return true;
}

void Ut_LauncherModel::init()
{
    desktopEntryValues.clear();
    desktopEntryMimeTypes.clear();
    launcherModel = new LauncherModel();
}

void Ut_LauncherModel::cleanup()
{
    delete launcherModel;
}

void Ut_LauncherModel::testUpdating()
{
    // Test if basic updating behavior works for a random package
    QVERIFY(launcherModel->packageInModel("somepackage") == nullptr);

    launcherModel->updatingStarted("somepackage", "Some Package",
                                   "/usr/share/pixmaps/example.png", "", "org.example.caller");

    auto item = launcherModel->packageInModel("somepackage");
    QVERIFY(item != nullptr);
    QVERIFY(item->updatingProgress() == -1);
    QVERIFY(item->isTemporary());
    QVERIFY(launcherModel->temporaryItemToReplace() == item);

    launcherModel->updatingProgress("somepackage", 20, "org.example.caller");

    QVERIFY(launcherModel->packageInModel("somepackage") == item);
    QVERIFY(item->updatingProgress() == 20);

    launcherModel->updatingProgress("somepackage", 40, "org.example.caller");

    QVERIFY(launcherModel->packageInModel("somepackage") == item);
    QVERIFY(item->updatingProgress() == 40);

    launcherModel->updatingFinished("somepackage", "org.example.caller");

    QVERIFY(launcherModel->packageInModel("somepackage") == nullptr);
    QVERIFY(launcherModel->temporaryItemToReplace() == nullptr);
}

void Ut_LauncherModel::testUpdatingFileAppears()
{
    // Test that starting an update with a non-existing item will add it to the
    // launcher list, and that when the file appears during the updating phase,
    // it will properly be transformed into a non-temporary launcher item and
    // persist even after the updating phase has finished.
    QVERIFY(launcherModel->packageInModel("somepackage") == nullptr);

    const QString DESKTOPFILE("/usr/share/applications/lipstick_ut_launchermodel.desktop");

    launcherModel->updatingStarted("somepackage", "Some Package",
                                   "/usr/share/pixmaps/example.png", DESKTOPFILE,
                                   "org.example.caller");

    auto item = launcherModel->packageInModel("somepackage");
    QVERIFY(item != nullptr);
    QVERIFY(item->updatingProgress() == -1);
    QVERIFY(item->isTemporary());
    QVERIFY(launcherModel->temporaryItemToReplace() == item);

    QVERIFY(launcherModel->itemInModel(DESKTOPFILE) == item);

    QStringList added, modified, removed;
    added << DESKTOPFILE;
    launcherModel->onFilesUpdated(added, modified, removed);

    QVERIFY(launcherModel->itemInModel(DESKTOPFILE) == item);
    QVERIFY(!item->isTemporary());

    launcherModel->updatingFinished("somepackage", "org.example.caller");

    QVERIFY(launcherModel->itemInModel(DESKTOPFILE) == item);
    QVERIFY(!item->isUpdating());

    QVERIFY(launcherModel->packageInModel("somepackage") == nullptr);
    QVERIFY(launcherModel->temporaryItemToReplace() == nullptr);
}

void Ut_LauncherModel::testReplacingLauncherFile()
{
    const QString oldDesktopFile("/usr/share/applications/launcher_old.desktop");
    const QString newDesktopFile("/usr/share/applications/launcher_new.desktop");
    const QString replacementIdentityKey("X-Test-Replacement-Identity");
    const QString replacementIdentity("stable-identity");
    const QString desktopEntryKey(QStringLiteral("Desktop Entry/") + replacementIdentityKey);

    desktopEntryValues[oldDesktopFile][desktopEntryKey] = replacementIdentity;
    desktopEntryValues[newDesktopFile][desktopEntryKey] = replacementIdentity;
    desktopEntryMimeTypes[oldDesktopFile] << QStringLiteral("application/old");
    desktopEntryMimeTypes[newDesktopFile] << QStringLiteral("application/new");
    launcherModel->setReplacementIdentityKey(replacementIdentityKey);
    launcherModel->setBlacklistedApplications(QStringList() << oldDesktopFile);

    launcherModel->onFilesUpdated(QStringList() << oldDesktopFile,
                                  QStringList(), QStringList());
    LauncherItem *item = launcherModel->itemInModel(oldDesktopFile);
    QVERIFY(item != nullptr);
    QVERIFY(item->canOpenMimeType(QStringLiteral("application/old")));
    QVERIFY(!item->canOpenMimeType(QStringLiteral("application/new")));

    LauncherItem *replacedItem = nullptr;
    QString replacedFilePath;
    connect(launcherModel, &LauncherModel::launcherReplaced,
            [&replacedItem, &replacedFilePath](LauncherItem *item, const QString &oldFilePath) {
        replacedItem = item;
        replacedFilePath = oldFilePath;
    });
    QSignalSpy addedSpy(launcherModel, &LauncherModel::itemAdded);
    QSignalSpy removedSpy(launcherModel, &LauncherModel::itemRemoved);
    QSignalSpy blacklistSpy(launcherModel, &LauncherModel::blacklistedApplicationsChanged);

    launcherModel->onFilesUpdated(QStringList() << newDesktopFile,
                                  QStringList(), QStringList() << oldDesktopFile);

    QCOMPARE(launcherModel->itemInModel(newDesktopFile), item);
    QVERIFY(launcherModel->itemInModel(oldDesktopFile) == nullptr);
    QCOMPARE(replacedItem, item);
    QCOMPARE(replacedFilePath, oldDesktopFile);
    QCOMPARE(addedSpy.count(), 0);
    QCOMPARE(removedSpy.count(), 0);
    QCOMPARE(blacklistSpy.count(), 0);
    QCOMPARE(launcherModel->blacklistedApplications(), QStringList() << oldDesktopFile);
    QVERIFY(!launcherModel->isBlacklisted(item));
    QVERIFY(!item->canOpenMimeType(QStringLiteral("application/old")));
    QVERIFY(item->canOpenMimeType(QStringLiteral("application/new")));
}

void Ut_LauncherModel::testRemovingLauncherFile()
{
    const QString desktopFile("/usr/share/applications/launcher_removed.desktop");

    launcherModel->onFilesUpdated(QStringList() << desktopFile,
                                  QStringList(), QStringList());
    QVERIFY(launcherModel->itemInModel(desktopFile) != nullptr);

    QSignalSpy removedSpy(launcherModel, &LauncherModel::itemRemoved);
    launcherModel->onFilesUpdated(QStringList(),
                                  QStringList(), QStringList() << desktopFile);

    QVERIFY(launcherModel->itemInModel(desktopFile) == nullptr);
    QCOMPARE(removedSpy.count(), 1);
}

QTEST_MAIN(Ut_LauncherModel)
