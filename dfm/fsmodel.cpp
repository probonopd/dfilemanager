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
#include <QDesktopServices>
#include <QFileSystemWatcher>
#include <QMessageBox>
#include <QPainter>
#include <QAbstractItemView>
#include <QDir>
#include <QTimer>
#include <QMimeData>
#include <QSettings>
#include <QDateTime>
#include <QUrl>
#include <QLabel>
#include <QDebug>
#include <QMap>
#include <QWaitCondition>
#include <QMenu>
#include <QApplication>
#if !defined(QT_NO_DBUS)
#include <QDBusMessage>
#include <QDBusConnection>
#endif
#include "fsmodel.h"
#include "fsnode.h"
#include "iojob.h"
#include "dataloader.h"
#include "mainwindow.h"
#include "dataloader.h"
#include "fsworkers.h"
#include "helpers.h"
#include "config.h"

using namespace DFM;
using namespace FS;

static FileIconProvider *s_instance = 0;

FileIconProvider
*FileIconProvider::instance()
{
    if (!s_instance)
        s_instance = new FileIconProvider();
    return s_instance;
}

QIcon
FileIconProvider::fileIcon(const QFileInfo &fileInfo)
{
    return instance()->icon(fileInfo);
}

QIcon
FileIconProvider::typeIcon(IconType type)
{
    return instance()->icon(type);
}

QIcon
FileIconProvider::icon(IconType type) const
{
    return QFileIconProvider::icon(type);
}

QIcon
FileIconProvider::icon(const QFileInfo &i) const
{
    if (!i.exists())
        return QFileIconProvider::icon(QFileIconProvider::File);
//    if (i.suffix().contains(QRegExp("r[0-9]{2}"))) //splitted rar files .r00, r01 etc...
//        i.setFile(QString("%1%2.rar").arg(i.filePath(), i.baseName()));
    const QIcon &icon = QFileIconProvider::icon(i);
    if (icon.isNull())
        qDebug() << "failed to locate icon for file" << i.filePath();
    return QIcon::fromTheme(icon.name(), icon);
}

//-----------------------------------------------------------------------------

Model::Model(QObject *parent)
    : QAbstractItemModel(parent)
    , m_rootNode(new Node(this))
    , m_showHidden(false)
    , m_sortOrder(Qt::AscendingOrder)
    , m_sortColumn(0)
    , m_watcher(new QFileSystemWatcher(this))
    , m_dataGatherer(new Worker::Gatherer(this))
    , m_lockHistory(false)
    , m_schemeMenu(new QMenu())
    , m_current(0)
    , m_currentRoot(0)
    , m_timer(new QTimer(this))
{
    connect(DDataLoader::instance(), SIGNAL(newData(QString)), this, SLOT(newData(QString)));
    connect(m_watcher, SIGNAL(directoryChanged(QString)), this, SLOT(dirChanged(QString)));
    connect(m_dataGatherer, SIGNAL(nodeGenerated(QString,Node*)), this, SLOT(nodeGenerated(QString,Node*)));
    connect(this, SIGNAL(fileRenamed(QString,QString,QString)), DDataLoader::instance(), SLOT(fileRenamed(QString,QString,QString)));
//    connect(m_timer, SIGNAL(timeout()), this, SLOT(refreshCurrent()));
    connect(DDataLoader::instance(), SIGNAL(noLongerExists(QString)), this, SLOT(fileDeleted(QString)));
    connect(Devices::instance(), SIGNAL(deviceAdded(Device*)), this, SLOT(updateFileNode()));
    connect(Devices::instance(), SIGNAL(deviceRemoved(Device*)), this, SLOT(updateFileNode()));
//    connect(this, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(rowsDeleted(QModelIndex,int,int)));
    connect(this, SIGNAL(deleteNodeLater(Node*)), this, SLOT(deleteNode(Node*)));
    schemeNode("file")->rePopulate();
}

Model::~Model()
{
    m_dataGatherer->setCancelled(true);
    m_dataGatherer->wait();
    delete m_schemeMenu;
    delete m_rootNode;
}

