/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2009-03-23
 * Description : Qt Model for Albums - abstract base classes
 *
 * Copyright (C) 2008-2009 by Marcel Wiesweg <marcel dot wiesweg at gmx dot de>
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

#ifndef ABSTRACTALBUMMODEL_H
#define ABSTRACTALBUMMODEL_H

// Qt includes

#include <QAbstractItemModel>
#include <QHash>
#include <QPixmap>
#include <QSet>

// Local includes

#include "album.h"

namespace Digikam
{

class Album;
class AlbumManager;
class AlbumModelDragDropHandler;
class AlbumModelPriv;

class AbstractAlbumModel : public QAbstractItemModel
{
    Q_OBJECT

public:

    /**
     *  AbstractAlbumModel is the abstract base class for all models that
     *  present Album objects as managed by AlbumManager.
     *  You will want to create an instance of the base classes.
     */

    enum RootAlbumBehavior
    {
        /** The root album will be included as a single parent item
            with all top-level album as children */
        IncludeRootAlbum,
        /** The root album will not be included, but all top-level album
            are represented as top-level items in this view */
        IgnoreRootAlbum
    };

    enum AlbumDataRole
    {
        /// Returns the album title. Principally the same as display role, but without any additions.
        AlbumTitleRole = Qt::UserRole,
        /// Returns the Album::Type of the associated album
        AlbumTypeRole = Qt::UserRole + 1,
        /// Returns a pointer to the associated Album object
        AlbumPointerRole = Qt::UserRole + 2,
        /// Returns the id of the associated Album object
        AlbumIdRole = Qt::UserRole + 3,
        /// Returns the global id (unique across all album types)
        AlbumGlobalIdRole = Qt::UserRole + 4
    };

    /**
     * Create an AbstractAlbumModel object for albums with the given type.
     * Pass the root album if it is already available.
     * Do not use this class directly, but one of the subclasses.
     */
    AbstractAlbumModel(Album::Type albumType, Album *rootAlbum, RootAlbumBehavior rootBehavior = IncludeRootAlbum,
                       QObject *parent = 0);
    ~AbstractAlbumModel();

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual Qt::ItemFlags flags(const QModelIndex& index) const;
    virtual bool hasChildren(const QModelIndex& parent = QModelIndex()) const;
    virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex& index) const;

    virtual Qt::DropActions supportedDropActions() const;
    virtual QStringList mimeTypes() const;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex& parent);
    virtual QMimeData * mimeData(const QModelIndexList& indexes) const;

    /// Set a drag drop handler.
    void setDragDropHandler(AlbumModelDragDropHandler *handler);

    /// Returns the drag drop handler, or 0 if none is installed
    AlbumModelDragDropHandler *dragDropHandler() const;

    /** Returns the album object associated with the given model index */
    Album *albumForIndex(const QModelIndex& index) const;
    /** Return the QModelIndex for the given album, or an invalid index if
        the album is not contained in this model. */
    QModelIndex indexForAlbum(Album *album) const;

    Album *rootAlbum() const;
    /// Return the index corresponding to the root album. If the policy is IgnoreRootAlbum, this is an invalid index. */
    QModelIndex rootAlbumIndex() const;

    /// Returns the Album::Type of the contained albums
    Album::Type albumType() const;

Q_SIGNALS:

    /** This is initialized once after creation, if the root album becomes available,
     *  if it was not already available at time of construction.
     *  This is emitted regardless of root album policy. */
    void rootAlbumAvailable();

protected:

    // these can be reimplemented in a subclass
    /// For subclassing convenience: A part of the implementation of data()
    virtual QVariant albumData(Album *a, int role) const;
    /// For subclassing convenience: A part of the implementation of data()
    virtual QVariant decorationRole(Album *a) const;
    /// For subclassing convenience: A part of the implementation of headerData()
    virtual QString columnHeader() const;
    /// For subclassing convenience: A part of the implementation of itemFlags()
    virtual Qt::ItemFlags itemFlags(Album *album) const;
    /** Returns true for those and only those albums that shall be contained in this model.
     *  They must have a common root album, which is set in the constructor. */
    virtual bool filterAlbum(Album *album) const;
    /// Notification when an entry is removed
    virtual void albumCleared(Album* /*album*/) {};
    /// Notification when all entries are removed
    virtual void allAlbumsCleared() {};

    /** Switch on drag and drop globally for all items. Default is true.
     *  For per-item cases reimplement itemFlags(). */
    void setEnableDrag(bool enable);
    void setEnableDrop(bool enable);

protected Q_SLOTS:

    void slotAlbumAboutToBeAdded(Album *album, Album *parent, Album *prev);
    void slotAlbumAdded(Album *);
    void slotAlbumAboutToBeDeleted(Album *album);
    void slotAlbumHasBeenDeleted(void *);
    void slotAlbumsCleared();
    void slotAlbumIconChanged(Album* album);
    void slotAlbumRenamed(Album *album);

