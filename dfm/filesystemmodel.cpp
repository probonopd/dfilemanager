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
#include <QDirIterator>
#include <QMutexLocker>

#include "filesystemmodel.h"
#include "iojob.h"
#include "thumbsloader.h"
#include "mainwindow.h"
#include "viewcontainer.h"

using namespace DFM;

QIcon
FileIconProvider::icon(const QFileInfo &info) const
{
#if 0
    if (Store::config.icons.customIcons.contains(info.filePath()))
        return QIcon(Store::config.icons.customIcons.value(info.filePath()));
#endif
    if (info.absoluteFilePath() == QDir::rootPath())
        if (QIcon::hasThemeIcon("folder-system"))
            return QIcon::fromTheme("folder-system");
        else if (QIcon::hasThemeIcon("inode-directory"))
            return QIcon::fromTheme("inode-directory");

    if (ThumbsLoader::hasIcon(info.absoluteFilePath()))
        if (QIcon::hasThemeIcon(ThumbsLoader::icon(info.absoluteFilePath())))
            return QIcon::fromTheme(ThumbsLoader::icon(info.absoluteFilePath()));

    QIcon icn = QFileIconProvider::icon(info);
    if (QIcon::hasThemeIcon(icn.name()))
        return QIcon::fromTheme(icn.name());

    return icn;
}

//-----------------------------------------------------------------------------
using namespace FS;

Model::Model(QObject *parent)
    : QAbstractItemModel(parent)
    , m_rootNode(new Node(this))
    , m_view(0)
    , m_showHidden(false)
    , m_ip(new FileIconProvider(this))
    , m_sortOrder(Qt::AscendingOrder)
    , m_sortColumn(0)
    , m_it(new ImagesThread(this))
    , m_watcher(new QFileSystemWatcher(this))
    , m_container(0)
    , m_dataGatherer(new Worker::Gatherer(this))
    , m_isPopulating(false)
    , m_schemeMenu(new QMenu())
{
    if (ViewContainer *vc = qobject_cast<ViewContainer *>(parent))
        m_container = vc;

    connect (ThumbsLoader::instance(), SIGNAL(thumbFor(QString,QString)), this, SLOT(thumbFor(QString,QString)));
    connect (m_it, SIGNAL(imagesReady(QString)), this, SLOT(flowDataAvailable(QString)));
    connect (this, SIGNAL(urlChanged(QUrl)), m_it, SLOT(clearData()));
    connect (m_watcher, SIGNAL(directoryChanged(QString)), this, SLOT(dirChanged(QString)));
    connect (m_dataGatherer, SIGNAL(nodeGenerated(QString,Node*)), this, SLOT(nodeGenerated(QString,Node*)));
    connect(this, SIGNAL(fileRenamed(QString,QString,QString)), ThumbsLoader::instance(), SLOT(fileRenamed(QString,QString,QString)));
    connect(m_dataGatherer, SIGNAL(finished()), this, SLOT(removeDeletedLater()));
}

Model::~Model()
{
    m_it->discontinue();
    if (m_it->isRunning())
        m_it->wait();
    delete m_rootNode;
    m_schemeMenu->deleteLater();
}

void
Model::setSort(const int sortColumn, const int sortOrder)
{
    m_sortColumn = sortColumn;
    m_sortOrder = (Qt::SortOrder)sortOrder;
    emit sortingChanged(m_sortColumn, (int)m_sortOrder);
}

void
Model::nodeGenerated(const QString &path, Node *node)
{
    if (path != rootPath() || node == m_rootNode)
        return;

    emit urlChanged(QUrl::fromLocalFile(path));
    dataGatherer()->populateNode(node);
}

void
Model::setRootPath(const QString &path)
{
    ThumbsLoader::clearQueue();
    m_rootPath = path;
    if (!m_watcher->directories().isEmpty())
        m_watcher->removePaths(m_watcher->directories());
    if (QFileInfo(m_rootPath).exists())
        m_watcher->addPath(m_rootPath);

    Node *node = rootNode()->fromUrl(QUrl::fromLocalFile(path));

    if (!node)
        dataGatherer()->generateNode(path);
    else
        nodeGenerated(path, node);
}

void
Model::setUrl(const QUrl &url)
{
    m_url = url;
    if (url.isLocalFile())
        setRootPath(url.toLocalFile());
    else
        emit urlChanged(url);
}

void
Model::setUrlFromDynamicPropertyUrl()
{
    if (!sender())
        return;

    QVariant var = sender()->property("url");
    if (var.isValid())
        setUrl(var.value<QUrl>());
}