void
Model::getSort(const QUrl &url)
{
    if (!url.isLocalFile())
        return;
    int sc = sortColumn();
    Qt::SortOrder so = sortOrder();
    Ops::getSorting(url.toLocalFile(), sc, so);
    if (sc != sortColumn() || so != sortOrder())
        setSort(sc, so);
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
    if (node == m_rootNode)
        return;

    m_current = node;
    emit urlChanged(QUrl::fromLocalFile(path));
    m_dataGatherer->populateNode(node);
}

bool
Model::handleFileUrl(QUrl &url, int &hasUrlReady)
{
    QString file = url.toLocalFile();
    if (file.endsWith(":")) //windows drive...
        file.append("/");

    const QFileInfo fi(file);
    if (fi.isDir())
        url = QUrl::fromLocalFile(file);
    if (!fi.exists())
        return false;

    Node *sNode = schemeNode(url.scheme());
    Node *node = sNode->localNode(file);

    if (!fi.isDir() && node)
    {
        node->exec();
        return false;
    }
    if (!m_watcher->directories().isEmpty())
        m_watcher->removePaths(m_watcher->directories());
    watchDir(file);
    m_currentRoot = sNode;
    if (!node)
        dataGatherer()->generateNode(file, sNode);
    else
        nodeGenerated(file, node);
    hasUrlReady = 0;
    return true;
}

bool
Model::handleSearchUrl(QUrl &url, int &hasUrlReady)
{
    QString searchPath = url.path();
#if defined(ISWINDOWS)
    if (!searchPath.isEmpty())
        while (searchPath.startsWith("/"))
            searchPath.remove(0, 1);
#endif

#if QT_VERSION < 0x050000
    const QString &searchName = url.encodedQuery();
#else
    const QString &searchName = url.query();
#endif

    Node *sNode = schemeNode("search");
    if (searchName.isEmpty())
        return false;

    sNode->clearVisible();
    const QFileInfo fi(searchPath);
    if (!fi.exists())
        return false;

    m_currentRoot = sNode;
    m_current = new Node(this, url, sNode);
    m_dataGatherer->search(searchName, fi.filePath(), m_current);
    hasUrlReady = 1;
    return true;
}

bool
Model::handleApplicationsUrl(QUrl &url, int &hasUrlReady)
{
#if defined(ISUNIX)
    m_currentRoot = m_current = schemeNode(url.scheme());
    hasUrlReady = 1;
    m_dataGatherer->populateApplications("/usr/share/applications", m_current);
    return true;
#endif
    return false;
}

bool
Model::handleDevicesUrl(QUrl &url, int &hasUrlReady)
{
    Q_UNUSED(url)
#if defined(ISUNIX)
    m_currentRoot = m_current = schemeNode("file");
    hasUrlReady = 2;
    return true;
#endif
    return false;
}

bool
Model::handleTrashUrl(QUrl &url, int &hasUrlReady)
{
#if defined(ISUNIX)
    m_currentRoot = m_current = schemeNode(url.scheme());
    hasUrlReady = 1;
    dataGatherer()->populateNode(m_current);
    return true;
#endif
    return false;
}

bool (Model::*Model::getUrlHandler(const QUrl &url))(QUrl &, int &)
{
    if (url.scheme() == "file")
        return &Model::handleFileUrl;
    else if (url.scheme() == "search")
        return &Model::handleSearchUrl;
    else if (url.scheme() == "applications")
        return &Model::handleApplicationsUrl;
    else if (url.scheme() == "devices")
        return &Model::handleDevicesUrl;
    else if (url.scheme() == "trash")
        return &Model::handleTrashUrl;
    else
        return 0;
}

