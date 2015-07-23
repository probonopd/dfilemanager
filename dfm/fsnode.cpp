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

#include "fsnode.h"
#include "fsmodel.h"
#include "fsworkers.h"
#include "dataloader.h"
#include "devices.h"

#include <QSettings>
#include <QDesktopServices>
#include <QProcess>
#include <QDirIterator>
#include <QDateTime>
#include <QDebug>

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

Node::Node(Model *model, const QUrl &url, Node *parent, const QString &filePath, const Type t)
    : QFileInfo(filePath)
    , m_isPopulated(false)
    , m_filePath(filePath)
    , m_filter(QString(""))
    , m_model(model)
    , m_parent(parent)
    , m_url(url)
    , m_isExe(-1)
    , m_isDeleted(false)
    , m_type(t)
    , m_invertFilter(false)
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

    if (parent)
        parent->addChild(this);
}

Node::~Node()
{
    if (m_parent)
        m_parent->removeChild(this);
    m_parent = 0;
    if (m_model->m_nodes.contains(m_url))
        m_model->m_nodes.remove(m_url);
    for (int i = 0; i < ChildrenTypeCount; ++i)
    {
        qDeleteAll(m_children[i]);
        m_children[i].clear();
    }
}

bool
Node::isFiltered(const QString &name)
{
    if (m_filter.isEmpty())
        return false;
    if (m_invertFilter)
        return name.toLower().contains(m_filter);
    return !name.toLower().contains(m_filter);
}

void
Node::deleteLater()
{
    m_isDeleted = true;
    m_parent->removeChild(this);
    emit m_model->deleteNodeLater(this);
}

