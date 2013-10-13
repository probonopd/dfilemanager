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


#include "viewcontainer.h"
#include "operations.h"
#include "iojob.h"
#include "deletedialog.h"
#include "mainwindow.h"
#include <QProcess>
#include <QMap>
#include <QInputDialog>
#include <QCompleter>
#include <QMenu>

using namespace DFM;

#define VIEWS(RUN)\
    m_iconView->RUN;\
    m_detailsView->RUN;\
    m_columnsWidget->RUN;\
    m_flowView->RUN

ViewContainer::ViewContainer(QWidget *parent, QString rootPath)
    : QFrame(parent)
    , m_model(new FileSystemModel(this))
    , m_viewStack(new QStackedWidget(this))
    , m_iconView(new IconView(this))
    , m_columnsWidget(new ColumnsWidget(this))
    , m_detailsView(new DetailsView(this))
    , m_flowView(new FlowView(this))
    , m_breadCrumbs(new BreadCrumbs(this, m_model))

{
    m_breadCrumbs->setVisible(Store::settings()->value("pathVisible", true).toBool());
    m_myView = Icon;
    m_back = false;

    QString parentPath = rootPath.mid(0, rootPath.lastIndexOf(QDir::separator()));
    static_cast<FileIconProvider *>(m_model->iconProvider())->loadThemedFolders(parentPath);

    m_viewStack->layout()->setSpacing(0);
    setFrameStyle(0/*QFrame::StyledPanel | QFrame::Sunken*/);
    setAutoFillBackground(false);

    setModel(m_model);
    setSelectionModel(new QItemSelectionModel(m_model));

    connect( m_model, SIGNAL(rootPathChanged(QString)), this, SLOT(rootPathChanged(QString)));
    connect( m_model, SIGNAL(directoryLoaded(QString)), this, SLOT(directoryLoaded(QString)));
    connect( m_iconView, SIGNAL(iconSizeChanged(int)), this, SIGNAL(iconSizeChanged(int)));

    connect( m_iconView, SIGNAL(newTabRequest(QModelIndex)), this, SLOT(genNewTabRequest(QModelIndex)));
    connect( m_detailsView, SIGNAL(newTabRequest(QModelIndex)), this, SLOT(genNewTabRequest(QModelIndex)));
    connect( m_flowView->detailsView(), SIGNAL(newTabRequest(QModelIndex)), this, SLOT(genNewTabRequest(QModelIndex)));

    foreach (QAbstractItemView *v, QList<QAbstractItemView *>() << m_iconView << m_detailsView /*<< m_columnsView*/ << m_flowView->detailsView())
    {
        v->setMouseTracking( true );
        connect( v, SIGNAL(entered(QModelIndex)), this, SIGNAL(entered(QModelIndex)));
        connect( v, SIGNAL(viewportEntered()), this, SIGNAL(viewportEntered()));
        connect( v, SIGNAL(activated(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)));
    }

    m_viewStack->addWidget(m_iconView);
    m_viewStack->addWidget(m_detailsView);
    m_viewStack->addWidget(m_columnsWidget);
    m_viewStack->addWidget(m_flowView);

    m_detailsView->setColumnHidden(4, true);
    m_flowView->detailsView()->setColumnHidden(4, true);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(m_viewStack);
    layout->addWidget(m_breadCrumbs);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    setLayout(layout);

    setView((View)Store::config.behaviour.view, false);
    m_model->setRootPath(rootPath);

    emit iconSizeChanged(Store::config.views.iconView.iconSize*16);

    sort(Store::config.behaviour.sortingCol, Store::config.behaviour.sortingOrd);
}

PathNavigator
*ViewContainer::pathNav()
{ return m_breadCrumbs->pathNav(); }

void
ViewContainer::setModel(QAbstractItemModel *model)
{
    VIEWS(setModel(model));
}

void
ViewContainer::setSelectionModel(QItemSelectionModel *selectionModel)
{
    m_selectModel = selectionModel;
    VIEWS(setSelectionModel(selectionModel));
}

FileSystemModel
*ViewContainer::model()
{
    return m_model;
}

QItemSelectionModel
*ViewContainer::selectionModel()
{
    return m_selectModel;
}

