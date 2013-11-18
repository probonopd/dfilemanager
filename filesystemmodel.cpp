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

#include <QImageReader>

#include "filesystemmodel.h"
#include "iojob.h"
#include "thumbsloader.h"
#include "mainwindow.h"
#include "viewcontainer.h"

using namespace DFM;

static QMap<QString, QString> s_themedDirs;

FileIconProvider::FileIconProvider(FileSystemModel *model)
    : QFileIconProvider()
    , m_fsModel(model)
{
    connect ( model, SIGNAL(directoryLoaded(QString)), this, SLOT(loadThemedFolders(QString)) );
    connect ( this, SIGNAL(dataChanged(QModelIndex,QModelIndex)), model, SIGNAL(dataChanged(QModelIndex,QModelIndex)) );
}

QIcon
FileIconProvider::icon(const QFileInfo &info) const
{
#if 0
    if ( Store::config.icons.customIcons.contains(info.filePath()) )
        return QIcon(Store::config.icons.customIcons.value(info.filePath()));
#endif
    if ( info.absoluteFilePath() == QDir::rootPath() )
        if ( QIcon::hasThemeIcon("folder-system") )
            return QIcon::fromTheme("folder-system");
        else if ( QIcon::hasThemeIcon("inode-directory") )
            return QIcon::fromTheme("inode-directory");

    if ( s_themedDirs.contains(info.absoluteFilePath()) )
        return QIcon::fromTheme(s_themedDirs.value(info.absoluteFilePath()));

    QIcon icn = QFileIconProvider::icon(info);
    if ( QIcon::hasThemeIcon(icn.name()) )
        return QIcon::fromTheme(icn.name());

    return QFileIconProvider::icon(info);
}

void
FileIconProvider::loadThemedFolders(const QString &path)
{
    if ( !m_fsModel )
        return;
    const QDir dir(path, "*", 0, QDir::NoDotAndDotDot|QDir::AllDirs);
    const QStringList &entries(dir.entryList());
    for ( int i = 0; i < entries.count(); ++i )
    {
        const QString &dirFile(dir.absoluteFilePath(entries.at(i)));
        const QString &settingsFile(QDir(dirFile).absoluteFilePath(".directory"));
        const QSettings settings(settingsFile, QSettings::IniFormat);
        const QString &iconName = settings.value("Desktop Entry/Icon").toString();
        if ( QIcon::hasThemeIcon(iconName)
             && ( !s_themedDirs.contains(dirFile)
             || ( s_themedDirs.contains(dirFile) && s_themedDirs.value(dirFile) != iconName ) ) )
        {
            s_themedDirs.insert(dirFile, iconName);
            const QModelIndex &index = m_fsModel->index(dir.absoluteFilePath(dirFile));
            emit dataChanged(index, index);
        }
    }
}
//-----------------------------------------------------------------------------

FileSystemModel::Node::Node(FileSystemModel *model, const QString &path, Node *parent)
    : m_parent(parent)
    , m_isPopulated(false)
    , m_fi(path)
    , m_model(model)
    , m_filePath(path)
    , m_isLocked(false)
{
    if ( m_parent )
        m_parent->insertChild(this);
}

FileSystemModel::Node::~Node() { for (int i = 0; i<Filtered+1; ++i ) qDeleteAll(m_children[i]); }

int FileSystemModel::Node::row() { return parent()?parent()->children().indexOf(this):0; }

int FileSystemModel::Node::childCount() { return children().count(); }

bool FileSystemModel::Node::hasChild(const int child) { return child>-1&&child<children().count(); }

FileSystemModel::Node *FileSystemModel::Node::child(const int c) { return hasChild(c)?children().at(c):0; }

QString FileSystemModel::Node::name() { return isRootNode()?QString():fileInfo().fileName().isEmpty()?filePath():m_fi.fileName(); }

FileSystemModel::Nodes FileSystemModel::Node::children() { return m_children[Visible]; }

bool FileSystemModel::Node::isHidden() { return fileInfo().isHidden()&&!isLocked(); }