void
Model::refresh()
{
    Node *node = rootNode()->fromUrl(QUrl::fromLocalFile(m_rootPath));
    dataGatherer()->populateNode(node);
}

void
Model::dirChanged(const QString &path)
{
    const QModelIndex &idx = index(QUrl::fromLocalFile(path));

    Node *node = nodeForIndex(idx);
    if (!node)
        return;

    node->refresh();
    if (!node->exists())
    {
        if (Node *parent = node->parent())
            parent->rePopulate();
    }
    else
        dataGatherer()->populateNode(node);
}

Qt::ItemFlags
Model::flags(const QModelIndex &index) const
{
    Node *node = fromIndex(index);
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    if (node->isWritable()) flags |= Qt::ItemIsEditable;
    if (node->isDir()) flags |= Qt::ItemIsDropEnabled;
    if (node->isReadable() && !isWorking()) flags |= Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
    return flags;
}

void
Model::forceEmitDataChangedFor(const QString &file)
{
    const QModelIndex &idx = index(QUrl::fromLocalFile(file));
    emit dataChanged(idx, idx);
    emit flowDataChanged(idx, idx);
}

void
Model::flowDataAvailable(const QString &file)
{
    const QModelIndex &idx = index(QUrl::fromLocalFile(file));
    emit flowDataChanged(idx, idx);
}

void
Model::thumbFor(const QString &file, const QString &iconName)
{
    const QModelIndex &idx = index(QUrl::fromLocalFile(file));
    emit dataChanged(idx, idx);
    if (m_container->currentViewType() == ViewContainer::Flow)
        if (!QFileInfo(file).isDir())
            m_it->queueFile(file, ThumbsLoader::thumb(file), true);
        else if (!iconName.isNull())
            m_it->queueName(QIcon::fromTheme(iconName));
}

bool Model::hasThumb(const QString &file) { return ThumbsLoader::instance()->hasThumb(file); }

QVariant
Model::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()||index.column()>3||index.row()>100000)
        return QString("--");

    Node *node = nodeForIndex(index);

    const QString &file = node->filePath();
    const int col = index.column();
    if (node == m_rootNode)
        return QVariant();
    if (role == Qt::DecorationRole && col > 0)
        return QVariant();
    if (role == FileNameRole)
        return node->name();
    if (role == FilePathRole)
        return node->filePath();
    if (role == FilePermissions)
        return node->permissionsString();

    if (role == Qt::TextAlignmentRole)
        if (col == 1)
            return int(Qt::AlignVCenter|Qt::AlignRight);
        else
            return int(Qt::AlignLeft|Qt::AlignVCenter);

    if (role == Qt::FontRole && !col)
    {
        QFont f(qApp->font());
        if (node->isSymLink())
        {
            f.setItalic(true);
            return f;
        }
        if (!node->isDir() && node->isExecutable() && (node->suffix().isEmpty()||node->suffix()=="sh"||node->suffix()=="exe"))
        {
            f.setUnderline(true);
            return f;
        }
    }

    if (role == Qt::DecorationRole)
    {
        if (ThumbsLoader::hasThumb(file))
            return QIcon(QPixmap::fromImage(ThumbsLoader::thumb(file)));
        else
        {
            if ((Store::config.views.showThumbs || node->isDir()) && !isWorking())
                ThumbsLoader::queueFile(file);
            return m_ip->icon(*node);
        }
    }

    if (role >= FlowImg && role != Category)
    {
        const QIcon &icon = m_ip->icon(*node);

        if (m_it->hasData(file))
            return QPixmap::fromImage(m_it->flowData(file, role == FlowRefl));

        if (!ThumbsLoader::hasThumb(file) && Store::config.views.showThumbs)
            ThumbsLoader::queueFile(file);

        if ((ThumbsLoader::hasThumb(file) && !m_it->hasData(file) && Store::config.views.showThumbs) && !isWorking())
            m_it->queueFile(file, ThumbsLoader::thumb(file));

        if (m_it->hasNameData(icon.name()))
            return QPixmap::fromImage(m_it->flowNameData(icon.name(), role == FlowRefl));
        else if (!icon.name().isEmpty())
            m_it->queueName(icon);
        else
        {
            QImage img = icon.pixmap(SIZE).toImage();
            m_it->queueFile(file, img);
        }
        return QPixmap();
    }

    if (role == Category)
        return node->category();

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    return node->data(col);
}

