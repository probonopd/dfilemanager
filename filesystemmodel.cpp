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
    connect ( model, SIGNAL(rootPathChanged(QString)), this, SLOT(loadThemedFolders(QString)) );
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
    if ( info.isDir() && s_themedDirs.contains(info.absoluteFilePath()) )
        return QIcon::fromTheme(s_themedDirs.value(info.absoluteFilePath()));
    QIcon icn = QFileIconProvider::icon(info);
    if ( QIcon::hasThemeIcon(icn.name()) )
        return QIcon::fromTheme(icn.name());
    return QFileIconProvider::icon(info);
}

void
FileIconProvider::loadThemedFolders(const QString &path)
{
    if ( m_fsModel )
    foreach ( const QFileInfo &file, QDir(path).entryInfoList(QDir::NoDotAndDotDot|QDir::AllDirs) )
    {
        QString dirPath = file.absoluteFilePath();
        if ( !dirPath.endsWith(QDir::separator()) )
            dirPath.append(QDir::separator());
        dirPath.append(".directory");
        const QSettings settings(dirPath, QSettings::IniFormat);
        const QString &iconName = settings.value("Desktop Entry/Icon").toString();
        if ( QIcon::hasThemeIcon(iconName)
             && ( !s_themedDirs.contains(file.absoluteFilePath())
             || ( s_themedDirs.contains(file.absoluteFilePath()) && s_themedDirs.value(file.absoluteFilePath()) != iconName ) ) )
        {
            s_themedDirs.insert(file.absoluteFilePath(), iconName);
            const QModelIndex &index = m_fsModel->index(file.absoluteFilePath());
            emit dataChanged(index, index);
        }
    }
}
//-----------------------------------------------------------------------------

FileSystemModel::Node::Node(FileSystemModel *model, const QString &path, Node *parent)
    : m_parent(0)
    , m_isPopulated(false)
    , m_fi(path)
    , m_model(model)
    , m_filePath(path)
    , m_isLocked(false)
{
    if ( parent )
    {
        m_parent = parent;
        m_parent->insertChild(this);
    }
}

FileSystemModel::Node::~Node() { qDeleteAll(m_children); }

int FileSystemModel::Node::row() { return parent()?parent()->children().indexOf(this):0; }

int FileSystemModel::Node::childCount() { return children().count(); }

bool FileSystemModel::Node::hasChild(const int child) { return child>-1&&child<children().count(); }

FileSystemModel::Node *FileSystemModel::Node::child(const int c) { return hasChild(c)?children().at(c):0; }

QString FileSystemModel::Node::name() { return fileInfo().fileName().isEmpty()?QDir::separator():m_fi.fileName(); }

FileSystemModel::Nodes FileSystemModel::Node::children() { return m_children; }

void
FileSystemModel::Node::insertChild(Node *node)
{
    if ( node->isHidden() && !showHidden() )
        m_hidden << node;
    else
        m_children << node;
}