void
FileSystemModel::Node::setFilter(const QString &filter)
{
    for ( int i = 0; i < Filtered; ++i )
    {
        int c = m_children[i].count();
        while ( --c > -1 )
            if ( !m_children[i].at(c)->name().contains(filter, Qt::CaseInsensitive) )
                m_children[Filtered] << m_children[i].takeAt(c);
    }

    int f = m_children[Filtered].count();
    while ( --f > -1 )
        if ( m_children[Filtered].at(f)->name().contains(filter, Qt::CaseInsensitive) )
        {
            if ( m_children[Filtered].at(f)->isHidden() && !showHidden() )
                m_children[Hidden] << m_children[Filtered].takeAt(f);
            else if ( m_children[Filtered].at(f)->isHidden() && showHidden() )
                m_children[Visible] << m_children[Filtered].takeAt(f);
            else
                m_children[Visible] << m_children[Filtered].takeAt(f);
        }
    if ( m_filter != filter )
    {
        m_filter = filter;
        model()->sort(sortColumn(), sortOrder());
    }
}

void
FileSystemModel::Node::insertChild(Node *node)
{
    if ( node->isHidden() && !showHidden() )
        m_children[Hidden] << node;
    else
        m_children[Visible] << node;
}

bool
FileSystemModel::Node::rename(const QString &newName)
{
    const QString &oldName = fileInfo().absoluteFilePath();
    const QDir dir = fileInfo().path();
    if ( QFile::rename(oldName, dir.absoluteFilePath(newName)) )
    {
        m_filePath = dir.absoluteFilePath(newName);
        m_fi.setFile(m_filePath);
        m_fi.refresh();
        return true;
    }
    return false;
}

QVariant
FileSystemModel::Node::data(const int column)
{
    switch (column)
    {
    case 0: return name(); break;
    case 1: return fileInfo().isDir()?QString("--"):Ops::prettySize(fileInfo().size()); break;
    case 2:
    {
        if ( fileInfo().isDir() )
            return QString("directory");
        else if ( fileInfo().isSymLink() )
            return QString("symlink");
        else if ( fileInfo().suffix().isEmpty() && fileInfo().isExecutable() )
            return QString("exec");
        else if ( fileInfo().suffix().isEmpty() )
            return QString("file");
        else
            return fileInfo().suffix();
        break;
    }
    case 3: return fileInfo().lastModified(); break;
    default: return QString("--");
    }
}

FileSystemModel::Node
*FileSystemModel::Node::nodeFromPath(const QString &path, bool checkOnly)
{
    if ( !isPopulated() )
    {
        if ( checkOnly )
            return 0;
        else
            populate();
    }

    for ( int i = 0; i < Filtered+1; ++i )
    {
        int c = m_children[i].count();
        while ( --c > -1 )
        {
            Node *node = m_children[i].at(c);
            if ( node && node->filePath() == path )
            {
                if ( i )
                    m_children[Visible] << m_children[i].takeAt(c);
                return node;
            }
        }
    }
    return 0;
}

FileSystemModel::Node
*FileSystemModel::Node::node(const QString &path, bool checkOnly)
{
    QFileInfo fi(path);
    if ( !fi.exists() || !fi.isAbsolute() )
        return 0;
    QStringList paths;
    QDir dir;
    if ( fi.isDir() )
        dir = QDir(path);
    else
    {
        dir = fi.dir();
        paths << dir.absolutePath();
    }
    paths << path;
    while (dir.cdUp())
        paths.prepend(dir.absolutePath());

    Node *node = model()->rootNode();

    while ( !paths.isEmpty() && node )
        node = node->nodeFromPath(paths.takeFirst(), checkOnly);

    if ( node && node->filePath() == path )
        return node;

    return 0;
}

static QDir::Filters filters = QDir::AllEntries|QDir::System|QDir::NoDotAndDotDot|QDir::Hidden;

