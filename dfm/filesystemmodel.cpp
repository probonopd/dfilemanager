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

#include "filesystemmodel.h"
#include "iojob.h"
#include "thumbsloader.h"
#include "mainwindow.h"
#include "viewcontainer.h"

using namespace DFM;
using namespace FS;

static FileIconProvider *s_instance = 0;

FileIconProvider::FileIconProvider() : QFileIconProvider()
{
}

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
    return QFileIconProvider::icon(i);
}

//-----------------------------------------------------------------------------

Model::Model(QObject *parent)
    : QAbstractItemModel(parent)
    , m_rootNode(new Node(this))
    , m_showHidden(false)
    , m_sortOrder(Qt::AscendingOrder)
    , m_sortColumn(0)
    , m_it(new ImagesThread(this))
    , m_watcher(new QFileSystemWatcher(this))
    , m_container(0)
    , m_dataGatherer(new Worker::Gatherer(this))
    , m_goingBack(false)
    , m_schemeMenu(new QMenu())
    , m_current(0)
    , m_timer(new QTimer(this))
{
    if (ViewContainer *vc = qobject_cast<ViewContainer *>(parent))
        m_container = vc;

    connect (ThumbsLoader::instance(), SIGNAL(thumbFor(QString,QString)), this, SLOT(thumbFor(QString,QString)));
    connect (m_it, SIGNAL(imagesReady(QString)), this, SLOT(flowDataAvailable(QString)));
    connect (this, SIGNAL(urlChanged(QUrl)), m_it, SLOT(clearData()));
    connect (m_watcher, SIGNAL(directoryChanged(QString)), this, SLOT(dirChanged(QString)));
    connect (m_dataGatherer, SIGNAL(nodeGenerated(QString,Node*)), this, SLOT(nodeGenerated(QString,Node*)));
    connect(this, SIGNAL(fileRenamed(QString,QString,QString)), ThumbsLoader::instance(), SLOT(fileRenamed(QString,QString,QString)));
    connect (m_timer, SIGNAL(timeout()), this, SLOT(refreshCurrent()));
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

    emit urlChanged(QUrl::fromLocalFile(path));
    m_current = node;
    m_dataGatherer->populateNode(node);
}

void
Model::setUrl(const QUrl &url)
{
    if (m_url == url)
        return;

    if (isWorking())
    {
        m_dataGatherer->setCancelled(true);
        m_dataGatherer->wait();
    }

    setFilter(QString());
    m_current = 0;

    m_history[Back] << url;
    if (!m_goingBack)
        m_history[Forward].clear();
    m_url = url;

    ThumbsLoader::clearQueue();
    if (!m_watcher->directories().isEmpty())
        m_watcher->removePaths(m_watcher->directories());

    if (url.isLocalFile() && QFileInfo(url.toLocalFile()).exists())
    {
        const QString &file = url.toLocalFile();
        m_watcher->addPath(file);
        Node *sNode = schemeNode(url.scheme());
        Node *node = sNode->localNode(file);
        if (!node)
            dataGatherer()->generateNode(file, sNode);
        else
            nodeGenerated(file, node);
    }
    else if (url.scheme() == "search")
    {
        QString searchPath = url.path();
#if !defined(Q_OS_UNIX)
        if (!searchPath.isEmpty())
            while (searchPath.startsWith("/"))
                searchPath.remove(0, 1);
#endif
#if QT_VERSION < 0x050000
        const QString &searchName = url.encodedQuery();
#else
        const QString &searchName = url.query();
#endif
        search(searchName, searchPath);
    }
#if defined(Q_OS_UNIX)
    else if (url.scheme() == "applications")
    {
        m_current = schemeNode(url.scheme());
        emit urlChanged(url);
        m_dataGatherer->populateApplications("/usr/share/applications", m_current);
    }
#endif
    else
    {
        emit urlChanged(url);
        emit urlLoaded(url);
    }
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
    const QUrl &forwardUrl = m_history[Back].takeLast();
    const QUrl &backUrl = m_history[Back].takeLast();
    m_goingBack=true;
    setUrl(backUrl);
    m_goingBack=false;
    m_history[Forward] << forwardUrl;
}