bool
Model::setUrl(QUrl url)
{
    if (m_url == url)
        return false;

    if (url.scheme().isEmpty())
        url = QUrl::fromLocalFile(url.toString());

    bool (Model::*urlHandler)(QUrl &, int &)(getUrlHandler(url));
    int isReady(0);
    if (urlHandler && (this->*urlHandler)(url, isReady))
    {
        DDataLoader::clearQueue();
        m_url = url;
        m_history[Back] << m_url;
        if (!m_lockHistory)
            m_history[Forward].clear();
        if (isReady>0)
            emit urlChanged(m_url);
        if (isReady>1)
            emit urlLoaded(m_url);
        return true;
    }
    else
        qDebug() << "unsupported url" << url;
    return false;
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
Model::goBack()
{
    if (m_history[Back].count()<2)
        return;
    if (m_history[Back].last() == m_url)
        m_history[Forward] << m_history[Back].takeLast(); //current location to forward
    const QUrl &backUrl = m_history[Back].takeLast();
    if (backUrl.isLocalFile() && !backUrl.path().isEmpty())
        if (!QFileInfo(backUrl.toLocalFile()).exists())
        {
            goBack();
            return;
        }
    m_lockHistory=true;
    setUrl(backUrl);
    m_lockHistory=false;
}

void
Model::goForward()
{
    if (!m_history[Forward].isEmpty())
    {
        m_lockHistory=true;
        setUrl(m_history[Forward].takeLast());
        m_lockHistory=false;
    }
}

void
Model::refresh()
{
    if (m_url.scheme()!="file")
        return;
    Node *node = schemeNode("file")->localNode(m_url.toLocalFile());
    dataGatherer()->populateNode(node);
}

void
Model::refresh(const QString &path)
{
    Node *node = schemeNode("file")->localNode(path);
    dataGatherer()->populateNode(node);
}

void
Model::updateFileNode()
{
    schemeNode("file")->rePopulate();
}

void
Model::dirChanged(const QString &path)
{
    if (QFileInfo(path).exists())
    {
        refresh(path);
        return;
    }
    emit directoryRemoved(path);
    if (Node *n = schemeNode("file")->localNode(path))
    {
        if (n == m_current)
            m_current = 0;
        if (Node *p = n->parent())
            dataGatherer()->populateNode(p);
    }
}

void
Model::refreshCurrent()
{
    m_timer->stop();
    if (!m_current || !m_current->url().isLocalFile())
        return;
    dataGatherer()->populateNode(m_current);
}

void
Model::watchDir(const QString &path)
{
    if (!m_watcher->directories().contains(path))
        m_watcher->addPath(path);
}

void
Model::unWatchDir(const QString &path)
{
    if (m_watcher->directories().contains(path))
        m_watcher->removePath(path);
}

Qt::ItemFlags
Model::flags(const QModelIndex &index) const
{
    Node *n = node(index);
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    if (n->isWritable()) flags |= Qt::ItemIsEditable;
    if (n->isDir()) flags |= Qt::ItemIsDropEnabled;
    if (n->isReadable() && !isWorking()) flags |= Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
    return flags;
}

void
Model::newData(const QString &file)
{
    const QModelIndex &idx = index(QUrl::fromLocalFile(file));
    if (idx.isValid())
    {
        emit dataChanged(idx, idx);
        for (int i = 1; i<columnCount(); ++i)
        {
            const QModelIndex &sibling = idx.sibling(idx.row(), i);
            emit dataChanged(sibling, sibling);
        }
    }
    else
    {
        const QModelIndex &parent = index(m_url);
        for (int i = 0; i < rowCount(index(m_url)); ++i)
        {
            const QModelIndex &idx = index(i, 0, parent);
            if (idx.data(FS::FilePathRole).toString() == file)
            {
                emit dataChanged(idx, idx);
                return;
            }
        }
    }
}

QVariant
Model::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()||index.column()>3||index.row()>100000)
        return QVariant();

    Node *n = node(index);
    const int col = index.column();

    if (n == m_rootNode || n == m_currentRoot || (role == FileIconRole && col > 0))
        return QVariant();

    switch (role)
    {
    case FilePathRole:
        return n->filePath();
    case FilePermissionsRole:
        return n->permissionsString();
    case FileIconRole:
        return n->icon();
    case FileIsDirRole:
        return n->isDir();
    case FileHasThumbRole:
    {
        if (Data *d = DDataLoader::data(n->filePath(), true))
            return !d->thumb.isNull();
        return false;
    }
    case Qt::TextAlignmentRole:
        return bool(col == 1) ? int(Qt::AlignVCenter|Qt::AlignRight) : int(Qt::AlignLeft|Qt::AlignVCenter);
    case CategoryRole:
        return n->category();
    case MimeTypeRole:
        return n->mimeType();
    case FileTypeRole:
        return n->fileType();
    case OwnderRole:
        return n->owner();
    case UrlRole:
        return n->url();
    case LastModifiedRole:
        return n->lastModified().toString();
    default: break;
    }

    if (!isWorking())
    if (role == Qt::FontRole && !col)
    {
        QFont f(qApp->font());
        f.setItalic(n->isSymLink());
        f.setUnderline(n->isExec());
        return f;
    }

    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return n->data(col);

    return QVariant();
}