void
FileSystemModel::Node::populate()
{
    if ( isLocked() )
        setLocked(false);
    if ( isPopulated() )
        return;
    m_isPopulated = true;
    if ( isRootNode() )
    {
        model()->beginInsertRows(QModelIndex(), 0, QDir::drives().count()-1);
        for ( int i = 0; i < QDir::drives().count(); ++i )
            new FileSystemModel::Node(model(), QDir::drives().at(i).filePath(), this);
        model()->endInsertRows();
    }
    else if ( fileInfo().isDir() )
    {
        refresh();
        QDir dir(filePath());
        const QStringList entries = dir.entryList(filters);
        const int end = entries.count();
        if ( !dir.isAbsolute() )
            return;
        model()->beginInsertRows(model()->index(filePath()), 0, end-1);
        for ( int i = 0; i < end; ++i )
            new FileSystemModel::Node(model(), dir.filePath(entries.at(i)), this);
        model()->endInsertRows();
    }
    if ( m_children[Visible].count()>1 )
        sort(&m_children[Visible]);
}

void
FileSystemModel::Node::rePopulate()
{
    if ( isLocked() )
        setLocked(false);
    if ( !isPopulated() )
    {
        populate();
        if ( filePath() == model()->rootPath() )
            emit model()->directoryLoaded(filePath());
        return;
    }
    const QModelIndex &index = model()->index(filePath());

    for ( int i = 0; i < Filtered+1; ++i )
    {
        int c = m_children[i].count();
        while ( --c > -1 )
        {
            Node *node = m_children[i].at(c);
            node->refresh();
            if ( !node->fileInfo().exists() )
            {
                if ( !i )
                    model()->beginRemoveRows(index, node->row(), node->row());
                delete m_children[i].takeAt(c);
                if ( !i )
                    model()->endRemoveRows();
            }
        }
    }

    const QDir dir(filePath());
    const QStringList entries = dir.entryList(filters);
    if ( dir.isAbsolute() )
        for ( int i = 0; i < entries.count(); ++i )
        {
            if ( !hasChild(entries.at(i)) )
            {
                model()->beginInsertRows(index, childCount(), childCount());
                new Node(m_model, dir.filePath(entries.at(i)), this);
                model()->endInsertRows();
            }
        }
    if ( filePath() == model()->rootPath() )
        emit model()->directoryLoaded(filePath());

    for ( int i = 0; i < m_children[Visible].count(); ++i )
        if ( m_children[Visible].at(i)->isPopulated() )
            m_children[Visible].at(i)->rePopulate();
}

bool
FileSystemModel::Node::hasChild(const QString &name)
{
    for ( int i = 0; i < Filtered+1; ++i )
    {
        FileSystemModel::Nodes::const_iterator start = m_children[i].constBegin(), end = m_children[i].constEnd();
        while ( start != end )
        {
            if ( (*start)->name() == name )
                return true;
            ++start;
        }
    }
    return false;
}

static bool lessThen(FileSystemModel::Node *n1, FileSystemModel::Node *n2)
{
    if ( n1==n2 || n1->name().isEmpty() || n2->name().isEmpty() )
        return false;
    const Qt::SortOrder order = n1->sortOrder();
    const int column = n1->sortColumn();

    if ( n1->fileInfo().isDir() && !n2->fileInfo().isDir() )
        return true;
    if ( !n1->fileInfo().isDir() && n2->fileInfo().isDir() )
        return false;

    if ( n1->isHidden() && !n2->isHidden() )
        return false;
    if ( !n1->isHidden() && n2->isHidden() )
        return true;

    switch (column)
    {
    case 0:
        if ( order == Qt::AscendingOrder ) //name
            return n1->name().toLower()<n2->name().toLower();
        else
            return !(n1->name().toLower()<n2->name().toLower());
        break;
    case 1:
        if ( order == Qt::AscendingOrder )  //size
            return n1->fileInfo().size()<n2->fileInfo().size();
        else
            return !(n1->fileInfo().size()<n2->fileInfo().size());
        break;
    case 2:
        if ( order == Qt::AscendingOrder )  //type
            return n1->fileInfo().suffix()<n2->fileInfo().suffix();
        else
            return !(n1->fileInfo().suffix()<n2->fileInfo().suffix());
        break;
    case 3:
        if ( order == Qt::AscendingOrder )  //lastModified
            return n1->fileInfo().lastModified()<n2->fileInfo().lastModified();
        else
            return !(n1->fileInfo().lastModified()<n2->fileInfo().lastModified());
        break;
    default:
        return false;
    }
}

