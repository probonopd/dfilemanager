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

#include <QDirIterator>
#include <QList>

#include "fsworkers.h"
#include "helpers.h"

using namespace DFM;
using namespace FS;


static bool lessThen(Node *n1, Node *n2)
{
    //dirs always first right?
    if (n1->isDir() && !n2->isDir())
        return true;
    else if (!n1->isDir() && n2->isDir())
        return false;

    //hidden... last?
    if (n1->isHidden() && !n2->isHidden())
        return false;
    else if (!n1->isHidden() && n2->isHidden())
        return true;

    bool lt = true;
    switch (n1->sortColumn())
    {
    case 0: lt = n1->name().toLower()<n2->name().toLower(); break; //name
    case 1: lt = n1->size()<n2->size(); break; //size
    case 2: //type
        if (n1->suffix().toLower()==n2->suffix().toLower())
            lt = n1->name().toLower()<n2->name().toLower();
        else
            lt = n1->suffix().toLower()<n2->suffix().toLower();
        break;
    case 3: lt = n1->lastModified()<n2->lastModified(); break; //lastModified
    case 4: lt = n1->permissions()<n2->permissions(); break; //permissions
    default: break;
    }
    return bool(n1->sortOrder())?!lt:lt;
}

Node::Node(Model *model, const QUrl &url, Node *parent, const QString &filePath)
    : QFileInfo(filePath)
    , m_isPopulated(false)
    , m_filePath(filePath)
    , m_filter(QString(""))
    , m_model(model)
    , m_parent(parent)
    , m_url(url)
{
    if (url.path().isEmpty() && !url.scheme().isEmpty())
        m_name = url.scheme();
    else if (!parent)
        m_name = "--";
    else if (fileName().isEmpty())
        m_name = filePath;
    else
        m_name = fileName();

    if (m_name.isEmpty())
        m_name = url.toEncoded(QUrl::RemoveScheme);

    if (exists())
    {
        m_iconName = m_mimeType = m_model->m_mimes.getMimeType(filePath);
        if (!m_iconName.isEmpty())
            m_iconName.replace("/", "-");
    }

    if (parent)
        parent->addChild(this);
}

Node::~Node()
{
    if (m_parent)
        m_parent->removeChild(this);
    if (m_model->m_nodes.contains(m_url))
        m_model->m_nodes.remove(m_url);
    for (int i = 0; i<ChildrenTypeCount; ++i)
        qDeleteAll(m_children[i]);
}

void
Node::removeChild(Node *node)
{
    for (int i = 0; i < ChildrenTypeCount; ++i)
    {
        m_mutex.lock();
        const bool hasNode = m_children[i].contains(node);
        m_mutex.unlock();
        if (hasNode)
        {
            if (!i)
                m_model->beginRemoveRows(m_model->createIndex(row(), 0, this), node->row(), node->row());
            m_mutex.lock();
            m_children[i].removeOne(node);
            m_mutex.unlock();
            if (!i)
                m_model->endRemoveRows();
        }
    }
}

void
Node::insertChild(Node *n, const int i)
{
    m_mutex.lock();
    m_children[Visible].insert(i, n);
    m_mutex.unlock();
}

void
Node::addChild(Node *node)
{
    if (!node->url().isLocalFile())
        m_model->m_nodes.insert(node->url(), node);

    if (node->isHidden() && !m_model->showHidden())
        m_children[Hidden] << node;
    else
    {
        int z = childCount(), i = -1;
        while (++i < z)
            if (lessThen(node, child(i)))
                break;
        m_model->beginInsertRows(m_model->createIndex(row(), 0, this), i, i);
        insertChild(node, i);
        m_model->endInsertRows();
    }
}

int
Node::childCount(Children children) const
{
    QMutexLocker locker(&m_mutex);
    return m_children[children].size();
}

Node
*Node::child(const int c, Children fromChildren)
{
    QMutexLocker locker(&m_mutex);
    if (c > -1 && c < m_children[fromChildren].size())
        return m_children[fromChildren].at(c);
    return 0;
}

bool
Node::hasChild(const QString &name, const bool nameIsPath)
{
    QMutexLocker locker(&m_mutex);
    for (int i = 0; i < ChildrenTypeCount; ++i)
        for (Nodes::const_iterator b = m_children[i].constBegin(), e = m_children[i].constEnd(); b!=e; ++b)
            if ((nameIsPath?(*b)->filePath():(*b)->name()) == name)
                return true;
    return false;
}

int
Node::row()
{
    if (!m_parent)
        return -1;

    return m_parent->rowOf(this);
}