void
ViewContainer::setView(View view, bool store)
{
    m_myView = view;
    if (view == Icon)
        m_viewStack->setCurrentWidget(m_iconView);
    else if (view == Details)
        m_viewStack->setCurrentWidget(m_detailsView);
    else if (view == Columns)
        m_viewStack->setCurrentWidget(m_columnsWidget);
    else if (view == Flow)
        m_viewStack->setCurrentWidget(m_flowView);
    emit viewChanged();

#ifdef Q_WS_X11
    if ( Store::config.views.dirSettings && store )
    {
        QString dirPath = m_model->rootPath();
        if ( !dirPath.endsWith(QDir::separator()) )
            dirPath.append(QDir::separator());
        dirPath.append(".directory");
        QSettings settings(dirPath, QSettings::IniFormat);
        settings.beginGroup("DFM");
        settings.setValue("view", (int)view);
        settings.endGroup();
    }
#endif
}

static QList<QAction *> s_actions;

QList<QAction *> ViewContainer::rightClickActions() { return s_actions; }

void
ViewContainer::addActions(QList<QAction *> actions)
{
    if ( s_actions.isEmpty() )
        s_actions = actions;
    VIEWS(addActions(actions));
    Store::settings()->beginGroup("Scripts");
    foreach ( const QString &string, Store::settings()->childKeys())
    {
        QStringList actions = Store::settings()->value(string).toStringList(); //0 == name, 1 == script, 2 == keysequence
        QAction *action = new QAction(actions[0], this);
        action->setData(actions[1]);
        if ( actions.count() > 2 )
            action->setShortcut(QKeySequence(actions[2]));
        connect (action, SIGNAL(triggered()), this, SLOT(scriptTriggered()));
        VIEWS(addAction(action));
    }
    Store::settings()->endGroup();
}

void
ViewContainer::customActionTriggered()
{
    QStringList action(static_cast<QAction *>(sender())->data().toString().split(" "));
    const QString &app = action.takeFirst();

    if ( selectionModel()->hasSelection() )
    {
        if ( selectionModel()->selectedRows().count() )
            foreach ( const QModelIndex &index, selectionModel()->selectedRows() )
                QProcess::startDetached(app, QStringList() << action << m_model->filePath( index ));
        else if ( selectionModel()->selectedIndexes().count() )
            foreach ( const QModelIndex &index, selectionModel()->selectedIndexes() )
                QProcess::startDetached(app, QStringList() << action << m_model->filePath( index ));
    }
    else
    {
        QProcess::startDetached(app, QStringList() << action << m_model->rootPath());
    }
}

void
ViewContainer::scriptTriggered()
{
    QStringList action(static_cast<QAction *>(sender())->data().toString().split(" "));
    const QString &app = action.takeFirst();
    qDebug() << "trying to launch" << app << "in" << m_model->rootPath();
    QProcess::startDetached(app, QStringList() << m_model->rootPath());
}

void
ViewContainer::activate(const QModelIndex &index)
{
    if ( !index.isValid() )
        return;

    if ( !m_model->fileInfo(index).exists() )
        return;

    QProcess process;
    if (m_model->isDir(index))
    {
        m_model->setRootPath(m_model->filePath(index));
        m_forwardList.clear();
    }
    else if(m_model->fileInfo(index).isExecutable() && (m_model->fileInfo(index).suffix().isEmpty() ||
                                                         m_model->fileInfo(index).suffix() == "sh" ||
                                                         m_model->fileInfo(index).suffix() == "exe")) // windows executable
    {
        QString file;
#ifdef Q_WS_X11 //linux
        file = m_model->filePath(index);
#else
        file = m_fsModel->fileName(index);
#endif
        process.startDetached(file);
    }
    else
    {
        QStringList list;
        list << m_model->filePath(index);

#ifdef Q_WS_X11 //linux
        process.startDetached("xdg-open", list);
#else //windows
        process.startDetached("cmd /c start " + list.at(0)); //todo: fix, this works but shows a cmd window real quick
#endif
    }
}