void
FileSystemModel::Node::sort(int column, Qt::SortOrder order)
{
    if ( m_children[Visible].isEmpty() )
        return;

    if ( m_children[Visible].count() > 1 )
        qStableSort(m_children[Visible].begin(), m_children[Visible].end(), lessThen);

    int i = m_children[Visible].count();
    while ( --i > -1 )
    {
        Node *node = m_children[Visible].at(i);
        if ( node->isPopulated() && node->hasChildren() )
            node->sort(column, order);
    }
}

void
FileSystemModel::Node::sort(Nodes *nodes)
{
    qStableSort(nodes->begin(), nodes->end(), lessThen);
}

void
FileSystemModel::Node::setHiddenVisible(bool visible)
{
    const QModelIndex &idx = model()->index(filePath());
    if ( visible )
    {
        model()->beginInsertRows(idx, m_children[Visible].count(), m_children[Visible].count()+m_children[Hidden].count());
        m_children[Visible]+=m_children[Hidden];
        model()->endInsertRows();
        sort(&m_children[Visible]);
        m_children[Hidden].clear();
    }
    else
    {
        int i = m_children[Visible].count();
        while ( --i > -1 )
            if ( m_children[Visible].at(i)->isHidden() )
            {
                model()->beginRemoveRows(idx, i, i);
                m_children[Hidden] << m_children[Visible].takeAt(i);
                model()->endRemoveRows();
            }
    }
    int i = m_children[Visible].count();
    while ( --i > -1 )
        if ( m_children[Visible].at(i)->isPopulated() )
            m_children[Visible].at(i)->setHiddenVisible(visible);
//    i = m_children[Hidden].count();
//    while ( --i > -1 )
//        if ( m_children[Hidden].at(i)->isPopulated() )
//            m_children[Hidden].at(i)->setHiddenVisible(visible);
}

//-----------------------------------------------------------------------------

void
DataGatherer::run()
{
    switch ( m_task )
    {
    case Populate:
    {
        if ( !m_node )
            return;
        m_node->rePopulate();
        m_node->setHiddenVisible(m_node->showHidden());
        FileSystemModel *model = m_node->model();
        if ( m_node->filePath() != model->rootPath() )
            return;
        QFileSystemWatcher *watcher = model->dirWatcher();
        if ( !watcher->directories().isEmpty() )
            watcher->removePaths(watcher->directories());
        QStringList newPaths;
        FileSystemModel::Node *node = m_node;
        while ( node )
        {
            if ( !node->filePath().isEmpty() )
                newPaths << node->filePath();
            node->setLocked(true);
            node = node->parent();
        }
        watcher->addPaths(newPaths);
        break;
    }
    case Generate:
    {
        if ( QFileInfo(m_path).isDir() )
            m_result = m_node->node(m_path, false);
        if ( m_result )
            emit nodeGenerated(m_path, m_result);
        break;
    }
    default:
        break;
    }
}

void
DataGatherer::populateNode(FileSystemModel::Node *node)
{
    if ( isRunning() )
        return;
    m_mutex.lock();
    m_task = Populate;
    m_node = node;
    m_mutex.unlock();
    start();
//    QTimer::singleShot(1000, this, SLOT(start()));
}

void
DataGatherer::generateNode(const QString &path, FileSystemModel::Node *node)
{
    if ( isRunning() )
        return;
    m_mutex.lock();
    m_task = Generate;
    m_path = path;
    m_node = node;
    m_result = 0;
    m_mutex.unlock();
    start();
}

//-----------------------------------------------------------------------------

