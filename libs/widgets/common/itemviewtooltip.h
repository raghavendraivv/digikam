/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2009-04-24
 * Description : A DItemToolTip prepared for use in QAbstractItemViews
 *
 * Copyright (C) 2009 by Marcel Wiesweg <marcel dot wiesweg at gmx dot de>
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

#ifndef ITEMVIEWTOOLTIP_H
#define ITEMVIEWTOOLTIP_H

// Qt includes

#include <QtGui/QAbstractItemView>

// KDE includes

// Local includes

#include "ditemtooltip.h"
#include "digikam_export.h"

namespace Digikam
{

class ItemViewToolTipPriv;

class DIGIKAM_EXPORT ItemViewToolTip : public DItemToolTip
{
public:

    ItemViewToolTip(QAbstractItemView *view);
    ~ItemViewToolTip();

    QAbstractItemView *view() const;
    QModelIndex currentIndex() const;

    void show(QHelpEvent *event, const QStyleOptionViewItem& option, const QModelIndex& index);

protected:

    virtual QRect repositionRect();

    bool eventFilter(QObject *o, QEvent *e);
    void hideEvent(QHideEvent*);
    void mouseMoveEvent(QMouseEvent *e);

private:

    ItemViewToolTipPriv* const d;
};

} // namespace Digikam

#endif /* ITEMVIEWTOOLTIP_H */