bool
FileSystemModel::Node::rename(const QString &newName)
{
    const QString &oldName = fileInfo().absoluteFilePath();
    const QDir dir = fileInfo().path();
    if ( QFile::rename(oldName, dir.absoluteFilePath(newName)) )
    {
        m_fi.setFile(dir.absoluteFilePath(newName));
        m_filePath = dir.absoluteFilePath(newName);
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
*FileSystemModel::Node::genPath(QStringList path, int index)
{
    if ( !isPopulated() /*&& isRootNode()*/ )
        populate();

    int i = m_children.count();
    while ( --i > -1 )
    {
        Node *node = m_children.at(i);
        if ( node && node->name() == path.at(index) )
        {
            if ( node->name() == path.last() )
                return node;
            else if ( index+1 < path.count() )
                return node->genPath(path, index+1);
        }
    }
    i = m_hidden.count();
    const QModelIndex &idx = model()->index(filePath());
    while ( --i > -1 )
    {
        Node *node = m_hidden.at(i);
        if ( node && node->name() == path.at(index) )
        {
            model()->beginInsertRows(idx, m_children.count(), m_children.count());
            m_children << m_hidden.takeAt(i);
            model()->endInsertRows();
            if ( node->name() == path.last() )
                return node;
            else if ( index+1 < path.count() )
                return node->genPath(path, index+1);
        }
    }
    return 0;
}

FileSystemModel::Node
*FileSystemModel::Node::fromPath(const QString &path)
{
    QStringList p(path.split(QDir::separator()));
    QString first(p.takeFirst());
    first.append(QDir::separator());
    p.prepend(first);
    if ( path == QDir::separator() )
        p = QStringList() << QDir::separator();
    return genPath(p);
}

void
FileSystemModel::Node::populate()
{
    if ( isLocked() )
        setLocked(false);
    if ( !isPopulated() )
    if ( fileInfo().isDir() || isRootNode() )
    {
        refresh();
        if ( isRootNode() )
        {
            for ( int i = 0; i < QDir::drives().count(); ++i )
            {
                model()->beginInsertRows(model()->index(filePath()), i, i);
                new FileSystemModel::Node(model(), QDir::drives().at(i).filePath(), this);
                model()->endInsertRows();
            }
        }
        else
        {
            QDir::Filters filters = QDir::AllEntries|QDir::System|QDir::NoDotAndDotDot|QDir::Hidden;
            QDir dir(filePath());
            const QStringList entries = dir.entryList(filters);
            const int end = entries.count();
            if ( dir.isAbsolute() )
                for ( int i = 0; i < end; ++i )
                {
                    model()->beginInsertRows(model()->index(filePath()), i, i);
                    new FileSystemModel::Node(model(), dir.filePath(entries.at(i)), this);
                    model()->endInsertRows();
                }
        }
    }

    m_isPopulated = true;
}

void
FileSystemModel::Node::rePopulate()
{
    if ( isLocked() )
        setLocked(false);
    if ( !isPopulated() )
        return;
    const QModelIndex &index = model()->index(filePath());

    int i = m_children.count();
    while ( --i > -1 )
    {
        Node *node = m_children.at(i);
        node->refresh();
        if ( !node->fileInfo().exists() )
        {
            model()->beginRemoveRows(index, node->row(), node->row());
            delete m_children.takeAt(i);
            model()->endRemoveRows();
        }
    }

    i = m_hidden.count();
    while ( --i > -1 )
    {
        Node *node = m_hidden.at(i);
        node->refresh();
        if ( !node->fileInfo().exists() )
            delete m_hidden.takeAt(i);
    }

    const QDir::Filters filters = QDir::AllEntries|QDir::System|QDir::NoDotAndDotDot|QDir::Hidden;
    const QDir dir(filePath());
    QStringList entries = dir.entryList(filters);
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
    for ( int i = 0; i < m_children.count(); ++i )
        if ( m_children.at(i)->isPopulated() )
            m_children.at(i)->rePopulate();
}

bool
FileSystemModel::Node::hasChild(const QString &name)
{
    FileSystemModel::Nodes::const_iterator i = m_children.constBegin(), end = m_children.constEnd();
    while ( i != end )
    {
        if ( (*i)->name() == name )
            return true;
        ++i;
    }
    i = m_hidden.constBegin();
    end = m_hidden.constEnd();
    while ( i != end )
    {
        if ( (*i)->name() == name )
            return true;
        ++i;
    }
    return false;
}

static bool lessThen(FileSystemModel::Node *n1, FileSystemModel::Node *n2)
{
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
    emit model()->layoutAboutToBeChanged();
    qStableSort(m_children.begin(), m_children.end(), lessThen);
    emit model()->layoutChanged();

    Nodes::const_iterator i = m_children.constBegin(), end = m_children.constEnd();
    while ( i != end )
    {
        if ( (*i)->isPopulated() )
            (*i)->sort(column, order);
        ++i;
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
        model()->beginInsertRows(idx, m_children.count(), m_children.count()+m_hidden.count());
        m_children+=m_hidden;
        model()->endInsertRows();
        sort(&m_children);
        m_hidden.clear();
    }
    else
    {
        int i = m_children.count();
        while ( --i > -1 )
            if ( m_children.at(i)->isHidden() )
            {
                model()->beginRemoveRows(idx, i, i);
                m_hidden << m_children.takeAt(i);
                model()->endRemoveRows();
            }
    }
    int i = m_children.count();
    while ( --i > -1 )
        if ( m_children.at(i)->isPopulated() )
            m_children.at(i)->setHiddenVisible(visible);
//    i = m_hidden.count();
//    while ( --i > -1 )
//        if ( m_hidden.at(i)->isPopulated() )
//            m_hidden.at(i)->setHiddenVisible(visible);
}

bool
FileSystemModel::Node::isHidden()
{
    bool hidden = fileInfo().isHidden();
    if ( isLocked() )
        hidden = false;
    return hidden;
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
{
    if ( ViewContainer *vc = qobject_cast<ViewContainer *>(parent) )
        m_container = vc;

    connect ( m_thumbsLoader, SIGNAL(thumbFor(QString)), this, SLOT(thumbFor(QString)) );
    connect ( m_it, SIGNAL(imagesReady(QString)), this, SLOT(flowDataAvailable(QString)) );
    connect ( this, SIGNAL(rootPathChanged(QString)), m_it, SLOT(clearData()) );
    connect ( m_watcher, SIGNAL(directoryChanged(QString)), this, SLOT(dirChanged(QString)) );
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
FileSystemModel::setRootPath(const QString &path)
{
    if ( !QFileInfo(path).isDir() )
        return;

    const QModelIndex &from = index(rootPath());
    m_rootPath = path;
    if ( !m_watcher->directories().isEmpty() )
        m_watcher->removePaths(m_watcher->directories());

    Node *node = rootNode()->fromPath(path);
    if ( node->isPopulated() )
        node->rePopulate();
    else
        node->populate();
    node->setHiddenVisible(showHidden());
    QStringList newPaths;
    while ( node )
    {
        if ( !node->filePath().isEmpty() )
            newPaths << node->filePath();
        node->setLocked(true);
        node = node->parent();
    }
    m_watcher->addPaths(newPaths);
    changePersistentIndex(from, index(rootPath()));
    emit rootPathChanged(rootPath());
}

void
FileSystemModel::dirChanged(const QString &path)
{
    const QModelIndex &idx = index(path);
    if ( !idx.isValid() )
        return;

    Node *node = rootNode()->fromPath(path);
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

    Node *node = static_cast<Node *>(index.internalPointer());
    if ( !node )
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

    Node *node = m_rootNode->fromPath(path);
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
    if ( !node )
        return false;
    return (node->fileInfo().isDir()||node==m_rootNode);
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
    {
        node->populate();
        node->sort(sortColumn(), sortOrder());
    }
//    else
//        node->rePopulate();

}

void
FileSystemModel::sort(int column, Qt::SortOrder order)
{
    m_sortColumn = column;
    m_sortOrder = order;
    const QModelIndex &from = index(rootPath());
    m_rootNode->sort(column, order);
    changePersistentIndex(from, index(rootPath()));
}

void
FileSystemModel::setHiddenVisible(bool visible)
{
    if ( visible == m_showHidden )
        return;

    m_showHidden = visible;
    const QModelIndex &from = index(rootPath());
    rootNode()->setHiddenVisible(showHidden());
    changePersistentIndex(from, index(rootPath()));
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