FileSystemModel::FileSystemModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_rootNode(new Node(this))
    , m_view(0)
    , m_showHidden(false)
    , m_ip(new FileIconProvider(this))
    , m_sortOrder(Qt::AscendingOrder)
    , m_sortColumn(0)
    , m_it(new ImagesThread(this))
    , m_thumbsLoader(new ThumbsLoader(this))
    , m_watcher(new QFileSystemWatcher(this))
    , m_container(0)
    , m_dataGatherer(new DataGatherer(this))
{
    if ( ViewContainer *vc = qobject_cast<ViewContainer *>(parent) )
        m_container = vc;

    connect ( m_thumbsLoader, SIGNAL(thumbFor(QString)), this, SLOT(thumbFor(QString)) );
    connect ( m_it, SIGNAL(imagesReady(QString)), this, SLOT(flowDataAvailable(QString)) );
    connect ( this, SIGNAL(rootPathChanged(QString)), m_it, SLOT(clearData()) );
    connect ( m_watcher, SIGNAL(directoryChanged(QString)), this, SLOT(dirChanged(QString)) );
    connect ( m_dataGatherer, SIGNAL(nodeGenerated(QString,FileSystemModel::Node*)), this, SLOT(nodeGenerated(QString,FileSystemModel::Node*)) );
}

FileSystemModel::~FileSystemModel()
{
    delete m_rootNode;
    m_thumbsLoader->clearQueue();
    m_it->clearQueue();
    while ( m_thumbsLoader->isRunning() )
        m_thumbsLoader->wait();
    while ( m_it->isRunning() )
        m_it->wait();
}

void
FileSystemModel::nodeGenerated(const QString &path, FileSystemModel::Node *node)
{
    if ( path != rootPath() )
        return;

    emit rootPathChanged(rootPath());
    dataGatherer()->populateNode(node);
}

void
FileSystemModel::setRootPath(const QString &path)
{
    if ( rootPath() == path )
        return;
    m_rootPath = path;
    Node *node = rootNode()->node(path);
    if ( !node )
        dataGatherer()->generateNode(path, rootNode());
    else
        nodeGenerated(path, node);
}

void
FileSystemModel::refresh()
{
    Node *node = rootNode()->node(rootPath());
    if ( !node )
        return;
    node->rePopulate();
}

void
FileSystemModel::dirChanged(const QString &path)
{
    const QModelIndex &idx = index(path);
    if ( !idx.isValid() || !idx.parent().isValid() )
        return;

    Node *node = rootNode()->node(path);
    if ( !node )
        return;
    node->refresh();
    if ( !node->fileInfo().exists() )
    {
        beginRemoveRows(idx.parent(), node->row(), node->row());
        delete node;
        endRemoveRows();
    }
    else
        node->rePopulate();
}

Qt::ItemFlags
FileSystemModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    Node *node = fromIndex(index);
    if ( node->fileInfo().isWritable() )
        flags |= Qt::ItemIsEditable;
    if ( node->fileInfo().isDir() )
        flags |= Qt::ItemIsDropEnabled;
    if ( node->fileInfo().isReadable() )
        flags |= Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
    return flags;
}

void
FileSystemModel::forceEmitDataChangedFor(const QString &file)
{
    const QModelIndex &idx = index(file);
    emit dataChanged(idx, idx);
    emit flowDataChanged(idx, idx);
}

void
FileSystemModel::flowDataAvailable(const QString &file)
{
    const QModelIndex &idx = index(file);
    emit flowDataChanged(idx, idx);
}

void
FileSystemModel::thumbFor(const QString &file)
{
    const QModelIndex &idx = index(file);
    emit dataChanged(idx, idx);
    if ( m_container->currentViewType() == ViewContainer::Flow )
        m_it->queueFile(file, m_thumbsLoader->thumb(file), true);
}

bool FileSystemModel::hasThumb(const QString &file) const { return m_thumbsLoader->hasThumb(file); }

