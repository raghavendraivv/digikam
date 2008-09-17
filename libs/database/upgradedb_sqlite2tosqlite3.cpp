/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2005-06-05
 * Description : SQlite 2 to SQlite 3 interface.
 *
 * Copyright (C) 2005 by Renchi Raju <renchi@pooh.tam.uiuc.edu>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

// Qt includes.

#include <QMap>
#include <QPair>
#include <QDir>
#include <QStringList>
#include <QFileInfo>
#include <QList>

// KDE includes.

#include <kstandarddirs.h>
#include <kio/global.h>

// Local includes.

#include "ddebug.h"
#include "albumdb.h"
#include "databaseaccess.h"
#include "databasebackend.h"
#include "albumdb_sqlite2.h"
#include "upgradedb_sqlite2tosqlite3.h"

namespace Digikam
{

struct _Album
{
    int     id;
    QString url;
    QString date;
    QString caption;
    QString collection;
    QString icon;
};

struct _Tag
{
    int     id;
    int     pid;
    QString name;
    QString icon;
};

qlonglong findOrAddImage(DatabaseAccess &access, int dirid, const QString& name,
                         const QString& caption)
{
    QList<QVariant> values;

    access.backend()->execSql(QString("SELECT id FROM Images WHERE dirid=? AND name=?"),
                              dirid, name, &values);

    if (!values.isEmpty())
    {
        return values.first().toLongLong();
    }

    QVariant id;
    access.backend()->execSql(QString("INSERT INTO Images (dirid, name, caption) \n "
                                      "VALUES(?, ?, ?);"),
                              dirid, name, caption, 0, &id);

    return id.toInt();
}


bool upgradeDB_Sqlite2ToSqlite3(DatabaseAccess &access, const QString& sql2DBPath)
{
    QString libraryPath = QDir::cleanPath(sql2DBPath);

    /*
    QString newDB= libraryPath + "/digikam3.db";

    #ifdef NFS_HACK
    newDB = locateLocal("appdata", KIO::encodeFileName(QDir::cleanPath(newDB)));
    DDebug(50003) << "NFS: " << newDB << endl;
    #endif

    AlbumDB db3;
    access.db()->setDBPath(newDB);
    if (!access.db()->isValid())
    {
        DWarning() << "Failed to open new Album Database" << endl;
        return false;
    }
    */

    //DatabaseAccess access;

    if (access.db()->getSetting("UpgradedFromSqlite2") == "yes")
        return true;

    QString dbPath = libraryPath + "/digikam3.db";

    /*
    #ifdef NFS_HACK
    dbPath = locateLocal("appdata", KIO::encodeFileName(QDir::cleanPath(dbPath)));
    DDebug(50003) << "From NFS: " << dbPath << endl;
    #endif
    */

    QFileInfo fi(dbPath);

    if (!fi.exists())
    {
        DDebug(50003) << "No old database present. Not upgrading" << endl;
        access.db()->setSetting("UpgradedFromSqlite2", "yes");
        return true;
    }

    AlbumDB_Sqlite2 db2;
    db2.setDBPath( dbPath );
    if (!db2.isValid())
    {
        DDebug(50003) << "Failed to initialize Old Album Database" << endl;
        return false;
    }

    // delete entries from sqlite3 database
    access.backend()->execSql("DELETE FROM Albums;");
    access.backend()->execSql("DELETE FROM Tags;");
    access.backend()->execSql("DELETE FROM TagsTree;");
    access.backend()->execSql("DELETE FROM Images;");
    access.backend()->execSql("DELETE FROM ImageTags;");
    access.backend()->execSql("DELETE FROM ImageProperties;");

    QStringList values;

    // update albums -------------------------------------------------

    values.clear();
    db2.execSql("SELECT id, url, date, caption, collection, icon FROM Albums;",
                &values);

    typedef QList<_Album> AlbumList;
    AlbumList albumList;

    typedef QMap<QString, int> AlbumMap;
    AlbumMap albumMap;

    access.backend()->beginTransaction();
    for (QStringList::iterator it=values.begin(); it!=values.end();)
    {
        _Album album;

        album.id     = (*it).toInt();
        ++it;
        album.url    = (*it);
        ++it;
        album.date   = (*it);
        ++it;
        album.caption    = (*it);
        ++it;
        album.collection = (*it);
        ++it;
        album.icon   = (*it);
        ++it;

        albumList.append(album);
        albumMap.insert(album.url, album.id);

        QList<QVariant> boundValues;
        boundValues << album.id << album.url << album.date << album.caption << album.collection;

        access.backend()->execSql(QString("INSERT INTO Albums (id, url, date, caption, collection) "
                                          "VALUES(?, ?, ?, ?, ?);"),
                                  boundValues);
    }
    access.backend()->commitTransaction();

    // update tags -------------------------------------------------

    values.clear();
    db2.execSql("SELECT id, pid, name, icon FROM Tags;",
                &values);

    typedef QList<_Tag> TagList;
    TagList tagList;

    access.backend()->beginTransaction();
    for (QStringList::iterator it=values.begin(); it!=values.end();)
    {
        _Tag tag;

        tag.id   = (*it).toInt();
        ++it;
        tag.pid  = (*it).toInt();
        ++it;
        tag.name = (*it);
        ++it;
        tag.icon = (*it);
        ++it;

        tagList.append(tag);

        access.backend()->execSql(QString("INSERT INTO Tags (id, pid, name) "
                                          "VALUES(?, ?, ?);"),
                                  tag.id, tag.pid, tag.name);
    }
    access.backend()->commitTransaction();

    // update images -------------------------------------------------

    values.clear();
    db2.execSql("SELECT dirid, name, caption FROM Images;",
                &values);

    access.backend()->beginTransaction();
    for (QStringList::iterator it=values.begin(); it!=values.end();)
    {
        int dirid   = (*it).toInt();
        ++it;
        QString name    = (*it);
        ++it;
        QString caption = (*it);
        ++it;

        findOrAddImage(access, dirid, name, caption);
    }
    access.backend()->commitTransaction();

    // update imagetags -----------------------------------------------

    values.clear();
    db2.execSql("SELECT dirid, name, tagid FROM ImageTags;",
                &values);
    access.backend()->beginTransaction();
    for (QStringList::iterator it=values.begin(); it!=values.end();)
    {
        int dirid = (*it).toInt();
        ++it;

        QString name = (*it);
        ++it;

        int tagid = (*it).toInt();
        ++it;

        qlonglong imageid = findOrAddImage(access, dirid, name, QString());

        access.backend()->execSql(QString("INSERT INTO ImageTags VALUES(?, ?)"),
                                  imageid, tagid);
    }
    access.backend()->commitTransaction();

    // update album icons -------------------------------------------------

    access.backend()->beginTransaction();
    for (AlbumList::iterator it = albumList.begin(); it != albumList.end();
         ++it)
    {
        _Album album = *it;

        if (album.icon.isEmpty())
            continue;

        qlonglong imageid = findOrAddImage(access, album.id, album.icon, QString());

        access.backend()->execSql(QString("UPDATE Albums SET icon=? WHERE id=?"),
                                  imageid, album.id);
    }
    access.backend()->commitTransaction();

    // -- update tag icons ---------------------------------------------------

    access.backend()->beginTransaction();
    for (TagList::iterator it = tagList.begin(); it != tagList.end(); ++it)
    {
        _Tag tag = *it;

        if (tag.icon.isEmpty())
            continue;

        QFileInfo fi(tag.icon);
        if (fi.isRelative())
        {
            access.backend()->execSql(QString("UPDATE Tags SET iconkde=? WHERE id=?"),
                                      tag.icon, tag.id);
            continue;
        }

        tag.icon = QDir::cleanPath(tag.icon);
        fi.setFile(tag.icon.remove(libraryPath));

        QString url  = fi.absolutePath();
        QString name = fi.fileName();

        AlbumMap::iterator it1 = albumMap.find(url);
        if (it1 == albumMap.end())
        {
            DDebug(50003) << "Could not find album with url: " << url << endl;
            DDebug(50003) << "Most likely an external directory. Rejecting." << endl;
            continue;
        }

        int dirid = it1.value();

        qlonglong imageid = findOrAddImage(access, dirid, name, QString());;

        access.backend()->execSql(QString("UPDATE Tags SET icon=? WHERE id=?"),
                                  imageid, tag.id);

    }
    access.backend()->commitTransaction();

    // -- Remove invalid entries ----------------------------------------
    access.backend()->execSql("DELETE FROM Images WHERE dirid=-1");

    // -- update setting entry ------------------------------------------
    access.db()->setSetting("UpgradedFromSqlite2", "yes");

    DDebug(50003) << "Successfully upgraded database to sqlite3 " << endl;

    // -- Check for db consistency ----------------------------------------

    /*
    DDebug(50003) << "Checking database consistency" << endl;


    DDebug(50003) << "Checking Albums.................." << endl;
    values.clear();
    db2.execSql("SELECT id, url, date, caption, collection FROM Albums;", &values);
    for (QStringList::iterator it = values.begin(); it != values.end();)
    {
        _Album album;
        album.id     = (*it).toInt();
        ++it;
        album.url    = (*it);
        ++it;
        album.date   = (*it);
        ++it;
        album.caption    = (*it);
        ++it;
        album.collection = (*it);
        ++it;

        QStringList list;
        access.backend()->execSql(QString("SELECT id FROM Albums WHERE \n"
                "    id=%1 AND \n"
                "    url='%2' AND \n"
                "    date='%3' AND \n"
                "    caption='%4' AND \n"
                "    collection='%5';")
                .arg(album.id)
                .arg(access.backend()->escapeString(album.url))
                .arg(access.backend()->escapeString(album.date))
                .arg(access.backend()->escapeString(album.caption))
                .arg(access.backend()->escapeString(album.collection)), &list, false);
        if (list.size() != 1)
        {
            DError() << "Failed" << endl;
            DWarning() << "" << endl;
            DWarning() << "Consistency check failed for Album: "
                       << album.url << endl;
            return false;
        }
    }
    DDebug(50003) << " (" << values.count()/5 << " Albums) "  << "OK" << endl;


    DDebug(50003) << "Checking Tags...................." << endl;
    values.clear();
    db2.execSql("SELECT id, pid, name FROM Tags;", &values);
    for (QStringList::iterator it = values.begin(); it != values.end();)
    {
        int id       = (*it).toInt();
        ++it;
        int pid      = (*it).toInt();
        ++it;
        QString name = (*it);
        ++it;

        QStringList list;
        access.backend()->execSql(QString("SELECT id FROM Tags WHERE \n"
                "    id=%1 AND \n"
                "    pid=%2 AND \n"
                "    name='%3';")
                .arg(id)
                .arg(pid)
                .arg(access.backend()->escapeString(name)),
        &list, false);
        if (list.size() != 1)
        {
            DError() << "Failed" << endl;
            DWarning() << "" << endl;
            DWarning() << "Consistency check failed for Tag: "
                       << name << endl;
            return false;
        }
    }
    DDebug(50003) << " (" << values.count()/3 << " Tags) "  << "OK" << endl;


    DDebug(50003) << "Checking Images.................." << endl;
    values.clear();
    db2.execSql("SELECT Albums.url, Images.name, Images.caption "
            "FROM Images, Albums WHERE Albums.id=Images.dirid;", &values);
    for (QStringList::iterator it = values.begin(); it != values.end();)
    {
        QString url  = (*it);
        ++it;
        QString name = (*it);
        ++it;
        QString caption = (*it);
        ++it;

        QStringList list;
        access.backend()->execSql(QString("SELECT Images.id FROM Images, Albums WHERE \n "
                "Albums.url = '%1' AND \n "
                "Images.dirid = Albums.id AND \n "
                "Images.name = '%2' AND \n "
                "Images.caption = '%3';")
                .arg(access.backend()->escapeString(url))
                .arg(access.backend()->escapeString(name))
                .arg(access.backend()->escapeString(caption)),
        &list, false);
        if (list.size() != 1)
        {
            DError() << "Failed" << endl;
            DWarning() << "" << endl;
            DWarning() << "Consistency check failed for Image: "
                       << url << ", " << name << ", " << caption  << endl;
            return false;
        }
    }
    DDebug(50003) << " (" << values.count()/3 << " Images) " << "OK" << endl;


    DDebug(50003) << "Checking ImageTags..............." << endl;
    values.clear();
    db2.execSql("SELECT Albums.url, ImageTags.name, ImageTags.tagid "
            "FROM ImageTags, Albums WHERE \n "
            "   Albums.id=ImageTags.dirid;", &values);
    for (QStringList::iterator it = values.begin(); it != values.end();)
    {
        QString url   = (*it);
        ++it;
        QString name  = (*it);
        ++it;
        int tagid = (*it).toInt();
        ++it;

        QStringList list;
        access.backend()->execSql(QString("SELECT Images.id FROM Albums, Images, ImageTags WHERE \n "
                "Albums.url = '%1' AND \n "
                "Images.dirid = Albums.id AND \n "
                "Images.name = '%2' AND \n "
                "ImageTags.imageid = Images.id AND \n "
                "ImageTags.tagid = %3;")
                .arg(access.backend()->escapeString(url))
                .arg(access.backend()->escapeString(name))
                .arg(tagid),
        &list, false);
        if (list.size() != 1)
        {
            DError() << "Failed" << endl;
            DWarning() << "" << endl;
            DWarning() << "Consistency check failed for ImageTag: "
                       << url << ", " << name << ", " << tagid << endl;
            return false;
        }
    }
    DDebug(50003) << " (" << values.count()/3 << " ImageTags) " << "OK" << endl;

    DDebug(50003) << "Checking Album icons ..............." << endl;
    values.clear();
    db2.execSql("SELECT url, icon FROM Albums;", &values);
    for (QStringList::iterator it = values.begin(); it != values.end();)
    {
        QString url    = (*it);
        ++it;
        QString icon   = (*it);
        ++it;

        if (icon.isEmpty())
            continue;

        QStringList list;
        access.backend()->execSql(QString("SELECT Images.id FROM Images, Albums WHERE \n "
                "Albums.url = '%1' AND \n "
                "Images.id = Albums.icon AND \n "
                "Images.name = '%2';")
                .arg(access.backend()->escapeString(url))
                .arg(access.backend()->escapeString(icon)), &list);

        if (list.size() != 1)
        {
            DError() << "Failed" << endl;
            DWarning() << "" << endl;
            DWarning() << "Consistency check failed for Album Icon: "
                       << url << ", " << icon << endl;

            return false;
        }
    }
    DDebug(50003) << " (" << values.count()/2 << " Album Icons) " << "OK" << endl;


    DDebug(50003) << "Checking Tag icons ..............." << endl;
    values.clear();
    db2.execSql("SELECT id, icon FROM Tags;", &values);
    for (QStringList::iterator it = values.begin(); it != values.end();)
    {
        int id       = (*it).toInt();
        ++it;
        QString icon = (*it);
        ++it;

        if (icon.isEmpty())
            continue;

        if (!icon.startsWith("/"))
        {
            QStringList list;
            access.backend()->execSql(QString("SELECT id FROM Tags WHERE \n "
                    "id = %1 AND \n "
                    "iconkde = '%2';")
                    .arg(id)
                    .arg(access.backend()->escapeString(icon)), &list);

            if (list.size() != 1)
            {
                DError() << "Failed" << endl;
                DWarning() << "" << endl;
                DWarning() << "Consistency check failed for Tag Icon: "
                           << id << ", " << icon << endl;

                return false;
            }
        }
        else
        {
            icon = QDir::cleanPath(icon);
            QFileInfo fi(icon.remove(libraryPath));

            QString url  = fi.absolutePath();
            QString name = fi.fileName();

            QStringList list;

            list.clear();
            access.backend()->execSql(QString("SELECT id FROM Albums WHERE url='%1'")
                    .arg(access.backend()->escapeString(url)), &list);
            if (list.isEmpty())
            {
                DWarning() << "Tag icon not in Album Library Path, Rejecting " << endl;
                DWarning() << "(" << icon << ")" << endl;
                continue;
            }

            list.clear();
            access.backend()->execSql(QString("SELECT Images.id FROM Images, Tags WHERE \n "
                    " Images.dirid=(SELECT id FROM Albums WHERE url='%1') AND \n "
                    " Images.name='%2' AND \n "
                    " Tags.id=%3 AND \n "
                    " Tags.icon=Images.id")
                    .arg(access.backend()->escapeString(url))
                    .arg(access.backend()->escapeString(name))
                    .arg(id), &list);
            if (list.size() != 1)
            {
                DError() << "Failed." << endl;
                DWarning() << "" << endl;
                DWarning() << "Consistency check failed for Tag Icon: "
                           << id << ", " << icon << endl;

                return false;
            }

        }
    }
    DDebug(50003) << " (" << values.count()/2 << " Tag Icons) " << "OK" << endl;

    DDebug(50003) << "" << endl;
    DDebug(50003) << "All Tests: A-OK" << endl;
    */

    return true;
}

}  // namespace Digikam