int
Node::rowOf(Node *child) const
{
    QMutexLocker locker(&m_mutex);
    return m_children[Visible].indexOf(child);
}

QIcon
Node::icon()
{
    if (m_icon.isNull())
        m_icon = QIcon::fromTheme(iconName(), FileIconProvider::fileIcon(*this));
    return m_icon;
}

QString
Node::iconName()
{
    return m_iconName;
}

bool
Node::isPopulated() const
{
    QMutexLocker locker(&m_mutex);
    return m_isPopulated;
}

int Node::sortColumn() { return m_model->sortColumn(); }

Qt::SortOrder Node::sortOrder() { return m_model->sortOrder(); }

bool Node::showHidden() { return m_model->showHidden(); }

Worker::Gatherer *Node::gatherer() { return m_model->dataGatherer(); }

QString
Node::mimeType()
{
    return m_mimeType;
}

QString
Node::permissionsString()
{
    const QFile::Permissions p = permissions();
    QString perm;
    perm.append(p.testFlag(QFile::ReadUser)?"R, ":"-, ");
    perm.append(p.testFlag(QFile::WriteUser)?"W, ":"-, ");
    perm.append(p.testFlag(QFile::ExeUser)?"X":"-");
    return perm;
}

bool
Node::rename(const QString &newName)
{
    const QString &oldFilePath = absoluteFilePath();
    const QString &newFilePath = dir().absoluteFilePath(newName);
    if (QFile::rename(oldFilePath, newFilePath))
    {
        m_filePath = newFilePath;
        m_name = newName;
        setFile(m_filePath);
        QString newUrl = m_url.toString();
        newUrl.replace(oldFilePath, newFilePath); //TODO: better url renaming...
        setUrl(QUrl(newUrl));
        refresh();
        return true;
    }
    return false;
}

QString
Node::category()
{
    if (m_parent && m_parent == m_model->m_rootNode)
        return QObject::tr("scheme");
    if (isHidden())
        return QString(isDir()?"hidden directory":"hidden file");
    switch (sortColumn())
    {
    case 0: return isDir()?QObject::tr("directory"):name().at(0).toLower();
    case 1: return QObject::tr(isDir()?"directory":size()<1048576?"small":size()<1073741824?"medium":"large");
    case 2: return data(2).toString();
    case 3:
    {
        int y,m,d;
        lastModified().date().getDate(&y, &m, &d);
        return QString("%1 %2").arg(QString::number(y), QString::number(m));
    }
    default: return QString("-"); break;
    }
}

QVariant
Node::data(const int column)
{
    if (exists())
        switch (column)
        {
        case 0: return m_name; break;
        case 1: return isDir()?QString("--"):Ops::prettySize(size()); break;
        case 2:
        {
            if (isDir())
                return QString("directory");
            else if (isSymLink())
                return QString("symlink");
            else if (suffix().isEmpty() && isExecutable())
                return QString("exec");
            else if (suffix().isEmpty())
                return QString("file");
            else
                return suffix();
            break;
        }
        case 3: return lastModified(); break;
        case 4: return permissionsString(); break;
        default: return QString("--");
        }
    return !column?m_name:QString("--");
}

Node
*Node::localNode(const QString &path, bool checkOnly)
{
    QFileInfo fi(path);
    if (!fi.exists() || !fi.isAbsolute())
        return 0;
    QStringList paths;
    QDir dir;
    if (fi.isDir())
        dir = QDir(path);
    else
    {
        dir = fi.dir();
        paths << dir.absolutePath();
    }
    paths << path;
    while (dir.cdUp())
        paths.prepend(dir.absolutePath());

    Node *n = this;
    while (!paths.isEmpty() && n)
        n = n->nodeFromLocalPath(paths.takeFirst(), checkOnly);
    return n;
}

Node
*Node::nodeFromLocalPath(const QString &path, bool checkOnly)
{
    if (!isPopulated())
    {
        if (checkOnly)
            return 0;
        else
            rePopulate();
    }
    for (int i = 0; i < ChildrenTypeCount; ++i)
    {
        int c = childCount((Children)i);
        while (--c > -1)
        {
            Node *node = child(c, (Children)i);
            if (node && node->filePath() == path)
            {
                if (i)
                {
                    const int r = childCount();
                    m_model->beginInsertRows(m_model->createIndex(row(), 0, this), r, r);
                    m_mutex.lock();
                    m_children[Visible] << m_children[i].takeAt(c);
                    m_mutex.unlock();
                    m_model->endInsertRows();
                }
                return node;
            }
        }
    }
    return 0;
}