QVariant
FileSystemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    Node *node = fromIndex(index);
    if ( node->isRootNode() )
        return QVariant();
    if ( role == Qt::DecorationRole && index.column() > 0 )
        return QVariant();
    if ( role == FileNameRole )
        return node->name();
    if ( role == FilePathRole )
        return node->filePath();

    if ( role == Qt::TextAlignmentRole )
    if ( index.column() == 1 )
        return int(Qt::AlignVCenter|Qt::AlignRight);
    else
        return int(Qt::AlignLeft|Qt::AlignVCenter);

    const QString &file = node->filePath();
    const bool customIcon = Store::config.icons.customIcons.contains(file);

    if ( index.column() == 0 && role == Qt::DecorationRole && customIcon )
        return QIcon(Store::config.icons.customIcons.value(file));

    if ( role == Qt::DecorationRole )
    {
        if ( m_thumbsLoader->hasThumb(file) )
            return QIcon(QPixmap::fromImage(m_thumbsLoader->thumb(file)));
        else
        {
            if ( Store::config.views.showThumbs )
                m_thumbsLoader->queueFile(file);
            return m_ip->icon(QFileInfo(file));
        }
    }

    if ( role >= FlowImg )
    {
        const QIcon &icon = customIcon ? QIcon(Store::config.icons.customIcons.value(file)) : m_ip->icon(node->fileInfo());

        if ( m_it->hasData(file) )
            return QPixmap::fromImage(m_it->flowData(file, role == FlowRefl));
        if ( !m_thumbsLoader->hasThumb(file) && Store::config.views.showThumbs )
            m_thumbsLoader->queueFile(file);

        if ( m_thumbsLoader->hasThumb(file) && !m_it->hasData(file) && Store::config.views.showThumbs )
            m_it->queueFile(file, m_thumbsLoader->thumb(file));

        if ( m_it->hasNameData(icon.name()) )
            return QPixmap::fromImage(m_it->flowNameData(icon.name(), role == FlowRefl));
        else if ( !icon.name().isEmpty() )
            m_it->queueName(icon);

        if ( role == FlowRefl )
            return QPixmap();

        return icon.pixmap(SIZE);
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    return node->data(index.column());
}

bool
FileSystemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if ( !index.isValid() || role != Qt::EditRole )
        return false;

    Node *node = fromIndex(index);
    const QString &newName = value.toString();
    const QString &oldName = node->fileInfo().fileName();
    const QString &path = node->fileInfo().path();
    if ( node->rename(newName) )
    {
        emit fileRenamed(path, oldName, newName);
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

QVariant
FileSystemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ( role == Qt::DisplayRole )
        switch ( section )
        {
        case 0: return tr("Name"); break;
        case 1: return tr("Size"); break;
        case 2: return tr("Type"); break;
        case 3: return tr("Last Modified"); break;
        default: break;
        }
    else if ( role == Qt::TextAlignmentRole )
        return bool(section == 1) ? int(Qt::AlignRight|Qt::AlignVCenter) : int(Qt::AlignLeft|Qt::AlignVCenter);
    return QVariant();
}

QMimeData
*FileSystemModel::mimeData(const QModelIndexList &indexes) const
{
    QList<QUrl> urls;
    for ( int i = 0; i < indexes.count(); ++i )
    {
        const QModelIndex &index = indexes.at(i);
        if ( !index.isValid() )
            continue;
        Node *node = fromIndex(index);
        urls << QUrl::fromLocalFile(node->filePath());
    }
    QMimeData *data = new QMimeData();
    data->setUrls(urls);
    return data;
}

int
FileSystemModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0 || !parent.isValid())
        return 0;

    return fromIndex(parent)->childCount();
}

FileSystemModel::Node
*FileSystemModel::fromIndex(const QModelIndex &index) const
{
    Node *node = static_cast<Node *>(index.internalPointer());
    if ( index.isValid() && node )
        return node;
    return m_rootNode;
}

QModelIndex
FileSystemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    Node *parentNode = fromIndex(parent);
    if ( !parentNode )
        return QModelIndex();
    Node *childNode = parentNode->child(row);

    if (childNode)
        return createIndex(row, column, childNode);
    else
        return QModelIndex();
}

