/**************************************************************************
*   Copyright (C) 2013 by Robert Metsaranta                               *
*   therealestrob@gmail.com                                               *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/


#ifndef FILESYSTEMMODEL_H
#define FILESYSTEMMODEL_H

#include <QFileIconProvider>
#include <QPainter>
#include <QAbstractItemView>
#include <QDir>
#include <QFileIconProvider>
#include <QTimer>
#include <QMimeData>
#include <QSettings>
#include <QDateTime>
#include <QUrl>
#include <QLabel>
#include <QDebug>
#include <QMap>
#include <QWaitCondition>
#include <QAbstractItemModel>
#include <QMutex>

#include "thumbsloader.h"
#include "viewcontainer.h"
#include "fsworkers.h"

namespace DFM
{

class FileSystemModel;

class FileIconProvider : public QFileIconProvider
{
public:
    inline explicit FileIconProvider(FileSystemModel *model = 0) : QFileIconProvider(), m_fsModel(model){}
    QIcon icon(const QFileInfo &info) const;

private:
    FileSystemModel *m_fsModel;
};

namespace Worker { class Gatherer; }
class ThumbsLoader;
class ImagesThread;
class ViewContainer;
class FileSystemNode;
class FileSystemModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum Roles
    {
        FileIconRole = Qt::DecorationRole,
        FilePathRole = Qt::UserRole + 1,
        FileNameRole = Qt::UserRole + 2,
        FilePermissions = Qt::UserRole + 3,
        FlowImg = Qt::UserRole +4,
        FlowRefl = Qt::UserRole +5,
        FlowShape = Qt::UserRole +6,
        Category = Qt::UserRole +7
    };
    enum Scheme
    {
        File = 0,
        Search = 1,
        History =2
    };

    explicit FileSystemModel(QObject *parent = 0);
    ~FileSystemModel();

    static bool hasThumb(const QString &file);

    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent) const { return 4; }

    FileSystemNode *fromIndex(const QModelIndex &index) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    QModelIndex index(const QString &path) const;

    void fetchMore(const QModelIndex &parent);
    bool canFetchMore(const QModelIndex &parent) const;
    bool hasChildren(const QModelIndex &parent) const;
    void sort(int column, Qt::SortOrder order);

    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    QStringList mimeTypes() const { return QStringList() << "text/uri-list"; }

    QStringList categories();
    QModelIndexList category(const QString &cat);
    QModelIndexList category(const QModelIndex &fromCat);

    void setUrl(const QUrl &url);
    void setRootPath(const QString &path);
    inline QString rootPath() const { return m_rootPath; }
    FileSystemNode *rootNode();
    FileSystemNode *schemeNode(const QString &scheme);

    void forceEmitDataChangedFor(const QString &file);
    inline QDir rootDirectory() const { return QDir(m_rootPath); }

    inline FileIconProvider *iconProvider() { return m_ip; }

    inline QIcon fileIcon(const QModelIndex &index) const { return bool(index.column()==0)?data(index, Qt::DecorationRole).value<QIcon>():QIcon(); }
    QString fileName(const QModelIndex &index) const;
    QString filePath(const QModelIndex &index) const;

    bool isDir(const QModelIndex &index) const;
    QFileInfo fileInfo(const QModelIndex &index) const;
    QModelIndex mkdir(const QModelIndex &parent, const QString &name);

    inline Worker::Gatherer *dataGatherer() { return m_dataGatherer; }
    inline QFileSystemWatcher *dirWatcher() { return m_watcher; }
    inline FileIconProvider *ip() { return m_ip; }

    void setSort(const int sortColumn, const int sortOrder);
    inline Qt::SortOrder sortOrder() const { return m_sortOrder; }
    inline int sortColumn() const { return m_sortColumn; }

    void setHiddenVisible(bool visible);
    inline bool showHidden() const { return m_showHidden; }

    void startWorking();
    void endWorking();
    inline bool isWorking() const { QMutexLocker locker(&m_mutex); return m_isPopulating; }

    void search(const QString &fileName);
    QString currentSearchString();

public slots:
    inline void setPath(const QString &path) { setRootPath(path); }
    void refresh();
    void endSearch();
    void cancelSearch();

private slots:
    void thumbFor( const QString &file, const QString &iconName );
    void flowDataAvailable( const QString &file );
    void dirChanged( const QString &path );
    void emitRootPathLater() { emit rootPathChanged(rootPath()); }
    void nodeGenerated(const QString &path, FileSystemNode *node);
    void removeDeletedLater();
    void removeDeleted();

signals:
    void flowDataChanged( const QModelIndex &start, const QModelIndex &end );
    void rootPathChanged(const QString &path);
    void directoryLoaded(const QString &path);
    void fileRenamed(const QString &path, const QString &oldName, const QString &newName);
    void hiddenVisibilityChanged(bool visible);
    void sortingChanged(const int sortCol, const int order);
    void paintRequest();
    void startedWorking();
    void finishedWorking();

private:
    FileSystemNode *m_rootNode, *m_current;
    QMap<QString, FileSystemNode *> m_schemes;
    QAbstractItemView *m_view;
    bool m_showHidden, m_isPopulating;
    QString m_rootPath;
    FileIconProvider *m_ip;
    int m_sortColumn;
    Qt::SortOrder m_sortOrder;
    ImagesThread *m_it;
    ViewContainer *m_container;
    QFileSystemWatcher *m_watcher;
    Worker::Gatherer *m_dataGatherer;
    friend class ImagesThread;
    friend class FileSystemNode;
    friend class Worker::Gatherer;
    mutable QMutex m_mutex;
};

}

Q_DECLARE_METATYPE(QPainterPath)

#endif // FILESYSTEMMODEL_H