void
Model::goForward()
{
    if (!m_history[Forward].isEmpty())
        setUrl(m_history[Forward].takeLast());
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
Model::dirChanged(const QString &path)
{
    if (m_current->filePath() != path || !m_current->url().isLocalFile())
        return;

    m_timer->start(50);
}

void
Model::refreshCurrent()
{
    m_timer->stop();
    if (!m_current || !m_current->url().isLocalFile())
        return;
    dataGatherer()->populateNode(m_current);
}

Qt::ItemFlags
Model::flags(const QModelIndex &index) const
{
    Node *node = nodeFromIndex(index);
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    if (node->isWritable()) flags |= Qt::ItemIsEditable;
    if (node->isDir()) flags |= Qt::ItemIsDropEnabled;
    if (node->isReadable() && !isWorking()) flags |= Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
    return flags;
}

void
Model::forceEmitDataChangedFor(const QString &file)
{
    QModelIndex idx;
    if (m_url.isLocalFile())
        idx = indexForLocalFile(file);
    else
        idx = index(QUrl(QString("%1%2").arg(m_url.toString(), file)));

    if (idx.isValid())
    {
        emit dataChanged(idx, idx);
        emit flowDataChanged(idx, idx);
    }
}

void
Model::flowDataAvailable(const QString &file)
{
    QModelIndex idx;
    if (m_url.isLocalFile())
        idx = indexForLocalFile(file);
    else
        idx = index(QUrl(QString("%1%2").arg(m_url.toString(), file)));

    if (idx.isValid())
        emit flowDataChanged(idx, idx);
}

void
Model::thumbFor(const QString &file, const QString &iconName)
{
    QModelIndex idx;
    if (m_url.isLocalFile())
        idx = indexForLocalFile(file);
    else
        idx = index(QUrl(QString("%1%2").arg(m_url.toString(), file)));

    if (idx.isValid())
        emit dataChanged(idx, idx);
    if (m_container->currentViewType() == ViewContainer::Flow)
        if (!QFileInfo(file).isDir())
            m_it->queueFile(file, ThumbsLoader::thumb(file), true);
        else if (!iconName.isNull())
            m_it->queueName(QIcon::fromTheme(iconName));
}

bool Model::hasThumb(const QString &file) { return ThumbsLoader::instance()->hasThumb(file); }

bool
Model::hasThumb(const QModelIndex &index)
{
    Node *node = nodeFromIndex(index);
    if (node->exists())
        return hasThumb(node->filePath());
    return false;
}

QVariant
Model::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()||index.column()>3||index.row()>100000)
        return QVariant();

    Node *node = nodeFromIndex(index);
    const int col = index.column();
    if (node == m_rootNode || (role == FileIconRole && col > 0))
        return QVariant();

    if (role == FilePathRole)
        return node->filePath();
    if (role == FilePermissions)
        return node->permissionsString();
    if (role == FileIconRole)
        return node->icon();
    if (role == Qt::TextAlignmentRole)
        return bool(col == 1) ? int(Qt::AlignVCenter|Qt::AlignRight) : int(Qt::AlignLeft|Qt::AlignVCenter);

    if (role == Qt::FontRole && !col && !node->isDir())
    {
        QFont f(qApp->font());
        f.setItalic(node->isSymLink());
        f.setUnderline(node->isExec());
        return f;
    }

    if (role > FilePermissions && role < Category) //flow stuff
    {
        const QIcon &icon = node->icon();
        if (m_it->hasData(node->filePath()))
            return QPixmap::fromImage(m_it->flowData(node->filePath(), role == FlowRefl));

        if (!ThumbsLoader::hasThumb(node->filePath()) && Store::config.views.showThumbs)
            ThumbsLoader::queueFile(node->filePath());

        if ((ThumbsLoader::hasThumb(node->filePath()) && !m_it->hasData(node->filePath()) && Store::config.views.showThumbs) && !isWorking())
            m_it->queueFile(node->filePath(), ThumbsLoader::thumb(node->filePath()));

        if (m_it->hasNameData(icon.name()))
            return QPixmap::fromImage(m_it->flowNameData(icon.name(), role == FlowRefl));
        else if (!icon.name().isEmpty())
            m_it->queueName(icon);
        else
            m_it->queueFile(node->filePath(), icon.pixmap(SIZE).toImage());

        return QPixmap();
    }

    if (role == Category)
        return node->category();

    if (role == MimeType)
        return node->mimeType();

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    return node->data(col);
}

bool
Model::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    Node *node = nodeFromIndex(index);
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
        Node *node = nodeFromIndex(index);
        urls << QUrl::fromLocalFile(node->filePath());
    }
    QMimeData *data = new QMimeData();
    data->setUrls(urls);
    return data;
}

int
Model::rowCount(const QModelIndex &parent) const
{
    return nodeFromIndex(parent)->childCount();
}

Node
*Model::nodeFromIndex(const QModelIndex &index) const
{
    Node *node = static_cast<Node *>(index.internalPointer());
    if (index.isValid() && index.row() < 100000 && index.column() < 5 && node)
        return node;
    return m_rootNode;
}

QModelIndex
Model::indexForLocalFile(const QString &filePath)
{
    Node *node = schemeNode("file")->localNode(filePath);
    if (node)
        return createIndex(node->row(), 0, node);
    return QModelIndex();
}