bool
Model::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    Node *node = fromIndex(index);
    const QString &newName = value.toString();
    const QString &oldName = node->fileName();
    const QString &path = node->path();
    if (node->rename(newName))
    {
        emit fileRenamed(path, oldName, newName);
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

QVariant
Model::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return QVariant();
    if (role == Qt::DisplayRole)
        switch (section)
        {
        case 0: return tr("Name"); break;
        case 1: return tr("Size"); break;
        case 2: return tr("Type"); break;
        case 3: return tr("Last Modified"); break;
        case 4: return tr("Permissions"); break;
        default: break;
        }
    else if (role == Qt::TextAlignmentRole)
        return bool(section == 1) ? int(Qt::AlignRight|Qt::AlignVCenter) : int(Qt::AlignLeft|Qt::AlignVCenter);
    return QVariant();
}

QMimeData
*Model::mimeData(const QModelIndexList &indexes) const
{
    QList<QUrl> urls;
    for (int i = 0; i < indexes.count(); ++i)
    {
        const QModelIndex &index = indexes.at(i);
        if (!index.isValid() || index.column())
            continue;
        Node *node = fromIndex(index);
        urls << QUrl::fromLocalFile(node->filePath());
    }
    QMimeData *data = new QMimeData();
    data->setUrls(urls);
    return data;
}

int
Model::rowCount(const QModelIndex &parent) const
{
    return nodeForIndex(parent)->childCount();
}

Node
*Model::fromIndex(const QModelIndex &index) const
{
    return nodeForIndex(index);
}

Node
*Model::nodeForIndex(const QModelIndex &index) const
{
    Node *node = static_cast<Node *>(index.internalPointer());
    if (index.isValid() && index.row() < 100000 && index.column() < 5 && node)
        return node;
    return m_rootNode;
}

QModelIndex
Model::index(const QUrl &url)
{
    Node *node = rootNode()->fromUrl(url);
    if (node)
        return createIndex(node->row(), 0, node);
    return QModelIndex();
}

QModelIndex
Model::index(int row, int column, const QModelIndex &parent) const
{
    Node *parentNode = nodeForIndex(parent);
    Node *childNode = parentNode->child(row);

    if (childNode)
        return createIndex(row, column, childNode);
    return QModelIndex();
}

QModelIndex
Model::parent(const QModelIndex &child) const
{
    Node *childNode = nodeForIndex(child);
    if (Node *parentNode = childNode->parent())
        return createIndex(parentNode->row(), 0, parentNode);
    return QModelIndex();
}

bool
Model::hasChildren(const QModelIndex &parent) const
{
    if (Node *node = fromIndex(parent))
        return node->isDir();
    return true;
}

bool
Model::canFetchMore(const QModelIndex &parent) const
{
    if (Node *node = fromIndex(parent))
        return node->isDir()&&!node->isSearchResult();
    return true;
}

void
Model::fetchMore(const QModelIndex &parent)
{
    if (parent.isValid() && parent.column() != 0)
        return;
    Node *node = fromIndex(parent);
    if (node && !node->isPopulated())
        dataGatherer()->populateNode(node);
}

void
Model::sort(int column, Qt::SortOrder order)
{
    if (rootPath().isEmpty() || isWorking())
        return;
    const bool orderChanged = bool(m_sortColumn!=column||m_sortOrder!=order);
    m_sortColumn = column;
    m_sortOrder = order;
    qDebug() << "sort called" << rootPath() << column << order;
    emit layoutAboutToBeChanged();
    const QModelIndexList &oldList = persistentIndexList();
    QList<QPair<int, Node *> > old;
    for (int i = 0; i < oldList.count(); ++i)
        old << QPair<int, Node *>(oldList.at(i).column(), fromIndex(oldList.at(i)));

    rootNode()->sort();

    QModelIndexList newList;
    for (int i = 0; i < old.count(); ++i)
    {
        QPair<int, Node *> n = old.at(i);
        Node *node = n.second;
        QModelIndex idx = index(QUrl::fromLocalFile(node->filePath()));
        if (n.first > 0)
            idx = idx.sibling(idx.row(), n.first);
        newList << idx;
    }
    changePersistentIndexList(oldList, newList);
    if (orderChanged)
        emit sortingChanged(column, (int)order);
    emit layoutChanged();

#ifdef Q_WS_X11
    if (Store::config.views.dirSettings && orderChanged)
    {
        QDir dir(rootPath());
        QSettings settings(dir.absoluteFilePath(".directory"), QSettings::IniFormat);
        settings.beginGroup("DFM");
        QVariant varCol = settings.value("sortCol");
        if (varCol.isValid())
        {
            int col = varCol.value<int>();
            if (col != column)
                settings.setValue("sortCol", column);
        }
        else
            settings.setValue("sortCol", column);
        QVariant varOrd = settings.value("sortOrd");
        if (varCol.isValid())
        {
            Qt::SortOrder ord = (Qt::SortOrder)varOrd.value<int>();
            if (ord != order)
                settings.setValue("sortOrd", order);
        }
        else
            settings.setValue("sortOrd", order);
        settings.endGroup();
    }
#endif
}

