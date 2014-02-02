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

#include "fsworkers.h"

using namespace DFM;

static bool lessThen(FileSystemNode *n1, FileSystemNode *n2)
{
    if ( n1 == n2 )
        return true;
    const bool desc = bool(n1->sortOrder() == Qt::DescendingOrder);
    const int column = n1->sortColumn();

    //dirs always first right?
    if ( n1->isDir() && !n2->isDir() )
        return true;
    if ( !n1->isDir() && n2->isDir() )
        return false;

    //hidden... last?
    if ( n1->isHidden() && !n2->isHidden() )
        return false;
    if ( !n1->isHidden() && n2->isHidden() )
        return true;

    bool lt = true;
    switch (column)
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
    return desc?!lt:lt;
}

static FileSystemNode::Nodes s_deletedNodes;

FileSystemNode::FileSystemNode(FileSystemModel *model, const QString &path, FileSystemNode *parent, const Type &t)
    : QFileInfo(path)
    , m_parent(parent)
    , m_isPopulated(false)
    , m_model(model)
    , m_filePath(path)
    , m_filter(QString(""))
    , m_type(t)
{
//    if ( t == FSNode )
//    {
        if ( !m_parent )
            m_name = "--";
        else if ( fileName().isEmpty() )
            m_name = filePath();
        else
            m_name = fileName();
//    }
//    else
//        m_name = filePath();
}

FileSystemNode::~FileSystemNode()
{
    for (int i = 0; i<HiddenFiles+1; ++i)
        if (!m_children[i].isEmpty())
            qDeleteAll(m_children[i]);
}

void
FileSystemNode::insertChild(FileSystemNode *node)
{
//    if (type() == SearchResult)
//    {
//        model()->beginInsertRows(QModelIndex(), m_children[Visible].count(), m_children[Visible].count());
//        m_mutex.lock();
//        m_children[Visible] << node;
//        m_mutex.unlock();
//        model()->endInsertRows();
//    }
//    else
//    {
        if ( node->isHidden() && !showHidden() )
            m_children[Hidden] << node;
        else
        {
            int z = m_children[Visible].size(), i = -1;
            while ( ++i < z )
                if ( !lessThen(m_children[Visible].at(i), node) )
                    break;

            Q_ASSERT(i<=m_children[Visible].size());
            model()->beginInsertRows(model()->index(filePath()), i, i);
            m_mutex.lock();
            m_children[Visible].insert(i, node);
            m_mutex.unlock();
            model()->endInsertRows();
        }
//    }
}

int
FileSystemNode::row()
{
    if (!parent())
        -1;

    return parent()->rowFor(this);
}

int
FileSystemNode::childCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_children[Visible].size();
}

FileSystemNode
*FileSystemNode::child(const int c)
{
    QMutexLocker locker(&m_mutex);
    if (c > -1 && c < m_children[Visible].size())
        return m_children[Visible].at(c);
    return 0;
}

FileSystemNode
*FileSystemNode::child(const QString &path)
{
    QMutexLocker locker(&m_mutex);
    int c = m_children[Visible].count();
    while ( --c > -1 )
    {
        FileSystemNode *node = m_children[Visible].at(c);
        if ( node && node->filePath() == path )
            return node;
    }
    return 0;
}

QString FileSystemNode::name() { return m_name; }

int FileSystemNode::sortColumn() { return model()->sortColumn(); }

Qt::SortOrder FileSystemNode::sortOrder() { return model()->sortOrder(); }

bool FileSystemNode::showHidden() { return model()->showHidden(); }

int
FileSystemNode::rowFor(FileSystemNode *child) const
{
    QMutexLocker locker(&m_mutex);
    return m_children[Visible].indexOf(child);
}

QString
FileSystemNode::category()
{
    if (isHidden())
        return QString(isDir()?"HIDDEN (DIRECTORY)":"HIDDEN (FILE)");
    switch (sortColumn())
    {
    case 0: return isDir()?QObject::tr("DIRECTORY"):name().at(0).toUpper(); break;
    case 1: return QObject::tr(isDir()?"DIRECTORY":size()<1048576?"SMALL":size()<1073741824?"MEDIUM":"LARGE"); break;
    case 2: return data(2).toString().toUpper(); break;
    case 3:
    {
        int y,m,d;
        lastModified().date().getDate(&y, &m, &d);
        return QString("%1 %2").arg(QString::number(y), QString::number(m));
        break;
    }
    default: return QString("-"); break;
    }
    return QString();
}

FileSystemModel *FileSystemNode::model() { return m_model; }
Worker::Gatherer *FileSystemNode::gatherer() { return model()->dataGatherer(); }

void
FileSystemNode::setFilter(const QString &filter)
{
    if (filter == m_filter || model()->isWorking())
        return;

    m_filter = filter;
    emit model()->layoutAboutToBeChanged();
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
    if ( m_children[Visible].count() > 1 )
        qStableSort(m_children[Visible].begin(), m_children[Visible].end(), lessThen);
    emit model()->layoutChanged();
}