QModelIndex
FileSystemModel::index(const QString &path) const
{
    if ( !QFileInfo(path).exists() )
        return QModelIndex();

    Node *node = m_rootNode->node(path);
    if ( !node )
        return QModelIndex();

    return createIndex(node->row(), 0, node);
}

QModelIndex
FileSystemModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    Node *childNode = static_cast<Node *>(child.internalPointer());
    Node *parentNode = childNode->parent();

    if (parentNode == m_rootNode)
        return QModelIndex();

    return createIndex(parentNode->row(), 0, parentNode);
}

bool
FileSystemModel::hasChildren(const QModelIndex &parent) const
{
    if ( !parent.isValid() )
        return false;
    return canFetchMore(parent);
}

bool
FileSystemModel::canFetchMore(const QModelIndex &parent) const
{
    if ( !parent.isValid() )
        return false;
    if ( parent.column() != 0 )
        return false;
    Node *node = fromIndex(parent);
    if ( node->isRootNode() )
        return true;
    return node->fileInfo().isDir();
}

void
FileSystemModel::fetchMore(const QModelIndex &parent)
{
    if ( parent.isValid() && parent.column() != 0 )
        return;

    Node *node = fromIndex(parent);
    if ( !node )
        return;
    if ( !node->isPopulated() )
        dataGatherer()->populateNode(node);

}

void
FileSystemModel::sort(int column, Qt::SortOrder order)
{
    const bool orderChanged = bool(m_sortColumn!=column||m_sortOrder!=order);
    m_sortColumn = column;
    m_sortOrder = order;
    emit layoutAboutToBeChanged();
    const QModelIndexList &oldList = persistentIndexList();
    Nodes old;
    for ( int i = 0; i < oldList.count(); ++i )
        old << fromIndex(oldList.at(i));

    rootNode()->sort(column, order);

    QModelIndexList newList;
    for ( int i = 0; i < old.count(); ++i )
        newList << index(old.at(i)->filePath());
    changePersistentIndexList(oldList, newList);
    if ( orderChanged )
        emit sortingChanged(column, order);
    emit layoutChanged();

#ifdef Q_WS_X11
    if ( Store::config.views.dirSettings && orderChanged )
    {
        QDir dir(rootPath());
        QSettings settings(dir.absoluteFilePath(".directory"), QSettings::IniFormat);
        settings.beginGroup("DFM");
        QVariant varCol = settings.value("sortCol");
        if ( varCol.isValid() )
        {
            int col = varCol.value<int>();
            if ( col != column )
                settings.setValue("sortCol", column);
        }
        else
            settings.setValue("sortCol", column);
        QVariant varOrd = settings.value("sortOrd");
        if ( varCol.isValid() )
        {
            Qt::SortOrder ord = (Qt::SortOrder)varOrd.value<int>();
            if ( ord != order )
                settings.setValue("sortOrd", order);
        }
        else
            settings.setValue("sortOrd", order);
        settings.endGroup();
    }
#endif
}

void
FileSystemModel::setHiddenVisible(bool visible)
{
    if ( visible == m_showHidden )
        return;

    m_showHidden = visible;
//    const QModelIndex &from = index(rootPath());
    rootNode()->setHiddenVisible(showHidden());
//    changePersistentIndex(from, index(rootPath()));
    emit hiddenVisibilityChanged(m_showHidden);
}

bool
FileSystemModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (!data->hasUrls())
        return false;

    Node *node = fromIndex(parent);
    if ( !node->fileInfo().isDir() )
        return false;

    IO::Job::copy(data->urls(), node->filePath(), true, true);
    return true;
}

QModelIndex
FileSystemModel::mkdir(const QModelIndex &parent, const QString &name)
{
    if ( parent.isValid() )
    {
        Node *node = fromIndex(parent);
        QDir dir(node->filePath());
        dir.mkdir(name);
        return index(dir.absoluteFilePath(name));
    }
    return QModelIndex();
}

QString
FileSystemModel::filePath(const QModelIndex &index) const
{
    if ( !index.isValid() )
        return QString();
    Node *node = fromIndex(index);
    if ( !node )
        return QString();
    return node->filePath();
}
