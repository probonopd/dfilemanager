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
#include "thumbsloader.h"
#include "viewcontainer.h"

#include <QWaitCondition>
#include <QAbstractItemModel>
#include <QMutex>

namespace DFM
{

class FileSystemModel;

class History : public QObject
{
    Q_OBJECT
public:
    explicit History(QObject *parent = 0) : QObject(parent) {}
    bool canGoBack() { return m_historyList.count(); }
    QString lastVisited() { return m_historyList.last(); }
    QStringList all() { return m_historyList; }

public slots:
    void addPath( const QString &path ) { m_historyList << path; m_historyList.removeDuplicates(); }

private:
    QStringList m_historyList;
};

class FileIconProvider : public QFileIconProvider
{
public:
    inline explicit FileIconProvider(FileSystemModel *model = 0) : QFileIconProvider(), m_fsModel(model){}
    QIcon icon(const QFileInfo &info) const;

private:
    FileSystemModel *m_fsModel;
};

class DataGatherer;
class ThumbsLoader;
class ImagesThread;
class ViewContainer;
class FileSystemModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    class Node;
    typedef QList<FileSystemModel::Node *> Nodes;
    class Node
    {
    public:
        enum Children { Visible = 0, Hidden = 1, Filtered = 2, Deleted = 3 };
        virtual ~Node();
        void insertChild(Node *node);
        inline QString filePath() { return m_filePath; }
        inline QFileInfo fileInfo() { return m_fi; }
        void populate();
        void rePopulate();
        bool hasChild( const int child );
        bool hasChild( const QString &name );
        inline bool hasChildren() { return !m_children[Visible].isEmpty(); }
        Nodes children();
        int childCount();
        int row();
        Node *child(const int c);
        QString name();
        inline Node *parent() const { return m_parent; }
        inline bool isPopulated() const { return m_isPopulated; }
        inline bool isDir() const { return m_fi.isDir(); }
        inline void refresh() { m_fi.refresh(); }
        bool rename(const QString &newName);
        QVariant data(const int column);

        void sort(int column, Qt::SortOrder order);
        static void sort(Nodes *nodes);
        inline int sortColumn() { return model()->sortColumn(); }
        inline Qt::SortOrder sortOrder() { return model()->sortOrder(); }

        void setHiddenVisible(bool visible);
        inline bool showHidden() { return model()->showHidden(); }
        bool isHidden();

        inline void setLocked(bool lock) { m_isLocked = lock; }
        inline bool isLocked() { return m_isLocked; }

        inline void lockChildren(bool lock) { m_isChildLocked = lock; }
        inline bool childrenLock() { return m_isChildLocked; }

        void setFilter(const QString &filter);
        inline QString filter() const { return m_filter; }

        void blockChildren(bool p);
        void pause();

        Node *node(const QString &path, bool checkOnly = true);
        FileSystemModel *model();

    protected:
        Node *nodeFromPath(const QString &path, bool checkOnly = true);
        Node(FileSystemModel *model = 0, const QString &path = QString(), Node *parent = 0);

    private:
        bool m_isPopulated, m_isLocked, m_isChildLocked;
        Node *m_parent;
        Nodes m_children[Deleted+1];
        QFileInfo m_fi;
        QString m_filePath, m_filter, m_name;
        FileSystemModel *m_model;
        friend class FileSystemModel;
        friend class DataGatherer;
    };
    class FileNode : public Node
    {
    public:
        FileNode(FileSystemModel *model = 0, const QString &path = QString(), Node *parent = 0) : Node(model, path, parent){}
        ~FileNode(){}
    };
    class DirNode : public FileNode
    {
    public:
        DirNode(FileSystemModel *model = 0, const QString &path = QString(), Node *parent = 0) : FileNode(model, path, parent), m_dir(path){}
        ~DirNode(){}
        QDir dir() { return m_dir; }
    private:
        QDir m_dir;
    };