void
Node::removeDeleted()
{
    Nodes deleted;
    m_mutex.lock();
    for (int i = 0; i < ChildrenTypeCount; ++i)
    {
        int c = m_children[i].size();
        while (--c > -1)
        {
            Node *node = m_children[i].at(c);
            node->refresh();
            if (!node->exists())
                deleted << node;
        }
    }
    m_mutex.unlock();
    qDeleteAll(deleted);
}

void
Node::sort()
{
    int i = childCount();
    if (!i)
        return;

    if (i > 1)
    {
        m_mutex.lock();
        qStableSort(m_children[Visible].begin(), m_children[Visible].end(), lessThen);
        m_mutex.unlock();
    }

    while (--i > -1)
    {
        Node *node = child(i);
        if (node->childCount())
            node->sort();
    }
}

void
Node::setHiddenVisible(bool visible)
{
    if (visible)
    {
        m_mutex.lock();
        m_children[Visible]+=m_children[Hidden];
        qStableSort(m_children[Visible].begin(), m_children[Visible].end(), lessThen);
        m_children[Hidden].clear();
        m_mutex.unlock();
    }
    else
    {
        int i = childCount();
        while (--i > -1)
        {
            m_mutex.lock();
            if (m_children[Visible].at(i)->isHidden())
                m_children[Hidden] << m_children[Visible].takeAt(i);
            m_mutex.unlock();
        }
    }
    int i = childCount();
    while (--i > -1)
    {
        Node *node = child(i);
        if (node->isPopulated()||node->hasChildren())
            node->setHiddenVisible(visible);
    }
}

void
Node::clearVisible()
{
    qDeleteAll(m_children[Visible]);
}

void
Node::setFilter(const QString &filter)
{
    QMutexLocker locker(&m_mutex);
    if (filter == m_filter || model()->isWorking())
        return;

    m_filter = filter;
    for (int i = 0; i < Filtered; ++i)
    {
        int c = m_children[i].count();
        while (--c > -1)
            if (!m_children[i].at(c)->name().contains(filter, Qt::CaseInsensitive))
                m_children[Filtered] << m_children[i].takeAt(c);
    }

    int f = m_children[Filtered].count();
    while (--f > -1)
        if (m_children[Filtered].at(f)->name().contains(filter, Qt::CaseInsensitive))
        {
            if (m_children[Filtered].at(f)->isHidden() && !showHidden())
                m_children[Hidden] << m_children[Filtered].takeAt(f);
            else if (m_children[Filtered].at(f)->isHidden() && showHidden())
                m_children[Visible] << m_children[Filtered].takeAt(f);
            else
                m_children[Visible] << m_children[Filtered].takeAt(f);
        }
    if (m_children[Visible].count() > 1)
        qStableSort(m_children[Visible].begin(), m_children[Visible].end(), lessThen);
}

void
Node::rePopulate()
{
    if (gatherer()->isCancelled())
        return;

    if (m_url == m_model->m_url)
        m_model->getSort(m_url);

    if (isPopulated())
        removeDeleted();

    if (isAbsolute())
    {
        QDirIterator it(filePath(), allEntries);
        while (it.hasNext() && !gatherer()->isCancelled())
        {
            const QString &file = it.next();
            if (!hasChild(file))
                new Node(m_model, QUrl::fromLocalFile(file), this, file);
        }
    }
    else if (scheme() == "file")
    {
        foreach (const QFileInfo &f, QDir::drives())
            if (!hasChild(f.filePath()))
                new Node(m_model, QUrl::fromLocalFile(f.filePath()), this, f.filePath());
    }

    m_mutex.lock();
    m_isPopulated = true;
    m_mutex.unlock();

    if (m_url == m_model->m_url)
        emit m_model->urlLoaded(url());

    for (int i = 0; i < childCount(); ++i)
    {
        Node *c = child(i);
        if (c->isPopulated())
            c->rePopulate();
    }
}

//-----------------------------------------------------------------------------

AppNode::AppNode(Model *model, Node *parent, const QUrl &url, const QString &filePath)
    : Node(model, url, parent, filePath)
{
    QSettings info(filePath, QSettings::IniFormat);
    info.beginGroup("Desktop Entry");
    m_appName = info.value("Name").toString();
    qDebug() << "constructing appnode for" << m_appName;
    m_comment = info.value("Comment").toString();
    m_appCmd = info.value("Exec").toString();
    m_appIcon = info.value("Icon").toString();
    m_type = info.value("Type").toString();
    QStringList categories = info.value("Categories").toString().split(";", QString::SkipEmptyParts);
    if (!categories.isEmpty())
        m_category = categories.first();
}