private:

    AlbumModelPriv* const d;
};

// ------------------------------------------------------------------

class AbstractSpecificAlbumModel : public AbstractAlbumModel
{
    Q_OBJECT

public:

    /// Abstract base class, do not instantiate.
    AbstractSpecificAlbumModel(Album::Type albumType, Album *rootAlbum,
                               RootAlbumBehavior rootBehavior = IncludeRootAlbum,
                               QObject *parent = 0);

protected:

    virtual QString  columnHeader() const;
    void setColumnHeader(const QString& header);

    /// You need to call this from your constructor if you intend to load the thumbnail facilities of this class
    void setupThumbnailLoading();
    void emitDataChangedForChildren(Album *album);

protected Q_SLOTS:

    void slotGotThumbnailFromIcon(Album *album, const QPixmap& thumbnail);
    void slotThumbnailLost(Album *album);
    void slotReloadThumbnails();

protected:

    QString m_columnHeader;
};

// ------------------------------------------------------------------

class AbstractCountingAlbumModel : public AbstractSpecificAlbumModel
{
    Q_OBJECT

public:

    /// Supports displaying a count alongside the album name in DisplayRole

    AbstractCountingAlbumModel(Album::Type albumType, Album *rootAlbum,
                               RootAlbumBehavior rootBehavior = IncludeRootAlbum,
                               QObject *parent = 0);

public Q_SLOTS:

    /// Call to enable or disable showing the count. Default is false.
    void setShowCount(bool show);
    bool showCount() const;

    /** Enable displaying the count. Set a map of album id -> count (excluding children).
     *  If an album is not contained, no count is displayed. To display a count of 0,
     *  there must be an entry album id -> 0. */
    void setCountMap(const QMap<int, int>& idCountMap);

    /** Displays only the count of the album, without adding child albums' counts.
     *  This is the default.
     *  Can connect to QTreeView's expanded() signal. */
    void excludeChildrenCount(const QModelIndex& index);
    /** Displays sum of the count of the album and child albums' counts.
     *  Can connect to QTreeView's collapsed() signal. */
    void includeChildrenCount(const QModelIndex& index);

    /**
     * Returns the number of included items for this album.
     *
     * @return positive value or -1 if unknown
     */
    virtual int albumCount(Album *album) const;

protected:

    /// need to implement in subclass
    virtual Album* albumForId(int id) const = 0;

    /// Can reimplement in subclass
    virtual QString albumName(Album *a) const;

    // Reimplemented from parent classes
    virtual QVariant albumData(Album *a, int role) const;
    virtual void albumCleared(Album *album);
    virtual void allAlbumsCleared();

    /// If you do not use setCountMap, excludeChildrenCount and includeChildrenCount, you can set a count here.
    void setCount(Album *album, int count);

private:

    void updateCount(Album *album);

private:

    bool            m_showCount;
    QMap<int, int>  m_countMap;
    QHash<int, int> m_countHashReady;
    QSet<int>       m_includeChildrenAlbums;
};

// ------------------------------------------------------------------

class AbstractCheckableAlbumModel : public AbstractCountingAlbumModel
{
    Q_OBJECT

public:

    /// Abstract base class that manages the check state of Albums.
    /// Call setCheckable(true) to enable checkable albums.

    AbstractCheckableAlbumModel(Album::Type albumType, Album *rootAlbum,
                                RootAlbumBehavior rootBehavior = IncludeRootAlbum,
                                QObject *parent = 0);

    /// Triggers if the albums in this model are checkable
    void setCheckable(bool isCheckable);
    bool isCheckable() const;
    /** Triggers if the albums in this model are tristate.
     *  Note that you want to set setCheckable(true) before. */
    void setTristate(bool isTristate);
    bool isTristate() const;

    /// Returns if the given album has the check state Checked
    bool isChecked(Album *album) const;
    /// Returns the check state of the album
    Qt::CheckState checkState(Album *album) const;

    /// Sets the check state of album to Checked or Unchecked
    void setChecked(Album *album, bool isChecked);
    /// Sets the check state of the album
    void setCheckState(Album *album, Qt::CheckState state);
    /// Toggles the check state of album between Checked or Unchecked
    void toggleChecked(Album *album);

    /// Returns a list of album with check state Checked
    QList<Album *> checkedAlbums() const;

    /// Resets the checked state of all albums to Qt::Unchecked
    void resetCheckedAlbums();

Q_SIGNALS:

    /** Emitted when the check state of an album changes.
     *  checkState contains the new Qt::CheckState of album */
    void checkStateChanged(Album *album, Qt::CheckState checkState);

protected:

    virtual QVariant albumData(Album *a, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex& index) const;
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole );

    virtual void albumCleared(Album *album);
    virtual void allAlbumsCleared();

private:

    Qt::ItemFlags                 m_extraFlags;
    QHash<Album*, Qt::CheckState> m_checkedAlbums;
};

} // namespace Digikam

#endif // ABSTRACTALBUMMODEL_H