    enum Roles
    {
        FileIconRole = Qt::DecorationRole,
        FilePathRole = Qt::UserRole + 1,
        FileNameRole = Qt::UserRole + 2,
        FilePermissions = Qt::UserRole + 3,
        FlowImg = Qt::UserRole +4,
        FlowRefl = Qt::UserRole +5,
        FlowShape = Qt::UserRole +6
    };
    explicit FileSystemModel(QObject *parent = 0);
    ~FileSystemModel();
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const { return 4; }
    Node *fromIndex(const QModelIndex &index) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    QModelIndex index(const QString &path) const;
    void fetchMore(const QModelIndex &parent);
    bool canFetchMore(const QModelIndex &parent) const;
    void setRootPath(const QString &path);
    bool hasChildren(const QModelIndex &parent) const;
    void sort(int column, Qt::SortOrder order);
    void setHiddenVisible(bool visible);
    inline bool showHidden() const { return m_showHidden; }
    inline int sortColumn() const { return m_sortColumn; }
    inline Qt::SortOrder sortOrder() const { return m_sortOrder; }
    inline Node *rootNode() { return m_rootNode; }
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    inline FileIconProvider *ip() { return m_ip; }
    inline QIcon fileIcon(const QModelIndex &index) const { return bool(index.column()==0)?data(index, Qt::DecorationRole).value<QIcon>():QIcon(); }
    inline QString fileName(const QModelIndex &index) const { return index.isValid()?fromIndex(index)->name():QString(); }
    QString filePath(const QModelIndex &index) const;
    bool hasThumb(const QString &file) const;
    void forceEmitDataChangedFor(const QString &file);
    inline QString rootPath() const { return m_rootPath; }
    inline QDir rootDirectory() const { return QDir(m_rootPath); }
    inline FileIconProvider *iconProvider() { return m_ip; }
    inline bool isDir(const QModelIndex &index) const { return index.isValid()?fromIndex(index)->fileInfo().isDir():false; }
    inline QFileInfo fileInfo(const QModelIndex &index) const { return index.isValid()?fromIndex(index)->fileInfo():QFileInfo(); }
    QModelIndex mkdir(const QModelIndex &parent, const QString &name);
    inline DataGatherer *dataGatherer() { return m_dataGatherer; }
    inline ThumbsLoader *thumbsLoader() { return m_thumbsLoader; }
    inline QFileSystemWatcher *dirWatcher() { return m_watcher; }
    void setSort(const int sortColumn, const int sortOrder);
    void startPopulating();
    void endPopulating();
    bool isPopulating() const { return m_isPopulating; }

public slots:
    inline void setPath(const QString &path) { setRootPath(path); }
    void refresh();

private slots:
    void thumbFor( const QString &file, const QString &iconName );
    void flowDataAvailable( const QString &file );
    void dirChanged( const QString &path );
    void emitRootPathLater() { emit rootPathChanged(rootPath()); }
    void nodeGenerated(const QString &path, FileSystemModel::Node *node);

signals:
    void flowDataChanged( const QModelIndex &start, const QModelIndex &end );
    void rootPathChanged(const QString &path);
    void directoryLoaded(const QString &path);
    void fileRenamed(const QString &path, const QString &oldName, const QString &newName);
    void hiddenVisibilityChanged(bool visible);
    void sortingChanged(const int sortCol, const int order);

private:
    Node *m_rootNode, *m_current;
    QAbstractItemView *m_view;
    bool m_showHidden, m_isPopulating;
    QString m_rootPath;
    FileIconProvider *m_ip;
    int m_sortColumn;
    Qt::SortOrder m_sortOrder;
    ImagesThread *m_it;
    ThumbsLoader *m_thumbsLoader;
    ViewContainer *m_container;
    QFileSystemWatcher *m_watcher;
    DataGatherer *m_dataGatherer;
    friend class ImagesThread;
    friend class Node;
};

class DataGatherer : public QThread
{
    Q_OBJECT
public:
    enum Task { Populate = 0, Generate = 1, GetData = 2 };
    explicit DataGatherer(QObject *parent = 0):QThread(parent), m_node(0), m_model(static_cast<FileSystemModel *>(parent)),m_paused(false){}
    void populateNode(FileSystemModel::Node *node);
    void generateNode(const QString &path, FileSystemModel::Node *node);
    inline void pause(bool p) { if (m_paused=p) { qDebug() << "tried to deadlock"; return;} m_mutex.lock(); m_paused = p; m_mutex.unlock(); if ( !p ) m_pause.wakeAll(); }
    inline void pause() { m_mutex.lock(); if ( m_paused ) m_pause.wait(&m_mutex); m_mutex.unlock(); }

protected:
    void run();

signals:
    void nodeGenerated(const QString &path, FileSystemModel::Node *node);

private:
    FileSystemModel::Node *m_node, *m_result;
    FileSystemModel *m_model;
    QMutex m_mutex;
    QWaitCondition m_pause;
    Task m_task;
    QString m_path;
    bool m_paused;
    friend class FileSystemModel::Node;
    friend class FileSystemModel;
};

}

Q_DECLARE_METATYPE(QPainterPath)

#endif // FILESYSTEMMODEL_H