bool
Model::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    switch (role)
    {
    case WatchDirectoryRole:
    {
        watchDir(value.toString());
        return true;
    }
    case Qt::EditRole:
    {
        Node *n = node(index);
        const QString &newName = value.toString();
        const QString &oldName = n->fileName();
        const QString &path = n->path();
        if (n->rename(newName))
        {
            emit fileRenamed(path, oldName, newName);
            emit dataChanged(index, index);
            return true;
        }
        else
            QMessageBox::warning(MainWindow::currentWindow(), "Failed to rename", QString("%1 to %2").arg(n->name(), newName));
    }
    default: break;
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
        Node *n = node(index);
        urls << QUrl::fromLocalFile(n->filePath());
    }
    QMimeData *data = new QMimeData();
    data->setUrls(urls);
    return data;
}

Node
*Model::node(const QModelIndex &index) const
{
    Node *node = static_cast<Node *>(index.internalPointer());
    if (index.isValid() && index.row() < 0xffffffff && index.column() < 5 && node)
        return node;
    return m_currentRoot?m_currentRoot:m_rootNode;
}

QModelIndex
Model::index(const QUrl &url)
{
    //QMutexLocker locker(&m_mutex);
    if (url.scheme().isEmpty())
        return QModelIndex();

    if (m_current)
    {
        if (url == m_url && m_current->url() == url)
            return createIndex(m_current->row(), 0, m_current);
        if (Node *n = m_current->childFromUrl(url))
            return createIndex(n->row(), 0, n);
    }
    if (url.isLocalFile())
    {
        Node *node = schemeNode("file")->localNode(url.toLocalFile());
        if (node == m_currentRoot || node == m_rootNode)
            return QModelIndex();
        if (node)
            return createIndex(node->row(), 0, node);
        return QModelIndex();
    }

    if (m_nodes.contains(url))
    {
        Node *node = m_nodes.value(url);
        return createIndex(node->row(), 0, node);
    }
    return QModelIndex();
}

QModelIndex
Model::index(int row, int column, const QModelIndex &parent) const
{
    //QMutexLocker locker(&m_mutex);
    Node *parentNode = node(parent);
    Node *childNode = parentNode->child(row);
    if (childNode)
    {
        if (childNode == m_currentRoot)
            return QModelIndex();
        return createIndex(row, column, childNode);
    }
    return QModelIndex();
}

QModelIndex
Model::parent(const QModelIndex &child) const
{
    //QMutexLocker locker(&m_mutex);
    if (!child.isValid())
        return QModelIndex();
    Node *childNode = node(child);
    if (Node *parentNode = childNode->parent())
    {
        if (parentNode == m_currentRoot || parentNode == m_rootNode)
            return QModelIndex();
        return createIndex(parentNode->row(), 0, parentNode);
    }
    return QModelIndex();
}

int
Model::rowCount(const QModelIndex &parent) const
{
    //QMutexLocker locker(&m_mutex);
    return node(parent)->childCount();
}

bool
Model::hasChildren(const QModelIndex &parent) const
{
    //QMutexLocker locker(&m_mutex);
    Node *n = node(parent);
    return n->hasChildren()||n->isDir();
}