bool
FileSystemNode::rename(const QString &newName)
{
    const QString &oldName = absoluteFilePath();
    if ( QFile::rename(oldName, dir().absoluteFilePath(newName)) )
    {
        m_filePath = dir().absoluteFilePath(newName);
        m_name = newName;
        setFile(m_filePath);
        refresh();
        return true;
    }
    return false;
}

QString
FileSystemNode::permissionsString()
{
    const QFile::Permissions p = permissions();
    QString perm;
    perm.append(p.testFlag(QFile::ReadUser)?"R, ":"-, ");
    perm.append(p.testFlag(QFile::WriteUser)?"W, ":"-, ");
    perm.append(p.testFlag(QFile::ExeUser)?"X":"-");
    return perm;
}

QVariant
FileSystemNode::data(const int column)
{
    switch (column)
    {
    case 0: return name(); break;
    case 1: return isDir()?QString("--"):Ops::prettySize(size()); break;
    case 2:
    {
        if ( isDir() )
            return QString("directory");
        else if ( isSymLink() )
            return QString("symlink");
        else if ( suffix().isEmpty() && isExecutable() )
            return QString("exec");
        else if ( suffix().isEmpty() )
            return QString("file");
        else
            return suffix();
        break;
    }
    case 3: return lastModified(); break;
    case 4: return permissionsString(); break;
    default: return QString("--");
    }
}

FileSystemNode
*FileSystemNode::nodeFromPath(const QString &path, bool checkOnly)
{
    if ( !isPopulated() )
    {
        if ( checkOnly )
            return 0;
        else
            rePopulate();
    }
    for ( int i = 0; i < Files; ++i )
    {
        if (!i)
            m_mutex.lock();
        int c = m_children[i].count();
        while ( --c > -1 )
        {
            FileSystemNode *node = m_children[i].at(c);
            if ( node && node->filePath() == path )
            {
                if ( i )
                    m_children[Visible] << m_children[i].takeAt(c);
                m_mutex.unlock();
                return node;
            }
        }
        if (!i)
            m_mutex.unlock();
    }
    return 0;
}

FileSystemNode
*FileSystemNode::node(const QString &path, bool checkOnly)
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

    FileSystemNode *n = model()->rootNode();

    while ( !paths.isEmpty() && n )
        n = n->nodeFromPath(paths.takeFirst(), checkOnly);

    if ( n && n->filePath() == path )
        return n;

    return 0;
}

bool
FileSystemNode::hasChild(const QString &name, const bool nameIsPath, const bool onlyVisible)
{
    int vis = onlyVisible ? 0 : Files;
    for ( int i = 0; i < vis; ++i )
    {
        Nodes::const_iterator start = m_children[i].constBegin(), end = m_children[i].constEnd();
        while ( start != end )
        {
            bool check = (*start)->name() == name;
            if (nameIsPath)
                check = (*start)->filePath() == name;
            if (check)
                return true;
            ++start;
        }
    }
    return false;
}

void
FileSystemNode::rePopulate()
{
    if (gatherer()->isCancelled())
        return;
    model()->startWorking();
    if (!m_children[Files].isEmpty())
    {
        m_mutex.lock();
        s_deletedNodes+=m_children[Visible];
        m_children[Visible].clear();
        m_children[Visible]=m_children[Files];
        m_children[Files].clear();
        m_children[Hidden]=m_children[HiddenFiles];
        m_children[HiddenFiles].clear();
        m_mutex.unlock();
    }
    if ( filePath() == model()->rootPath() )
    {
        int sc = model()->sortColumn();
        Qt::SortOrder so = model()->sortOrder();
        Ops::getSorting(filePath(), sc, so);
        if ( sc != model()->sortColumn() || so != model()->sortOrder() )
            model()->setSort(sc, so);
    }
    if ( isPopulated() )
        for ( int i = 0; i < Files; ++i )
        {
            if (gatherer()->isCancelled())
            {
                model()->endWorking();
                return;
            }
            int c = m_children[i].count();
            while ( --c > -1 )
            {
                if (gatherer()->isCancelled())
                {
                    model()->endWorking();
                    return;
                }
                FileSystemNode *node = m_children[i].at(c);
                node->refresh();
                if ( !node->exists() )
                {
                    if ( !i )
                    {
                        model()->beginRemoveRows(model()->index(filePath()), c, c);
                        m_mutex.lock();
                    }
                    s_deletedNodes << m_children[i].takeAt(c);
                    if ( !i )
                    {
                        m_mutex.unlock();
                        model()->endRemoveRows();
                    }
                }
            }
        }
    const QDir dir(filePath());
    if ( this == model()->rootNode() )
    {
        for ( int i = 0; i < QDir::drives().count(); ++i )
        {
            if ( hasChild(QDir::drives().at(i).filePath()) )
                continue;
            insertChild(new FileSystemNode(model(), QDir::drives().at(i).filePath(), this));
        }
    }
    else if ( dir.isAbsolute() )
    {
        QDirIterator it(filePath(), allEntries);
        while ( it.hasNext() )
        {
            if (gatherer()->isCancelled())
            {
                model()->endWorking();
                return;
            }
            const QFileInfo file(it.next());
            if ( hasChild(file.fileName()) )
                continue;

            insertChild(new FileSystemNode(model(), file.filePath(), this));
        }
    }

    m_isPopulated = true;
    if ( filePath() == model()->rootPath() )
        emit model()->directoryLoaded(filePath());

//    qDeleteAll(s_deletedNodes);
//    s_deletedNodes.clear();

    for ( int i = 0; i < m_children[Visible].count(); ++i )
        if ( m_children[Visible].at(i)->isPopulated() )
            m_children[Visible].at(i)->rePopulate();

    model()->endWorking();
}