void
Model::setHiddenVisible(bool visible)
{
    if (visible == m_showHidden)
        return;

    m_showHidden = visible;
    m_rootNode->setHiddenVisible(showHidden());
    emit hiddenVisibilityChanged(m_showHidden);
}

bool
Model::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (!data->hasUrls())
        return false;

    Node *node = fromIndex(parent);
    if (!node->isDir())
        return false;

    IO::Manager::copy(data->urls(), node->filePath(), true, true);
    return true;
}

QModelIndex
Model::mkdir(const QModelIndex &parent, const QString &name)
{
    if (parent.isValid())
    {
        Node *node = nodeForIndex(parent);
        QDir dir(node->filePath());
        dir.mkdir(name);
        return index(QUrl::fromLocalFile(dir.absoluteFilePath(name)));
    }
    return QModelIndex();
}

QUrl
Model::url(const QModelIndex &index) const
{
    return nodeForIndex(index)->url();
}

void
Model::startWorking()
{
    QMutexLocker locker(&m_mutex);
    m_isPopulating = true;
}

void
Model::endWorking()
{
    QMutexLocker locker(&m_mutex);
    m_isPopulating = false;
}

QModelIndexList
Model::category(const QString &cat)
{
    QModelIndexList categ;
    const QModelIndex &parent = index(m_url);
    if (!parent.isValid())
        return categ;
    const int count = rowCount(parent);
    for (int i = 0; i<count; ++i)
    {
        const QModelIndex &child = index(i, 0, parent);
        if (!child.isValid())
            continue;
        if (nodeForIndex(child)->category() == cat)
            categ << child;
    }
    return categ;
}

QModelIndexList
Model::category(const QModelIndex &fromCat)
{
    return category(nodeForIndex(fromCat)->category());
}

QStringList
Model::categories()
{
    const QModelIndex &parent = index(m_url);
//    if (!parent.isValid())
//        return QStringList();
    const int count = rowCount(parent);
    QStringList cats;
    for (int i = 0; i<count; ++i)
    {
        const QModelIndex &child = index(i, 0, parent);
        if (!child.isValid())
            continue;

        if (Node *node = nodeForIndex(child))
            if (!cats.contains(node->category()))
                cats << node->category();
    }
    return cats;
}

void
Model::search(const QString &fileName)
{
    search(fileName, m_url.toLocalFile());
}

void
Model::search(const QString &fileName, const QString &filePath)
{
    QUrl url;
    url.setScheme("search");
    Node *node = rootNode()->fromUrl(url);
    setUrl(url);
    dataGatherer()->search(fileName, filePath, node);
}

void
Model::endSearch()
{
//    Node *node = schemeNode("search");
    setUrl(QUrl::fromLocalFile(m_rootPath));
}

void
Model::cancelSearch()
{
    dataGatherer()->setCancelled(true);
}

QString
Model::currentSearchString()
{
    QUrl url;
    url.setScheme("search");
    Node *node = rootNode()->fromUrl(url);
    return node?node->filter():QString();
}

void
Model::schemeFromSchemeMenu()
{
    QAction *action = static_cast<QAction *>(sender());
    if (!action)
        return;

    QUrl url;
    url.setScheme(action->text());
    setUrl(url);
}

QString
Model::fileName(const QModelIndex &index) const
{
    return nodeForIndex(index)->name();
}

bool Model::isDir(const QModelIndex &index) const { return index.isValid()?fromIndex(index)->isDir():false; }
QFileInfo Model::fileInfo(const QModelIndex &index) const { return index.isValid()?*fromIndex(index):QFileInfo(); }

Node *Model::rootNode() { return m_rootNode; }

void
Model::removeDeletedLater()
{
//    QTimer::singleShot(500, this, SLOT(removeDeleted()));
}

void
Model::removeDeleted()
{
//    qDeleteAll(s_deletedNodes);
//    s_deletedNodes.clear();
}

QString
Model::title(const QUrl &url)
{
    QUrl u = url;
    if (u.isEmpty())
        u = m_url;

    if (u.path().isEmpty())
        return u.scheme();

    if (u.isLocalFile())
        return QFileInfo(url.toLocalFile()).fileName();

    return u.toString();
}