bool
Model::canFetchMore(const QModelIndex &parent) const
{
    //QMutexLocker locker(&m_mutex);
    return node(parent)->isDir();
}

void
Model::fetchMore(const QModelIndex &parent)
{
    //QMutexLocker locker(&m_mutex);
    if (parent.isValid() && parent.column() != 0)
        return;
    Node *n = node(parent);
    if (n && !n->isPopulated() && n->exists())
        dataGatherer()->populateNode(n);
}

void
Model::sortNode(Node *n)
{
    if (!n)
        n = m_currentRoot;
    if (!n)
        n = m_rootNode;
    emit layoutAboutToBeChanged();
    const QModelIndexList &oldList = persistentIndexList();
    QList<QPair<int, Node *> > old;
    for (int i = 0; i < oldList.count(); ++i)
        old << QPair<int, Node *>(oldList.at(i).column(), node(oldList.at(i)));

    n->sort();

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
    emit layoutChanged();
}

void
Model::sort(int column, Qt::SortOrder order)
{
    if (isWorking() || m_url.isEmpty())
        return;
    const bool orderChanged = bool(m_sortColumn!=column||m_sortOrder!=order);
    m_sortColumn = column;
    m_sortOrder = order;
//    qDebug() << "sort called" << m_url << column << order;
    sortNode();
    if (orderChanged)
        emit sortingChanged(column, (int)order);

#if defined(HASX11)
    if (Store::config.views.dirSettings && orderChanged && m_url.isLocalFile())
    {
        QDir dir(m_url.toLocalFile());
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
    emit layoutAboutToBeChanged();
    schemeNode("file")->setHiddenVisible(visible);
    emit layoutChanged();
    emit hiddenVisibilityChanged(visible);
}

bool
Model::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Node *n = node(parent);
    if (!n->isDir())
        return false;
#if !defined(QT_NO_DBUS)
    //ark extract
    if (data->hasFormat("application/x-kde-ark-dndextract-service") && data->hasFormat("application/x-kde-ark-dndextract-path"))
    {
        const QString dBusPath(data->data("application/x-kde-ark-dndextract-path"));
        const QString dBusService(data->data("application/x-kde-ark-dndextract-service"));
        QDBusMessage message = QDBusMessage::createMethodCall(dBusService, dBusPath, "org.kde.ark.DndExtract", "extractSelectedFilesTo");
        message << n->filePath();
        QDBusConnection::sessionBus().send(message);
        return true;
    }
#endif
    if (!data->hasUrls())
        return false;

    QMenu m;
    QAction *move = m.addAction(QString("move to %1").arg(n->filePath()));
    m.addAction(QString("copy to %1").arg(n->filePath()));
    if (QAction *a = m.exec(QCursor::pos()))
    {
        const bool cut(a == move);
        IO::Manager::copy(data->urls(), n->filePath(), cut, cut);
    }
    return true;
}

QModelIndex
Model::mkdir(const QModelIndex &parent, const QString &name)
{
    if (parent.isValid())
    {
        Node *n = node(parent);
        QDir dir(n->filePath());
        if (!dir.isAbsolute())
            return QModelIndex();
        m_watcher->blockSignals(true);
        if (!dir.mkdir(name))
            QMessageBox::warning(static_cast<QWidget *>(this->QObject::parent()), "There was an error", QString("Could not create folder in %2").arg(dir.path()));
        n->rePopulate();
        m_watcher->blockSignals(false);
        return index(QUrl::fromLocalFile(dir.absoluteFilePath(name)));
    }
    return QModelIndex();
}

QUrl
Model::url(const QModelIndex &index) const
{
    return node(index)->url();
}

QModelIndexList
Model::category(const QString &cat)
{
    QModelIndexList categ;
    const QModelIndex &parent = index(m_url);
    const int count = rowCount(parent);
    for (int i = 0; i < count; ++i)
    {
        const QModelIndex &child = index(i, 0, parent);
        if (!child.isValid())
            continue;
        if (node(child)->category() == cat)
            categ << child;
    }
    return categ;
}

