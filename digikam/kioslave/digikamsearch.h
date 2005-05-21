/* ============================================================
 * Author: Renchi Raju <renchi@pooh.tam.uiuc.edu>
 * Date  : 2005-04-21
 * Copyright 2005 by Renchi Raju
 *
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
 * ============================================================ */

#ifndef DIGIKAMSEARCH_H
#define DIGIKAMSEARCH_H

#include <kio/slavebase.h>

typedef struct sqlite sqleet;
class QStringList;

class kio_digikamsearch : public KIO::SlaveBase
{
public:

    enum SKey
    {
        ALBUM = 0,
        ALBUMNAME,
        ALBUMCAPTION,
        ALBUMCOLLECTION,
        TAG,
        TAGNAME,
        IMAGENAME,
        IMAGECAPTION,
        IMAGEDATE
    };

    enum SOperator
    {
        EQ = 0,
        NE,
        LT,
        GT,
        LIKE,
        NLIKE
    };
    
    
    kio_digikamsearch(const QCString &pool_socket,
                      const QCString &app_socket);
    ~kio_digikamsearch();

    void special(const QByteArray& data);

private:

    QString miniListing(const KURL& url) const;
    QString fullListing(const KURL& url) const;
    
    void openDB();
    void closeDB();
    bool execSql(const QString& sql, QStringList* const values = 0,
                 QString* const errMsg = 0, const bool debug = false) const;
    QString escapeString(const QString& str) const;

    QString subQuery(enum SKey key, enum SOperator op, const QString& val) const;

private:
    
    class RuleType
    {
    public:

        SKey      key;
        SOperator op;
        QString   val;
    };

    mutable sqleet*       m_db;
    QString               m_libraryPath;
};


#endif /* DIGIKAMSEARCH_H */