void
Node::removeChild(Node *node)
{
    for (int i = 0; i < ChildrenTypeCount; ++i)
    {
        m_mutex.lock();
        const int idx(m_children[i].indexOf(node));
        m_mutex.unlock();
        if (idx != -1)
        {
            if (!i)
                m_model->beginRemoveRows(m_model->createIndex(row(), 0, this), idx, idx);
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

    if ((node->isHidden() && !m_model->showHidden()))
        m_children[Hidden] << node;
    else if (isFiltered(node->name()))
        m_children[Filtered] << node;
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
*Node::child(const int c, Children fromChildren) const
{
    QMutexLocker locker(&m_mutex);
    if (c > -1 && c < m_children[fromChildren].size())
        return m_children[fromChildren].at(c);
    return 0;
}

Node
*Node::child(const QString &name, const bool nameIsPath) const
{
    QMutexLocker locker(&m_mutex);
    for (int i = 0; i < ChildrenTypeCount; ++i)
        for (Nodes::const_iterator b = m_children[i].constBegin(), e = m_children[i].constEnd(); b!=e; ++b)
            if ((nameIsPath?(*b)->filePath():(*b)->name()) == name)
                return *b;
    return 0;
}

Node
*Node::childFromUrl(const QUrl &url) const
{
    QMutexLocker locker(&m_mutex);
    for (int i = 0; i < ChildrenTypeCount; ++i)
        for (Nodes::const_iterator b = m_children[i].constBegin(); b!=m_children[i].constEnd(); ++b)
            if ((*b)->url() == url)
                return *b;
    return 0;
}


int
Node::row() const
{
    if (!m_parent)
        return -1;

    return m_parent->rowOf(this);
}

int
Node::rowOf(const Node *node) const
{
    QMutexLocker locker(&m_mutex);
    return m_children[Visible].indexOf(const_cast<Node *>(node));
}

Node
*Node::parent() const
{
//    QMutexLocker locker(&m_mutex);
    return m_parent;
}

bool
Node::hasChildren() const
{
    QMutexLocker locker(&m_mutex);
    return !m_children[Visible].isEmpty();
}

QIcon
Node::icon() const
{
    if (Devices::instance()->mounts().contains(m_filePath))
        return FileIconProvider::typeIcon(FileIconProvider::Drive);
    QIcon icon = FileIconProvider::fileIcon(*this);
    if (icon.pixmap(16).isNull())
        icon = FileIconProvider::typeIcon(isDir()?FileIconProvider::Folder:FileIconProvider::File);
    if (Data *d = moreData())
    {
        if (!d->thumb.isNull())
            return QIcon(QPixmap::fromImage(d->thumb));
        return QIcon::fromTheme(d->iconName, icon);
    }
    return icon;
}

bool
Node::isPopulated() const
{
    QMutexLocker locker(&m_mutex);
    return m_isPopulated;
}

int Node::sortColumn() const { return m_model->sortColumn(); }

Qt::SortOrder Node::sortOrder() const { return m_model->sortOrder(); }

bool Node::showHidden() const { return m_model->showHidden(); }

Worker::Gatherer *Node::gatherer() const { return m_model->dataGatherer(); }

QString
Node::mimeType() const
{
    if (Data *d = moreData())
        return d->mimeType;
    return QString();
}

QString
Node::fileType() const
{
    if (isDir())
        return QString("directory");

    if (Data *d = moreData())
        return d->fileType;
    return suffix();
}

Data
*Node::moreData() const
{
    return DDataLoader::data(m_filePath, m_model->isWorking());
}

QString
Node::permissionsString() const
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
Node::category() const
{
//    if (m_parent && m_parent == static_cast<Node *>(&m_model->m_rootNode))
//        return QObject::tr("scheme");
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
Node::data(const int column) const
{
    if (exists())
        switch (column)
        {
        case 0: return name(); break;
        case 1: return isDir()?(moreData()?moreData()->count:QString("--")):Ops::prettySize(size()); break;
        case 2:
        {
            if (isSymLink())
                return QString("symlink");
            else if (isDir())
                return QString("directory");
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
    if ((!fi.exists() || !fi.isAbsolute()) && !checkOnly)
        return 0;

    QStringList paths;
    QDir dir;

    if (fi.isDir())
        dir = QDir(path);
    else
    {
        dir = fi.dir();
        paths << fi.absoluteFilePath();
    }

    Node *n = this;
    do
    {
        const QString &p = dir.absolutePath();
        paths.prepend(p);
    }
    while (dir.cdUp());

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
        int c = childCount(i);
        while (--c > -1)
        {
            Node *node = child(c, i);
            if (node && node->filePath() == path && !isFiltered(node->name()))
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
#include <unistd.h>
void
Node::setFilter(const QString &filter)
{
    const QString low(filter.toLower());
    if (low == m_filter || model()->isWorking())
        return;

    m_filter = low;
    m_invertFilter = m_filter.startsWith("!");
    if (m_invertFilter)
        m_filter.remove(0, 1);
    emit m_model->layoutAboutToBeChanged();
    const QModelIndexList oldList(m_model->persistentIndexList());
    QList<QPair<int, Node *> > old;
    for (int i = 0; i < oldList.count(); ++i)
        old << QPair<int, Node *>(oldList.at(i).column(), m_model->node(oldList.at(i)));

    m_mutex.lock();
    //add unfiltered to filter...
    if (!m_filter.isEmpty())
    for (int i = 0; i < Filtered; ++i)
    {
        int c = m_children[i].count();
        while (--c > -1)
            if (isFiltered(m_children[i].at(c)->name()))
                m_children[Filtered] << m_children[i].takeAt(c);
    }
    //show previously filtered...
    int f = m_children[Filtered].count();
    while (--f > -1)
    {
        Node *n(m_children[Filtered].at(f));
        if (!isFiltered(n->name()))
            m_children[n->isHidden() && !showHidden() ? Hidden : Visible] << m_children[Filtered].takeAt(f);
    }
    m_mutex.unlock();

    QModelIndexList newList;
    for (int i = 0; i < old.count(); ++i)
    {
        QPair<int, Node *> n = old.at(i);
        Node *node = n.second;
        QModelIndex idx = m_model->index(QUrl::fromLocalFile(node->filePath()));
        if (n.first > 0)
            idx = idx.sibling(idx.row(), n.first);
        newList << idx;
    }
    m_model->changePersistentIndexList(oldList, newList);
    emit m_model->layoutChanged();

    m_model->sortNode(this);
//    if (m_children[Visible].count() > 1)
//        qStableSort(m_children[Visible].begin(), m_children[Visible].end(), lessThen);
//        m_model->sort(sortColumn(), sortOrder());
}

void
Node::removeDeleted()
{
    for (int i = 0; i < ChildrenTypeCount; ++i)
    {
        int c = childCount(i);
        while (--c > -1)
        {
            Node *node = child(c, i);
            node->refresh();
            if (!node->exists())
                node->deleteLater();
        }
    }
}

void
Node::rePopulate()
{
    if (gatherer()->isCancelled())
        return;

    if (m_url == m_model->m_url)
        m_model->getSort(m_url);

    if (isPopulated())
    {
        removeDeleted();
        setHiddenVisible(showHidden());
    }

    if (isAbsolute())
    {
        QDirIterator it(filePath(), allEntries);
        while (it.hasNext() && !gatherer()->isCancelled())
        {
            const QString &file = it.next();
            QString url = m_url.toString();
            url.append(file.mid(file.lastIndexOf("/")+(url.endsWith("/"))));
            if (!child(file))
                new Node(m_model, QUrl(url), this, file);
        }
    }
    else if (parent() == m_model->m_rootNode)
    {
        QList<Device *> devices = Devices::instance()->devices();
        for (int i = 0; i < devices.count(); ++i)
        {
            Device *device = devices.at(i);
            if (device->isMounted() && !child(device->mountPath()))
                new Node(m_model, QUrl::fromLocalFile(device->mountPath()), this, device->mountPath());
        }
    }
    else
        return;

    m_mutex.lock();
    m_isPopulated = true;
    m_mutex.unlock();

//    qDebug() << m_url << m_model->m_url;
    if (m_url.path() == m_model->m_url.path())
        emit m_model->urlLoaded(url());

    for (int i = 0; i < childCount(); ++i)
    {
        Node *c = child(i);
        if (c->isPopulated())
            c->rePopulate();
    }
}

bool
Node::isExec() const
{
    if (m_isExe == -1)
    {
        if (Data *d = moreData())
        {
            const bool exeSuffix = bool(suffix() == "exe");
            const bool exeFileInfo = mimeType().contains("executable", Qt::CaseInsensitive)||d->fileType.contains("executable", Qt::CaseInsensitive);
            if (isExecutable() && (exeSuffix || exeFileInfo && !isDir()))
                m_isExe = 1;
            else
                m_isExe = 0;
        }
        return false;
    }
    return m_isExe;
}

void
Node::exec()
{
    if (!isAbsolute())
        return;

    if (isDir())
        m_model->setUrl(m_url);
    else if (isExec())
        QProcess::startDetached(m_filePath);
    else
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_filePath));
}

//-----------------------------------------------------------------------------

AppNode::AppNode(Model *model, Node *parent, const QUrl &url, const QString &filePath)
    : Node(model, url, parent, filePath, App)
{
    QSettings info(filePath, QSettings::IniFormat);
    info.beginGroup("Desktop Entry");
    m_appName = info.value("Name").toString();
    m_comment = info.value("Comment").toString();
    m_appCmd = info.value("Exec").toString();
    m_appIcon = info.value("Icon").toString();
    m_type = info.value("Type").toString();
    QStringList categories = info.value("Categories").toString().split(";", QString::SkipEmptyParts);
    if (!categories.isEmpty())
        m_category = categories.first();
}

QIcon
AppNode::icon() const
{
    return QIcon::fromTheme(m_appIcon, FileIconProvider::fileIcon(*this));
}

QString
AppNode::category() const
{
    return m_category;
}

QString
AppNode::name() const
{
    return m_appName;
}

QVariant
AppNode::data(const int column) const
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

void
AppNode::exec()
{
    if (m_appCmd.isEmpty())
        return;
#if defined(ISUNIX)
    QString app = m_appCmd.split(" ", QString::SkipEmptyParts).first();
    const QStringList p = QString(getenv("PATH")).split(":", QString::SkipEmptyParts);
    for (QStringList::const_iterator b = p.constBegin(), e = p.constEnd(); b!=e; ++b)
    {
        const QString &file = QString("%1/%2").arg(*b, app);
        if (QFileInfo(file).exists())
        {
            QProcess::startDetached(file);
            return;
        }
    }
#endif
}

//-----------------------------------------------------------------------------

QString
TrashNode::trashPath()
{
    return DTrash::trashPath();
}

TrashNode::TrashNode(Model *model, Node *parent, const QUrl &url, const QString &filePath)
    : Node(model, url, parent, filePath, Trash)
{
}

//-----------------------------------------------------------------------------