QModelIndexList
Model::category(const QModelIndex &fromCat)
{
    return category(node(fromCat)->category());
}

QStringList
Model::categories()
{
    const QModelIndex &parent = index(m_url);
    const int count = rowCount(parent);
    QStringList cats;
    for (int i = 0; i < count; ++i)
    {
        const QModelIndex &child = index(i, 0, parent);
        if (!child.isValid())
            continue;

        if (Node *n = node(child))
            if (!cats.contains(n->category()))
                cats << n->category();
    }
    return cats;
}

void
Model::exec(const QModelIndex &index)
{
    node(index)->exec();
}

void
Model::setFilter(const QString &filter, const QString &path)
{
    emit layoutAboutToBeChanged();
    if (!path.isEmpty())
    {
        if (Node *n = m_currentRoot->localNode(path))
            n->setFilter(filter);
    }
    else if (m_current)
        m_current->setFilter(filter);
    emit layoutChanged();
}

QString
Model::filter(const QString &path) const
{
    if (!path.isEmpty())
    {
        if (Node *n = m_currentRoot->localNode(path))
            return n->filter();
    }
    else if (m_current)
        return m_current->filter();
    return QString();
}

void
Model::search(const QString &fileName)
{
    QUrl url;
    url.setScheme("search");
    QString searchPath = m_url.toLocalFile();
    if (searchPath.isEmpty())
        searchPath = m_dataGatherer->m_searchPath;
    if (searchPath.isEmpty())
        return;
    url.setPath(searchPath);
#if QT_VERSION < 0x050000
    url.setEncodedQuery(fileName.toLocal8Bit());
#else
    url.setQuery(fileName);
#endif
    setUrl(url);
}

void
Model::endSearch()
{
    if (!m_url.isLocalFile())
        setUrl(QUrl::fromLocalFile(m_dataGatherer->m_searchPath));
}

void
Model::cancelSearch()
{
    dataGatherer()->setCancelled(true);
}

bool
Model::isSearching() const
{
    if (m_current)
        return m_current->scheme() == "search";
    if (m_currentRoot)
        return m_currentRoot->scheme() == "search";
    return false;
}

QString
Model::searchString() const
{
    return m_dataGatherer->m_searchName;
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
    return node(index)->name();
}

bool Model::isDir(const QModelIndex &index) const
{
    return node(index)->isDir();
}
QFileInfo Model::fileInfo(const QModelIndex &index) const
{
    return *node(index);
}

Node *Model::rootNode() const { return m_currentRoot; }

bool Model::isWorking() const { return m_dataGatherer->isRunning(); }

QString
Model::title(const QUrl &url) const
{
    QUrl u = url;
    if (u.isEmpty())
        u = m_url;

    if (u.path().isEmpty())
        return u.scheme();

    if (u.isLocalFile())
    {
        const QString &path = url.toLocalFile();
        QString title = QFileInfo(path).fileName();
        if (title.isEmpty())
            title = path;
        return title;
    }
    return u.toString();
}

Node
*Model::schemeNode(const QString &scheme)
{
    if (scheme.isEmpty())
        return m_rootNode;
    if (m_schemeNodes.contains(scheme))
        return m_schemeNodes.value(scheme);

    QUrl url;
    url.setScheme(scheme);

    Node *node = 0;
    if (scheme.toLower() == "trash")
        node = new Node(this, url, m_rootNode, DTrash::trashPath(DTrash::TrashFiles));
    else
        node = new Node(this, url, m_rootNode);
    m_schemeMenu->addAction(scheme, this, SLOT(schemeFromSchemeMenu()));
    m_schemeNodes.insert(scheme, node);
    return node;
}

void
Model::fileDeleted(const QString &path)
{
    Node *n = schemeNode("file")->localNode(path);
    if (!n)
        return;
    if (Node *p = n->parent())
        m_dataGatherer->populateNode(p);
}

void
Model::deleteNode(Node *node)
{
    delete node;
}