void
ViewContainer::rootPathChanged(const QString &path)
{
    if ( path.isEmpty() )
        return;

    const QModelIndex &rootIndex = m_model->index(path);
    setRootIndex(rootIndex);
    emit currentPathChanged(path);
    m_selectModel->clearSelection();

    if (!m_back && rootIndex.isValid() && (m_backList.isEmpty() || m_backList.count() &&  path != m_backList.last()))
        m_backList.append(path);

    m_back = false;
    m_iconView->updateLayout();
    sort(m_model->sortingColumn(), m_model->sortingOrder());

#ifdef Q_WS_X11
    if ( Store::config.views.dirSettings )
    {
        QString dirPath = path;
        if ( !dirPath.endsWith(QDir::separator()) )
            dirPath.append(QDir::separator());
        dirPath.append(".directory");
        QSettings settings(dirPath, QSettings::IniFormat);
        settings.beginGroup("DFM");
        QVariant var = settings.value("view");
        if ( var.isValid() )
        {
            View view = (View)var.value<int>();
            setView(view, false);
        }
        settings.endGroup();
    }
#endif
}

void
ViewContainer::directoryLoaded(QString index)
{
//    m_fsModel->loadThumbails(index);
}

void
ViewContainer::goBack()
{
    if(m_backList.count() == 1)
        return;

    m_back = true;
    m_forwardList << m_backList.takeLast();
    m_model->setRootPath(m_backList.last());
}

void
ViewContainer::goForward()
{
    if (m_forwardList.isEmpty())
        return;

    m_model->setRootPath(m_forwardList.takeLast());
}

bool
ViewContainer::goUp()
{
    if (m_breadCrumbs->currentPath() == "/")
        return false;
    m_model->setRootPath(QFileInfo(m_model->rootPath()).dir().path());
    return true;
}

void
ViewContainer::goHome()
{
    m_model->setRootPath(QDir::homePath());
}

void
ViewContainer::refresh()
{
    m_model->blockSignals(true);
    m_model->setRootPath("");
    m_model->setRootPath(m_breadCrumbs->currentPath());
    m_model->blockSignals(false);
    setRootIndex(m_model->index(m_breadCrumbs->currentPath()));
}

void
ViewContainer::rename()
{
    switch (m_myView)
    {
    case Icon : m_iconView->edit(m_iconView->currentIndex()); break;
    case Details : m_detailsView->edit(m_detailsView->currentIndex()); break;
    case Columns : m_columnsWidget->edit(m_columnsWidget->currentIndex()); break;
    case Flow : m_flowView->detailsView()->edit(m_flowView->detailsView()->currentIndex()); break;
    default: break;
    }
}

void
ViewContainer::setFilter(QString filter)
{
    VIEWS(setFilter(filter));
    m_dirFilter = filter;
    emit filterChanged();
}

void
ViewContainer::deleteCurrentSelection()
{
    QModelIndexList delUs;
    if(m_selectModel->selectedRows(0).count())
        delUs = m_selectModel->selectedRows(0);
    else
        delUs = m_selectModel->selectedIndexes();

    if ( DeleteDialog(delUs).result() == 0 )
        return;

    QStringList delList;
    for (int i = 0; i < delUs.count(); ++i)
    {
        QFileInfo file(m_model->filePath(delUs.at(i)));
        if (file.isWritable())
            delList << file.filePath();
    }
    if ( !delList.isEmpty() )
        IO::Job::remove(delList);
}

void
ViewContainer::scrollToSelection()
{
    if(m_selectModel->selectedIndexes().isEmpty())
    {
        m_iconView->scrollToTop();
        m_detailsView->scrollToTop();
//        m_columnsWidget->scrollToTop();
        m_flowView->detailsView()->scrollToTop();
    }
    else
    {
        m_iconView->scrollTo(m_selectModel->selectedIndexes().first());
        m_detailsView->scrollTo(m_selectModel->selectedIndexes().first());
//        m_columnsWidget->scrollTo(m_selectModel->selectedIndexes().first());
        m_flowView->detailsView()->scrollTo(m_selectModel->selectedIndexes().first());
    }
}

