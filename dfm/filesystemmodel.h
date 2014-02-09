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

class ThumbsLoader;
class ImagesThread;
class ViewContainer;

namespace FS {class Model;}
class FileIconProvider : public QFileIconProvider
{
public:
    inline explicit FileIconProvider(FS::Model *model = 0) : QFileIconProvider(), m_fsModel(model){}
    QIcon icon(const QFileInfo &info) const;

private:
    FS::Model *m_fsModel;
};

namespace FS
{

class Model;
namespace Worker {class Gatherer;}

class Node;
class Model : public QAbstractItemModel
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
    enum History { Back, Forward };

    explicit Model(QObject *parent = 0);
    ~Model();

    static bool hasThumb(const QString &file);

    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent) const { return 4; }

    Node *nodeFromIndex(const QModelIndex &index) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    QModelIndex index(const QUrl &url);

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

    QUrl rootUrl() { return m_url; }
    Node *rootNode();

    void forceEmitDataChangedFor(const QString &file);

    inline FileIconProvider *iconProvider() { return m_ip; }

    QIcon fileIcon(const QModelIndex &index) const;
    QString fileName(const QModelIndex &index) const;
    QUrl url(const QModelIndex &index) const;
    QUrl localUrl(const QModelIndex &index) const;

    bool isDir(const QModelIndex &index) const;
    QFileInfo fileInfo(const QModelIndex &index) const;
    QModelIndex mkdir(const QModelIndex &parent, const QString &name);

    inline Worker::Gatherer *dataGatherer() { return m_dataGatherer; }
    inline QFileSystemWatcher *dirWatcher() { return m_watcher; }
    inline FileIconProvider *ip() { return m_ip; }

    void getSort(const QUrl &url);
    void setSort(const int sortColumn, const int sortOrder);
    inline Qt::SortOrder sortOrder() const { return m_sortOrder; }
    inline int sortColumn() const { return m_sortColumn; }

    void setHiddenVisible(bool visible);
    inline bool showHidden() const { return m_showHidden; }
    bool isWorking() const;

    void goBack();
    void goForward();
    inline bool canGoBack() const { return m_history[Back].count()>1; }
    inline bool canGoForward() const { return !m_history[Forward].isEmpty(); }

    void search(const QString &fileName, const QString &filePath);
    void search(const QString &fileName);
    QString currentSearchString();

    inline QMenu *schemes() { return m_schemeMenu; }

    QString title(const QUrl &url = QUrl());

public slots:
    void setUrl(const QUrl &url);
    void setUrlFromDynamicPropertyUrl();
    void refresh();
    void endSearch();
    void cancelSearch();

private slots:
    void thumbFor(const QString &file, const QString &iconName);
    void flowDataAvailable(const QString &file);
    void dirChanged(const QString &path);
    void nodeGenerated(const QString &path, Node *node);
    void schemeFromSchemeMenu();

signals:
    void flowDataChanged(const QModelIndex &start, const QModelIndex &end);
    void directoryLoaded(const QString &path);
    void urlChanged(const QUrl &url);
    void fileRenamed(const QString &path, const QString &oldName, const QString &newName);
    void hiddenVisibilityChanged(bool visible);
    void sortingChanged(const int sortCol, const int order);
    void startedWorking();
    void finishedWorking();
    void urlLoaded(const QUrl &url);

private:
    Node *m_current;
    Node *m_rootNode;
    QAbstractItemView *m_view;
    bool m_showHidden, m_goingBack;
    FileIconProvider *m_ip;
    int m_sortColumn;
    Qt::SortOrder m_sortOrder;
    ImagesThread *m_it;
    ViewContainer *m_container;
    QFileSystemWatcher *m_watcher;
    Worker::Gatherer *m_dataGatherer;
    QMenu *m_schemeMenu;
    QUrl m_url;
    QList<QUrl> m_history[Forward+1];
    friend class ImagesThread;
    friend class Node;
    friend class Worker::Gatherer;
};

}

}

Q_DECLARE_METATYPE(QPainterPath)

#endif // FILESYSTEMMODEL_H
