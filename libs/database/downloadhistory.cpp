/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2007-11-01
 * Description : Access image position stored in database.
 *
 * Copyright (C) 2007 by Marcel Wiesweg <marcel dot wiesweg at gmx dot de>
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

// Qt includes

// Local includes.

#include "albumdb.h"
#include "databaseaccess.h"
#include "downloadhistory.h"

namespace Digikam
{

DownloadHistory::Status DownloadHistory::status(const QString &position, const QString &name, int fileSize, const QDateTime &date)
{
    int id = DatabaseAccess().db()->findInDownloadHistory(position, name, fileSize, date);

    if (id != -1)
        return Downloaded;
    else
        return NotDownloaded;
}

void DownloadHistory::setDownloaded(const QString &position, const QString &name, int fileSize, const QDateTime &date)
{
    DatabaseAccess().db()->addToDownloadHistory(position, name, fileSize, date);
}


} // namespace Digikam