QIcon
AppNode::icon()
{
    return QIcon::fromTheme(m_appIcon, FileIconProvider::fileIcon(*this));
}

QString
AppNode::category()
{
    return m_category;
}

QString
AppNode::name() const
{
    return m_appName;
}

QVariant
AppNode::data(const int column)
{
    if (exists())
        switch (column)
        {
        case 0: return m_appName; break;
        case 1: return QString("--"); break;
        case 2: return m_type; break;
        default: break;
        }
    return Node::data(column);
}

//-----------------------------------------------------------------------------

using namespace Worker;

Gatherer::Gatherer(QObject *parent)
    :Thread(parent)
    , m_node(0)
    , m_model(static_cast<FS::Model *>(parent))
    , m_isCancelled(false)
{
    connect(this, SIGNAL(started()), m_model, SIGNAL(startedWorking()));
    connect(this, SIGNAL(finished()), m_model, SIGNAL(finishedWorking()));
}

void
Gatherer::setCancelled(bool cancel)
{
    m_mutex.lock();
    m_isCancelled=cancel;
    m_mutex.unlock();
}

void
Gatherer::run()
{
    switch (m_task)
    {
    case Populate:
    {
        if (m_node)
            m_node->rePopulate();
        break;
    }
    case Generate:
    {
        if (QFileInfo(m_path).isDir())
            m_result = m_node->localNode(m_path, false);
        if (m_result)
            emit nodeGenerated(m_path, m_result);
        break;
    }
    case Search:
    {
        searchResultsForNode(m_searchName, m_searchPath, m_node);
        break;
    }
    case GetApps:
    {
        getApplications(m_appsPath, m_node);
        break;
    }
    default:
        break;
    }
    m_taskMutex.lock();
    m_node = 0;
    m_task = NoTask;
    m_taskMutex.unlock();
}

void
Gatherer::search(const QString &name, const QString &path, Node *node)
{
    setCancelled(true);
    wait();
    setCancelled(false);

    QMutexLocker locker(&m_taskMutex);
    m_task = Search;
//    QUrl url;
//    url.setScheme(node->scheme());
//    url.setPath(path);
//    url.setEncodedQuery(name.toLocal8Bit());
//    Node *searchNode = new Node(m_model, url, node, QUrl::fromLocalFile(path));
//    node->addChild(searchNode);
//    qDebug() << url;
//    emit m_model->urlChanged(url);
//    emit m_model->urlLoaded(url);
    m_node = node;
    m_searchName = name;
    m_searchPath = path;
    start();
}

void
Gatherer::populateApplications(const QString &appsPath, Node *node)
{
    setCancelled(true);
    wait();
    setCancelled(false);

    QMutexLocker locker(&m_taskMutex);
    m_task = GetApps;
    m_node = node;
    m_appsPath = appsPath;
    start();
}

void
Gatherer::populateNode(Node *node)
{
    QMutexLocker locker(&m_taskMutex);
    if (m_task == Populate && node == m_node) //already populating the node....
        return;

    setCancelled(true);
    wait();
    setCancelled(false);

    m_task = Populate;
    m_node = node;
    start();
}

void
Gatherer::generateNode(const QString &path, Node *parent)
{
    setCancelled(true);
    wait();
    setCancelled(false);

    QMutexLocker locker(&m_taskMutex);
    m_task = Generate;
    m_path = path;
    m_node = parent;
    m_result = 0;
    start();
}

void
Gatherer::getApplications(const QString &appsPath, Node *node)
{
    QDirIterator it(appsPath, allEntries, QDirIterator::Subdirectories);
    while (it.hasNext()&&!m_model->m_dataGatherer->isCancelled())
    {
        const QString &file(it.next());
        if (QFileInfo(file).isDir())
            continue;
        const QUrl &url = QUrl(QString("%1%2").arg(m_model->m_url.toString(), file));
        new AppNode(m_model, node, url, file);
    }
    emit m_model->urlLoaded(m_model->m_url);
}

void
Gatherer::searchResultsForNode(const QString &name, const QString &filePath, Node *node)
{
    QDirIterator it(filePath, allEntries, QDirIterator::Subdirectories);
    while (it.hasNext()&&!m_model->m_dataGatherer->isCancelled())
    {
        const QFileInfo &file(it.next());
        if (file.fileName().contains(name, Qt::CaseInsensitive))
        {
            const QUrl &url = QUrl(QString("%1%2").arg(m_model->m_url.toString(), file.filePath()));
            new Node(m_model, url, node, file.filePath());
        }
    }
    emit m_model->urlLoaded(m_model->m_url);
}