void
FileSystemNode::sort()
{
    if ( model()->isWorking() )
        return;
    if ( m_children[Visible].isEmpty() )
        return;

    if ( m_children[Visible].count() > 1 )
        qStableSort(m_children[Visible].begin(), m_children[Visible].end(), lessThen);

    int i = m_children[Visible].count();
    while ( --i > -1 )
    {
        FileSystemNode *node = m_children[Visible].at(i);
        if ( node->isPopulated() && node->isDir() )
            node->sort();
    }
}

void
FileSystemNode::setHiddenVisible(bool visible)
{
    emit model()->layoutAboutToBeChanged();
    if ( visible )
    {
        m_children[Visible]+=m_children[Hidden];
        qStableSort(m_children[Visible].begin(), m_children[Visible].end(), lessThen);
        m_children[Hidden].clear();
    }
    else
    {
        int i = m_children[Visible].count();
        while ( --i > -1 )
            if ( m_children[Visible].at(i)->isHidden() )
                m_children[Hidden] << m_children[Visible].takeAt(i);
    }
    int i = m_children[Visible].count();
    while ( --i > -1 )
        if ( m_children[Visible].at(i)->isPopulated() )
            m_children[Visible].at(i)->setHiddenVisible(visible);
    emit model()->layoutChanged();
}

void
FileSystemNode::search(const QString &fileName)
{
    model()->startWorking();
    m_filter=fileName;
    emit model()->layoutAboutToBeChanged();
    if (m_children[Files].isEmpty())
    {
        m_mutex.lock();
        m_children[HiddenFiles]=m_children[Hidden];
        m_children[Hidden].clear();
        m_children[Files]=m_children[Visible];
        m_children[Visible].clear();
        m_mutex.unlock();
    }
    else
    {
        m_mutex.lock();
        s_deletedNodes+=m_children[Visible]; //only previous searchresults here...
        m_children[Visible].clear();
        m_mutex.unlock();
    }
    emit model()->layoutChanged();

    QDirIterator it(m_filePath, allEntries, QDirIterator::Subdirectories);
    while (it.hasNext()&&!model()->dataGatherer()->isCancelled())
    {
        const QFileInfo &file(it.next());
        if (file.fileName().contains(fileName, Qt::CaseInsensitive))
            insertChild(new FileSystemNode(m_model, file.filePath(), this, FileSystemNode::SearchResult));
    }
    model()->endWorking();
}

void
FileSystemNode::endSearch()
{
    if (!m_children[Files].isEmpty())
    {
        emit model()->layoutAboutToBeChanged();
        s_deletedNodes+=m_children[Visible];
        m_children[Visible].clear();
        m_children[Visible]=m_children[Files];
        m_children[Hidden]=m_children[HiddenFiles];
        m_children[Files].clear();
        m_children[HiddenFiles].clear();
        emit model()->layoutChanged();
    }
}

//-----------------------------------------------------------------------------

using namespace Worker;

Gatherer::Gatherer(QObject *parent)
    :Thread(parent)
    , m_node(0)
    , m_model(static_cast<FileSystemModel *>(parent))
    , m_isCancelled(false)
{
    connect(this, SIGNAL(started()), m_model, SIGNAL(startedWorking()));
    connect(this, SIGNAL(finished()), m_model, SIGNAL(finishedWorking()));
}

void
Gatherer::run()
{
    switch ( m_task )
    {
    case Populate:
    {
        if ( m_node )
            m_node->rePopulate();
        break;
    }
    case Generate:
    {
        if ( QFileInfo(m_path).isDir() )
            m_result = m_model->rootNode()->node(m_path, false);
        if ( m_result )
            emit nodeGenerated(m_path, m_result);
        break;
    }
    case Search:
    {
        m_node->search(m_searchName);
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
Gatherer::search(const QString &name, FileSystemNode *node)
{
    setCancelled(true);
    wait();
    setCancelled(false);

    QMutexLocker locker(&m_taskMutex);
    m_task = Search;
    m_node = node;
    m_searchName = name;
    start();
}

void
Gatherer::populateNode(FileSystemNode *node)
{
    if ( node == m_model->rootNode() )
        return;

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
Gatherer::generateNode(const QString &path)
{
    setCancelled(true);
    wait();
    setCancelled(false);

    QMutexLocker locker(&m_taskMutex);

    m_task = Generate;
    m_path = path;
    m_node = 0;
    m_result = 0;
    start();
}