void
ViewContainer::customCommand()
{
    QModelIndexList selectedItems;
    if(m_selectModel->selectedRows(0).count())
        selectedItems = m_selectModel->selectedRows(0);
    else
        selectedItems = m_selectModel->selectedIndexes();

    QModelIndex idx;
    if(selectedItems.count() == 1)
        idx = selectedItems.first();
    bool ok;
    QString text = QInputDialog::getText(this, tr("Open With"), tr("Custom Command:"), QLineEdit::Normal, QString(), &ok);
    if(!ok)
        return;

    QStringList args(text.split(" "));
    args << (m_model->filePath(idx).isNull() ? m_model->rootPath() : m_model->filePath(idx));
    QString program = args.takeFirst();
    QProcess process;
    process.startDetached(program,args);
}

void
ViewContainer::sort(const int column, const Qt::SortOrder order, const QString &path)
{
    const QString &rootPath = m_model->rootPath();
    if ( !path.isEmpty() || m_myView == Columns )
    {
        m_model->blockSignals(true);
        m_model->setRootPath(path);
    }
    m_model->sort(column, order);
    if ( !path.isEmpty() || m_myView == Columns )
    {
        m_model->setRootPath(rootPath);
        m_model->blockSignals(false);
    }
    emit sortingChanged(column, order);
}

void
ViewContainer::setRootIndex(const QModelIndex &index)
{
    VIEWS(setRootIndex(index));
}

QSize
ViewContainer::iconSize()
{
    return m_iconView->iconSize();
}

void
ViewContainer::createDirectory()
{
    m_model->mkdir(m_model->index(m_model->rootPath()),"new_directory");
    QModelIndex newFolder = m_model->index(m_model->rootPath() + QDir::separator() + "new_directory");

    switch (m_myView)
    {
    case Icon : m_iconView->edit(newFolder); m_iconView->scrollTo(newFolder); break;
    case Details : m_detailsView->edit(newFolder); m_detailsView->scrollTo(newFolder); break;
    case Columns : m_columnsWidget->edit(newFolder); m_columnsWidget->scrollTo(newFolder); break;
    case Flow : m_flowView->detailsView()->edit(newFolder); m_flowView->detailsView()->scrollTo(newFolder); break;
    default: break;
    }
}

QAbstractItemView
*ViewContainer::currentView() const
{
    if ( m_myView == Columns )
        return m_columnsWidget->currentView();
    if ( m_myView != Flow )
        return static_cast<QAbstractItemView*>(m_viewStack->currentWidget());
    return static_cast<QAbstractItemView*>(m_flowView->detailsView());
}

QModelIndex
ViewContainer::indexAt(const QPoint &p) const
{
    return currentView()->indexAt(mapFromParent(p));
}

bool
ViewContainer::setPathVisible(bool visible)
{
    m_breadCrumbs->setVisible(visible);
}

bool
ViewContainer::pathVisible()
{
    return m_breadCrumbs->isVisible();
}

bool
ViewContainer::canGoBack()
{
    return m_backList.count() > 1;
}

bool
ViewContainer::canGoForward()
{
    return m_forwardList.count() >= 1;
}

void
ViewContainer::genNewTabRequest(QModelIndex index)
{
    emit newTabRequest(m_model->filePath(index));
}

void
ViewContainer::setRootPath(const QString &rootPath)
{
//    m_proxyModel->setRootPath(rootPath);
}

//void
//ViewContainer::resizeEvent(QResizeEvent *event)
//{
//    pathNav->setFixedWidth(rect().width() - style()->pixelMetric(QStyle::PM_ScrollBarExtent)*2);
//    pathNav->move(style()->pixelMetric(QStyle::PM_ScrollBarExtent),rect().height() - (pathNav->rect().height() + style()->pixelMetric(QStyle::PM_ScrollBarExtent)));
//}

BreadCrumbs
*ViewContainer::breadCrumbs()
{
    return m_breadCrumbs;
}

void
ViewContainer::setPathEditable(bool editable)
{
    m_breadCrumbs->setEditable(editable);
}

void
ViewContainer::animateIconSize(int start, int stop)
{
    m_iconView->setNewSize(stop);
}

QString
ViewContainer::currentFilter()
{
    return m_dirFilter;
}
