/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2011-08-03
 * Description : digital camera thumbnails controller
 *
 * Copyright (C) 2011 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#include "camerathumbsctrl.moc"

// Qt includes

#include <QCache>
#include <QPair>

// KDE includes

#include <kdebug.h>
#include <kurl.h>
#include <kio/previewjob.h>

// Local includes

#include "cameracontroller.h"
#include "thumbnailsize.h"

namespace Digikam
{

class CameraThumbsCtrl::CameraThumbsCtrlPriv
{

public:

    CameraThumbsCtrlPriv()
      : controller(0)
    {}

    QCache<KUrl, CameraThumbsCtrl::CacheItem> cache;  // Camera info/thumb cache based on item url keys.

    KUrl::List                                pendingItems;

    CameraController*                         controller;
};

// --------------------------------------------------------

CameraThumbsCtrl::CameraThumbsCtrl(CameraController* ctrl, QObject* parent)
    : QObject(parent), d(new CameraThumbsCtrlPriv)
{
    d->controller = ctrl;

    connect(d->controller, SIGNAL(signalThumbInfo(QString, QString, CamItemInfo, QImage)),
            this, SLOT(slotThumbInfo(QString, QString, CamItemInfo, QImage)));

    connect(d->controller, SIGNAL(signalThumbInfoFailed(QString, QString, CamItemInfo)),
            this, SLOT(slotThumbInfoFailed(QString, QString, CamItemInfo)));

    setCacheSize(200);
}

CameraThumbsCtrl::~CameraThumbsCtrl()
{
    clearCache();
    delete d;
}

void CameraThumbsCtrl::getThumbsInfo(const CamItemInfoList& list)
{
    CamItemInfoList toProcess;

    foreach (CamItemInfo info, list)
    {
        // We look if items are not in cache.

        if (hasItemFromCache(info.url()))
        {
            const CacheItem* item = retrieveItemFromCache(info.url());
            emit signalThumbInfo(item->first, item->second.toImage());
            // kDebug() << "Found in cache: " << info.url();
        }

        // We look if items are not in pending list.

        else if (!d->pendingItems.contains(info.url()))
        {
            toProcess.append(info);
            d->pendingItems << info.url();
            // kDebug() << "Request thumbs from camera : " << info.url();
        }
    }

    // Finally, We get items from camera device...

    if (!toProcess.isEmpty())
    {
        d->controller->getThumbsInfo(toProcess);
    }
}

void CameraThumbsCtrl::slotThumbInfo(const QString&, const QString& file, const CamItemInfo& info, const QImage& thumb)
{
    QImage thumbnail = thumb;

    if (thumb.isNull())
    {
        thumbnail = d->controller->mimeTypeThumbnail(file).toImage();
    }

    emit signalThumbInfo(info, thumbnail);
    putItemToCache(info.url(), info, QPixmap::fromImage(thumbnail));
    d->pendingItems.removeAll(info.url());
}

void CameraThumbsCtrl::slotThumbInfoFailed(const QString& folder, const QString& file, const CamItemInfo& info)
{
    if (d->controller->cameraDriverType() == DKCamera::UMSDriver)
    {
        putItemToCache(info.url(), info, QPixmap());
        emit signalInfo(folder, file, info);
        startKdePreviewJob(info.url());
    }
    else
    {
        QPixmap pix = d->controller->mimeTypeThumbnail(file);
        putItemToCache(info.url(), info, pix);
        emit signalThumbInfo(info, pix.toImage());
        d->pendingItems.removeAll(info.url());
    }
}

void CameraThumbsCtrl::startKdePreviewJob(const KUrl& url)
{

    KIO::PreviewJob* job = KIO::filePreview(KUrl::List() << url, ThumbnailSize::Huge);

    connect(job, SIGNAL(gotPreview(KFileItem, QPixmap)),
            this, SLOT(slotGotKDEPreview(KFileItem, QPixmap)));

    connect(job, SIGNAL(failed(KFileItem)),
            this, SLOT(slotFailedKDEPreview(KFileItem)));

    kDebug() << "pending thumbs from KDE Preview : " << url;
}

void CameraThumbsCtrl::slotGotKDEPreview(const KFileItem& item, const QPixmap& pix)
{
    procressKDEPreview(item, pix);
}

void CameraThumbsCtrl::slotFailedKDEPreview(const KFileItem& item)
{
    procressKDEPreview(item);
}

void CameraThumbsCtrl::procressKDEPreview(const KFileItem& item, const QPixmap& pix)
{
    QString file   = item.url().fileName();
    QString folder = item.url().toLocalFile().remove(QString("/") + file);
    QImage thumb;

    if (pix.isNull())
    {
        // This call must be run outside Camera Controller thread.
        thumb = d->controller->mimeTypeThumbnail(file).toImage();
        kDebug() << "Failed thumb from KDE Preview : " << item.url();
    }
    else
    {
        thumb = pix.toImage();
        kDebug() << "Got thumb from KDE Preview : " << item.url();
    }

    const CacheItem* cit = retrieveItemFromCache(item.url());
    putItemToCache(item.url(), cit->first, QPixmap::fromImage(thumb));
    emit signalThumb(folder, file, thumb);
    d->pendingItems.removeAll(item.url());
}

const CameraThumbsCtrl::CacheItem* CameraThumbsCtrl::retrieveItemFromCache(const KUrl& url) const
{
    return d->cache[url];
}

bool CameraThumbsCtrl::hasItemFromCache(const KUrl& url) const
{
    return d->cache.contains(url);
}

void CameraThumbsCtrl::putItemToCache(const KUrl& url, const CamItemInfo& info, const QPixmap& thumb)
{
    int infoCost = sizeof(info);
    int thumbCost = thumb.width() * thumb.height() * thumb.depth() / 8;
    d->cache.insert(url,
                    new CacheItem(info, thumb),
                    infoCost + thumbCost);
}

void CameraThumbsCtrl::removeItemFromCache(const KUrl& url)
{
    d->cache.remove(url);
}

void CameraThumbsCtrl::clearCache()
{
    d->cache.clear();
}

// NOTE: Marcel, how to compute cost for CamItemInfo container. I set 2 Kb : it's fine ?

void CameraThumbsCtrl::setCacheSize(int numberOfItems)
{
    d->cache.setMaxCost( (numberOfItems * 256 * 256 * QPixmap::defaultDepth() / 8) +
                         (numberOfItems * 1024 * 2));
}

}  // namespace Digikam