QModelIndex
Model::index(const QUrl &url)
{
    if (url.scheme().isEmpty())
        return QModelIndex();

    if (url == m_url && m_current)
        return createIndex(m_current->row(), 0, m_current);

    Node *sNode = schemeNode(url.scheme());

    if (url.path().isEmpty())
        return createIndex(sNode->row(), 0, sNode);

    if (url.isLocalFile())
        return indexForLocalFile(url.toLocalFile());

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
    Node *parentNode = nodeFromIndex(parent);
    Node *childNode = parentNode->child(row);

    if (childNode)
        return createIndex(row, column, childNode);
    return QModelIndex();
}

QModelIndex
Model::parent(const QModelIndex &child) const
{
    Node *childNode = nodeFromIndex(child);
    if (Node * parentNode = childNode->parent())
        return createIndex(parentNode->row(), 0, parentNode);
    return QModelIndex();
}

bool
Model::hasChildren(const QModelIndex &parent) const
{
    Node *n = nodeFromIndex(parent);
    return n->hasChildren()||n->isDir();
}

bool
Model::canFetchMore(const QModelIndex &parent) const
{
    return nodeFromIndex(parent)->isDir();
}

void
Model::fetchMore(const QModelIndex &parent)
{
    if (parent.isValid() && parent.column() != 0)
        return;
    Node *node = nodeFromIndex(parent);
    if (node && !node->isPopulated() && node->exists())
        dataGatherer()->populateNode(node);
}

void
Model::sort(int column, Qt::SortOrder order)
{
    if (isWorking() || m_url.isEmpty())
        return;
    const bool orderChanged = bool(m_sortColumn!=column||m_sortOrder!=order);
    m_sortColumn = column;
    m_sortOrder = order;
    qDebug() << "sort called" << m_url << column << order;
    emit layoutAboutToBeChanged();
    const QModelIndexList &oldList = persistentIndexList();
    QList<QPair<int, Node *> > old;
    for (int i = 0; i < oldList.count(); ++i)
        old << QPair<int, Node *>(oldList.at(i).column(), nodeFromIndex(oldList.at(i)));

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
    if (!data->hasUrls())
        return false;

    Node *node = nodeFromIndex(parent);
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
        Node *node = nodeFromIndex(parent);
        QDir dir(node->filePath());
        dir.mkdir(name);
        return index(QUrl::fromLocalFile(dir.absoluteFilePath(name)));
    }
    return QModelIndex();
}

QUrl
Model::url(const QModelIndex &index) const
{
    return nodeFromIndex(index)->url();
}

QModelIndexList
Model::category(const QString &cat)
{
    QModelIndexList categ;
    const QModelIndex &parent = index(m_url);
    const int count = rowCount(parent);
    for (int i = 0; i<count; ++i)
    {
        const QModelIndex &child = index(i, 0, parent);
        if (!child.isValid())
            continue;
        if (nodeFromIndex(child)->category() == cat)
            categ << child;
    }
    return categ;
}

QModelIndexList
Model::category(const QModelIndex &fromCat)
{
    return category(nodeFromIndex(fromCat)->category());
}

QStringList
Model::categories()
{
    const QModelIndex &parent = index(m_url);
    const int count = rowCount(parent);
    QStringList cats;
    for (int i = 0; i<count; ++i)
    {
        const QModelIndex &child = index(i, 0, parent);
        if (!child.isValid())
            continue;

        if (Node *node = nodeFromIndex(child))
            if (!cats.contains(node->category()))
                cats << node->category();
    }
    return cats;
}

void
Model::exec(const QModelIndex &index)
{
    nodeFromIndex(index)->exec();
}

void
Model::setFilter(const QString &filter)
{
    if (!m_current)
        return;
    emit layoutAboutToBeChanged();
    m_current->setFilter(filter);
    emit layoutChanged();
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
Model::search(const QString &fileName, const QString &filePath)
{
    Node *sNode = schemeNode("search");
    if (fileName.isEmpty())
        return;

    sNode->clearVisible();
    m_current = new Node(this, m_url, sNode);
    emit urlChanged(m_url);
    dataGatherer()->search(fileName, filePath, m_current);
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

QString
Model::currentSearchString()
{
    return QString();
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
    return nodeFromIndex(index)->name();
}

QIcon
Model::fileIcon(const QModelIndex &index) const
{
    return nodeFromIndex(index)->icon();
}

bool Model::isDir(const QModelIndex &index) const
{
    return nodeFromIndex(index)->isDir();
}
QFileInfo Model::fileInfo(const QModelIndex &index) const
{
    return *nodeFromIndex(index);
}

Node *Model::rootNode() { return m_rootNode; }

bool Model::isWorking() const { return m_dataGatherer->isRunning(); }

QString
Model::title(const QUrl &url)
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
    Node *node = new Node(this, url, m_rootNode);
    m_schemeMenu->addAction(scheme, this, SLOT(schemeFromSchemeMenu()));
    m_schemeNodes.insert(scheme, node);
    return node;
}
