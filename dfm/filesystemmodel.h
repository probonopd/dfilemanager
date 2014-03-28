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

#include "dataloader.h"
#include "fsworkers.h"
#include "helpers.h"
#include "globals.h"

namespace DFM
{

class DataLoader;

namespace FS
{

class FileIconProvider : public QFileIconProvider
{
public:
    FileIconProvider();
    QIcon icon(const QFileInfo &i) const;
    QIcon icon(IconType type) const;
    static QIcon fileIcon(const QFileInfo &fileInfo);
    static QIcon typeIcon(IconType type);
    static FileIconProvider *instance();
};

class Model;
namespace Worker {class Gatherer;}

class Node;
class Model : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum History { Back, Forward };

    explicit Model(QObject *parent = 0);
    ~Model();

    bool hasThumb(const QString &file);
    bool hasThumb(const QModelIndex &index);

    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    template <typename T> inline T tData(const QModelIndex &index, int role = Qt::DisplayRole) const
    {
        return data(index, role).value<T>();
    }
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const { return 4; }

    Node *nodeFromIndex(const QModelIndex &index) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    QModelIndex index(const QUrl &url);
    QModelIndex indexForLocalFile(const QString &filePath);

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

    QUrl rootUrl() const { return m_url; }
    Node *rootNode();

    QIcon fileIcon(const QModelIndex &index) const;
    QString fileName(const QModelIndex &index) const;
    QUrl url(const QModelIndex &index) const;
    QUrl localUrl(const QModelIndex &index) const;

    bool isDir(const QModelIndex &index) const;
    void exec(const QModelIndex &index);
    QFileInfo fileInfo(const QModelIndex &index) const;
    QModelIndex mkdir(const QModelIndex &parent, const QString &name);

    inline Worker::Gatherer *dataGatherer() { return m_dataGatherer; }
    inline QFileSystemWatcher *dirWatcher() { return m_watcher; }

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

    void search(const QString &fileName);
    void setFilter(const QString &setFilter);
    QString currentSearchString();

    inline QMenu *schemes() { return m_schemeMenu; }
    Node *schemeNode(const QString &scheme);

    QString title(const QUrl &url = QUrl());
    void watchDir(const QString &path);
    void unWatchDir(const QString &path);

protected:
    bool handleFileUrl(QUrl &url = defaultUrl, int &hasUrlReady = defaultInteger);
    bool handleSearchUrl(QUrl &url = defaultUrl, int &hasUrlReady = defaultInteger);
    bool handleApplicationsUrl(QUrl &url = defaultUrl, int &hasUrlReady = defaultInteger);
    bool handleDevicesUrl(QUrl &url = defaultUrl, int &hasUrlReady = defaultInteger);

public slots:
    bool setUrl(QUrl url);
    void setUrlFromDynamicPropertyUrl();
    void refresh();
    void refresh(const QString &path);
    void endSearch();
    void cancelSearch();

private slots:
    void newData(const QString &file);
    void dirChanged(const QString &path);
    void nodeGenerated(const QString &path, Node *node);
    void schemeFromSchemeMenu();
    void refreshCurrent();
    void fileDeleted(const QString &path);
    void updateFileNode();

signals:
    void flowDataChanged(const QModelIndex &start, const QModelIndex &end);
    void directoryLoaded(const QString &path);
    void directoryRemoved(const QString &path);
    void urlChanged(const QUrl &url);
    void fileRenamed(const QString &path, const QString &oldName, const QString &newName);
    void hiddenVisibilityChanged(bool visible);
    void sortingChanged(const int sortCol, const int order);
    void startedWorking();
    void finishedWorking();
    void urlLoaded(const QUrl &url);

private:
    Node *m_rootNode, *m_current;
    mutable QMap<QString, Node *> m_schemeNodes;
    QHash<QUrl, Node *> m_nodes;
    bool m_showHidden, m_lockHistory, m_isDestroyed;
    Qt::SortOrder m_sortOrder;
    int m_sortColumn;
    QFileSystemWatcher *m_watcher;
    Worker::Gatherer *m_dataGatherer;
    QMenu *m_schemeMenu;
    QUrl m_url;
    QList<QUrl> m_history[Forward+1];
    QTimer *m_timer;
    QObject *m_parent;
    friend class FlowDataLoader;
    friend class Node;
    friend class Worker::Gatherer;
};

}

}

#endif // FILESYSTEMMODEL_H
