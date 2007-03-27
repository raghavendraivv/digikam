/* ============================================================
 * Authors: Gilles Caulier <caulier dot gilles at gmail dot com>
 * Date   : 2007-12-03
 * Description : Greycstoration interface.
 *
 * Copyright 2007 by Gilles Caulier
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

// C++ includes.

#include <cassert>

// Local includes.

#define cimg_plugin "greycstoration.h"
#if cimg_OS!=2
#include <pthread.h>
#endif
#include "CImg.h"
using namespace cimg_library;

// Local includes.

#include "ddebug.h"
#include "greycstorationsettings.h"
#include "greycstorationiface.h"

namespace Digikam
{

class GreycstorationIfacePriv
{

public:

    GreycstorationIfacePriv()
    {
        mode = GreycstorationIface::Restore;
    }

    int                    mode;

    QImage                 inPaintingMask;

    GreycstorationSettings settings;

    CImg<>                 img;
    CImg<uchar>            mask;
};

GreycstorationIface::GreycstorationIface(DImg *orgImage,
                                         GreycstorationSettings settings,
                                         int mode,
                                         int newWidth, int newHeight,
                                         const QImage& inPaintingMask,
                                         QObject *parent)
                   : DImgThreadedFilter(orgImage, parent)
{
    d = new GreycstorationIfacePriv;
    d->settings       = settings;
    d->mode           = mode;
    d->inPaintingMask = inPaintingMask;

    if (d->mode == Resize || d->mode == SimpleResize)
    {
        m_destImage = Digikam::DImg(newWidth, newHeight,
                                    m_orgImage.sixteenBit(), m_orgImage.hasAlpha());
        DDebug() << "GreycstorationIface::Resize: new size: ("
                 << newWidth << ", " << newHeight << ")" << endl;
    }
    else
    {
        m_destImage = Digikam::DImg(m_orgImage.width(), m_orgImage.height(),
                                    m_orgImage.sixteenBit(), m_orgImage.hasAlpha());
    }

    initFilter();
}

GreycstorationIface::~GreycstorationIface()
{
    delete d;
}

// We need to re-implemente this method from DImgThreadedFilter class because
// target image size can be different from original if m_resize is enable.

void GreycstorationIface::initFilter()
{
    if (m_orgImage.width() && m_orgImage.height())
    {
        if (m_parent)
            start();             // m_parent is valide, start thread ==> run()
        else
            startComputation();  // no parent : no using thread.
    }
    else  // No image data
    {
        if (m_parent)           // If parent then send event about a problem.
        {
            postProgress(0, false, false);
            DDebug() << m_name << "::No valid image data !!! ..." << endl;
        }
    }
}

void GreycstorationIface::stopComputation()
{
    // Because Greycstoration algorithm run in a child thread, we need
    // to stop it before to stop this thread.
    if (d->img.greycstoration_is_running())
    {
        // If the user abort, we stop the algorithm.
        DDebug() << "Stop Greycstoration computation..." << endl;

        d->img.greycstoration_stop();
    }

    Digikam::DImgThreadedFilter::stopComputation();
}

void GreycstorationIface::cleanupFilter()
{
    d->img  = CImg<>();
    d->mask = CImg<uchar>();
}

void GreycstorationIface::filterImage()
{
    register int x, y;

    DDebug() << "GreycstorationIface::Initialization..." << endl;

    // Copy the src image data into a CImg type image with three channels and no alpha.

    uchar* imageData = m_orgImage.bits();
    int imageWidth   = m_orgImage.width();
    int imageHeight  = m_orgImage.height();

    d->img = CImg<>(imageWidth, imageHeight, 1, 4);

    if (!m_orgImage.sixteenBit())           // 8 bits image.
    {
        uchar *ptr = imageData;

        for (y = 0; y < imageHeight; y++)
        {
            for (x = 0; x < imageWidth; x++)
            {
                d->img(x, y, 0) = ptr[0];        // Blue.
                d->img(x, y, 1) = ptr[1];        // Green.
                d->img(x, y, 2) = ptr[2];        // Red.
                d->img(x, y, 3) = ptr[3];        // Alpha.
                ptr += 4;
            }
        }
    }
    else                                // 16 bits image.
    {
        unsigned short *ptr = (unsigned short *)imageData;

        for (y = 0; y < imageHeight; y++)
        {
            for (x = 0; x < imageWidth; x++)
            {
                d->img(x, y, 0) = ptr[0];        // Blue.
                d->img(x, y, 1) = ptr[1];        // Green.
                d->img(x, y, 2) = ptr[2];        // Red.
                d->img(x, y, 3) = ptr[3];        // Alpha.
                ptr += 4;
            }
        }
    }

    DDebug() << "GreycstorationIface::Process Computation..." << endl;

    try
    {
        switch (d->mode)
        {
            case Restore:
                restoration();
                break;

            case InPainting:
                inpainting();
                break;

            case Resize:
                resize();
                break;

            case SimpleResize:
                simpleResize();
                break;
        }
    }
    catch(...)         // Everything went wrong.
    {
       DDebug() << "GreycstorationIface::Error during Greycstoration filter computation!" << endl;

       if (m_parent)
          postProgress( 0, false, false );

       return;
    }

    if (m_cancel)
        return;

    // Copy CImg onto destination.

    DDebug() << "GreycstorationIface::Finalization..." << endl;

    uchar* newData = m_destImage.bits();
    int newWidth   = m_destImage.width();
    int newHeight  = m_destImage.height();

    if (!m_orgImage.sixteenBit())           // 8 bits image.
    {
        uchar *ptr = newData;

        for (y = 0; y < newHeight; y++)
        {
            for (x = 0; x < newWidth; x++)
            {
                // Overwrite RGB values to destination.
                ptr[0] = (uchar) d->img(x, y, 0);        // Blue
                ptr[1] = (uchar) d->img(x, y, 1);        // Green
                ptr[2] = (uchar) d->img(x, y, 2);        // Red
                ptr[3] = (uchar) d->img(x, y, 3);        // Alpha
                ptr += 4;
            }
        }
    }
    else                                     // 16 bits image.
    {
        unsigned short *ptr = (unsigned short *)newData;

        for (y = 0; y < newHeight; y++)
        {
            for (x = 0; x < newWidth; x++)
            {
                // Overwrite RGB values to destination.
                ptr[0] = (unsigned short) d->img(x, y, 0);        // Blue
                ptr[1] = (unsigned short) d->img(x, y, 1);        // Green
                ptr[2] = (unsigned short) d->img(x, y, 2);        // Red
                ptr[3] = (unsigned short) d->img(x, y, 3);        // Alpha
                ptr += 4;
            }
        }
    }
}

void GreycstorationIface::restoration()
{
    for (uint iter = 0 ; !m_cancel && (iter < d->settings.nbIter) ; iter++)
    {
        // This function will start a thread running one iteration of the GREYCstoration filter.
        // It returns immediately, so you can do what you want after (update a progress bar for
        // instance).
        d->img.greycstoration_run(d->settings.amplitude,
                                  d->settings.sharpness,
                                  d->settings.anisotropy,
                                  d->settings.alpha,
                                  d->settings.sigma,
                                  d->settings.dl,
                                  d->settings.da,
                                  d->settings.gaussPrec,
                                  d->settings.interp,
                                  d->settings.fastApprox,
                                  d->settings.tile,
                                  d->settings.btile,
                                  2);                     // 2 separated threads to compute.

        iterationLoop(iter);
    }
}

void GreycstorationIface::inpainting()
{
    if (!d->inPaintingMask.isNull())
    {
        // Copy the inpainting image data into a CImg type image with three channels and no alpha.

        register int x, y;

        d->mask    = CImg<uchar>(d->inPaintingMask.width(), d->inPaintingMask.height(), 1, 3);
        uchar *ptr = d->inPaintingMask.bits();

        for (y = 0; y < d->inPaintingMask.height(); y++)
        {
            for (x = 0; x < d->inPaintingMask.width(); x++)
            {
                d->mask(x, y, 0) = ptr[2];        // blue.
                d->mask(x, y, 1) = ptr[1];        // green.
                d->mask(x, y, 2) = ptr[0];        // red.
                ptr += 4;
            }
        }
    }
    else
    {
        DDebug() << "Inpainting image: mask is null!" << endl;
        m_cancel = true;
        return;
    }

    for (uint iter=0 ; !m_cancel && (iter < d->settings.nbIter) ; iter++)
    {
        // This function will start a thread running one iteration of the GREYCstoration filter.
        // It returns immediately, so you can do what you want after (update a progress bar for
        // instance).
        d->img.greycstoration_run(d->mask,
                                  d->settings.amplitude,
                                  d->settings.sharpness,
                                  d->settings.anisotropy,
                                  d->settings.alpha,
                                  d->settings.sigma,
                                  d->settings.dl,
                                  d->settings.da,
                                  d->settings.gaussPrec,
                                  d->settings.interp,
                                  d->settings.fastApprox,
                                  d->settings.tile,
                                  d->settings.btile,
                                  2);                     // 2 separated threads to compute.
        iterationLoop(iter);
    }
}

void GreycstorationIface::resize()
{
    const bool anchor       = true;   // Anchor original pixels.
    const unsigned int init = 5;      // Initial estimate (1=block, 3=linear, 5=bicubic).

    const CImgStats stats(d->img, false);
    DDebug() << "Resizing image: size " << d->img.width << " x " << d->img.height 
             << ", value range [" << stats.min << " , " << stats.max << "]" << endl;

    int w = m_destImage.width();
    int h = m_destImage.height();
    d->mask.assign(d->img.dimx(), d->img.dimy(), 1, 1, 255);

    if (!anchor) 
        d->mask.resize(w, h, 1, 1, 1); 
    else 
        d->mask = !d->mask.resize(w, h, 1, 1, 4);

    d->img.resize(w, h, 1, -100, init);

    for (uint iter = 0 ; !m_cancel && (iter < d->settings.nbIter) ; iter++)
    {
        // This function will start a thread running one iteration of the GREYCstoration filter.
        // It returns immediately, so you can do what you want after (update a progress bar for
        // instance).
        d->img.greycstoration_run(d->mask,
                                  d->settings.amplitude,
                                  d->settings.sharpness,
                                  d->settings.anisotropy,
                                  d->settings.alpha,
                                  d->settings.sigma,
                                  d->settings.dl,
                                  d->settings.da,
                                  d->settings.gaussPrec,
                                  d->settings.interp,
                                  d->settings.fastApprox,
                                  d->settings.tile,
                                  d->settings.btile,
                                  2);                     // 2 separated threads to compute.
        iterationLoop(iter);
    }
}

void GreycstorationIface::simpleResize()
{
    const unsigned int init = 3;      // Initial estimate (1=block, 3=linear, 5=bicubic).

    int w = m_destImage.width();
    int h = m_destImage.height();

    d->img.resize(w, h, 1, -100, init);
}

void GreycstorationIface::iterationLoop(uint iter)
{
    uint mp = iter*100;

    do
    {
        if (m_parent && !m_cancel)
        {
            // Update the progress bar in dialog. We simply computes the global
            // progression indice (including all iterations).

            uint p = (uint)((iter*100 + d->img.greycstoration_progress())/d->settings.nbIter);
            if (p == mp+1)
            {
                postProgress(p);
                mp = p;
            }
        }
    }
    while (d->img.greycstoration_is_running() && !m_cancel);
}

}  // NameSpace Digikam
