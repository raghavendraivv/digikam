/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2009-02-04
 * Description : an unit-test to detect image quality level
 *
 * Copyright (C) 2011-2018 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#include "imgqsorttest.h"

// Qt includes

#include <QtTest>
#include <QStringList>
#include <QString>
#include <QFileInfoList>
#include <QDebug>

// Local includes

#include "dimg.h"
#include "previewloadthread.h"
#include "imagequalitysettings.h"
#include "imagequalityparser.h"

using namespace Digikam;

QTEST_MAIN(ImgQSortTest)

void ImgQSortTest::initTestCase()
{
}

QDir ImgQSortTest::imageDir() const
{
    QDir dir(QFINDTESTDATA("data/"));
    qDebug() << "Images Directory:" << dir;
    return dir;
}

void ImgQSortTest::testParseTestImagesForBlurDetection()
{
    QFileInfoList list = imageDir().entryInfoList(QStringList() << QLatin1String("test_blurred*.jpg"),
                                                  QDir::Files, QDir::Name);
    qDebug() << "Process images for Blur detection (" << list.size() << ")";

    int results[NumberOfPickLabels] = {0};

    foreach (QFileInfo inf, list)
    {
        QString path = inf.filePath();
        qDebug() << path;

        DImg dimg    = PreviewLoadThread::loadFastSynchronously(path, 1024);

        if (dimg.isNull())
        {
            qDebug() << path << "File cannot be loaded...";
        }

        ImageQualitySettings settings;
        settings.enableSorter       = true;
        settings.detectBlur         = true;
        settings.detectNoise        = false;
        settings.detectCompression  = false;
        settings.detectOverexposure = false;
        settings.lowQRejected       = true;
        settings.mediumQPending     = true;
        settings.highQAccepted      = true;
        settings.rejectedThreshold  = 10;
        settings.pendingThreshold   = 40;
        settings.acceptedThreshold  = 60;
        settings.blurWeight         = 100;
        settings.noiseWeight        = 100;
        settings.compressionWeight  = 100;
        settings.speed              = 1;

        PickLabel pick;
        ImageQualityParser parser (dimg, settings, &pick);
        parser.startAnalyse();

        qDebug() << "==> Blur quality result is" << pick << "(0:None, 1:Rejected, 2:Pending, 3:Accepted)";
        results[pick]++;
    }

    QVERIFY(results[NoPickLabel]   == 0);
    QVERIFY(results[RejectedLabel] == 0);
    QVERIFY(results[PendingLabel]  == 8);
    QVERIFY(results[AcceptedLabel] == 1);
}
