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

ViewContainer::ViewContainer(QWidget *parent, QString rootPath) : QFrame(parent)
{
    m_viewStack = new QStackedWidget(this);
    m_fsModel = new FileSystemModel(this);
    m_iconView = new IconView(this);
    m_detailsView = new DetailsView(this);
    m_columnsView = new ColumnsView(this);
    m_flowView = new FlowView(this);
    m_breadCrumbs = new BreadCrumbs( this, m_fsModel );
    m_breadCrumbs->setVisible(Configuration::settings()->value("pathVisible", true).toBool());

    m_myView = Icon;

    m_back = false;

    m_viewStack->layout()->setSpacing(0);
//    setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);

    setModel(m_fsModel);
    setSelectionModel(new QItemSelectionModel(m_fsModel));

    if ( Configuration::config.views.singleClick )
    {
        connect( m_iconView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)) );
        connect( m_detailsView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)));
        connect( m_columnsView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)));
        connect( m_flowView->detailsView(), SIGNAL(clicked(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)));
    }
    else
    {
        connect( m_iconView, SIGNAL(activated(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)));
        connect( m_detailsView, SIGNAL(activated(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)));
        connect( m_columnsView, SIGNAL(activated(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)));
        connect( m_flowView->detailsView(), SIGNAL(activated(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)));
    }
    connect( m_fsModel, SIGNAL(rootPathChanged(QString)),this, SLOT(rootPathChanged(QString)));
    connect( m_fsModel, SIGNAL(directoryLoaded(QString)),this, SLOT(directoryLoaded(QString)));
    connect( m_iconView, SIGNAL(iconSizeChanged(int)),this, SIGNAL(iconSizeChanged(int)));

    connect( m_iconView, SIGNAL(newTabRequest(QModelIndex)),this, SLOT(genNewTabRequest(QModelIndex)));
    connect( m_detailsView, SIGNAL(newTabRequest(QModelIndex)),this, SLOT(genNewTabRequest(QModelIndex)));
    connect( m_flowView->detailsView(), SIGNAL(newTabRequest(QModelIndex)),this, SLOT(genNewTabRequest(QModelIndex)));
    connect( m_columnsView, SIGNAL(newTabRequest(QModelIndex)),this, SLOT(genNewTabRequest(QModelIndex)));

    connect( MainWindow::currentWindow(), SIGNAL(settingsChanged()), this, SLOT(settingsChanged()) );

//    connect( m_breadCrumbs, SIGNAL(newPath(QString)), this, SLOT(rootPathChanged(QString)) );

    foreach (QAbstractItemView *v, QList<QAbstractItemView *>() << m_iconView << m_detailsView << m_columnsView << m_flowView->detailsView())
    {
        v->setMouseTracking( true );
        connect( v, SIGNAL(entered(QModelIndex)), this, SIGNAL(entered(QModelIndex)));
        connect( v, SIGNAL(viewportEntered()), this, SIGNAL(viewportEntered()));
        connect ( m_fsModel, SIGNAL(rootPathAsIndex(QModelIndex)), v, SLOT(setRootIndex(QModelIndex)) );
    }

    connect( m_fsModel, SIGNAL(rootPathAsIndex(QModelIndex)), m_flowView, SLOT(setRootIndex(QModelIndex)) );

    m_viewStack->addWidget(m_iconView);
    m_viewStack->addWidget(m_detailsView);
    m_viewStack->addWidget(m_columnsView);
    m_viewStack->addWidget(m_flowView);

    m_detailsView->setColumnHidden(2, true);
    m_flowView->detailsView()->setColumnHidden(2, true);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(m_viewStack);
    layout->addWidget(m_breadCrumbs);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    setLayout(layout);

    m_fsModel->setRootPath(rootPath);
    setView((View)Configuration::config.behaviour.view);
    emit iconSizeChanged(Configuration::config.views.iconView.iconSize*16);
}

void
ViewContainer::setModel(FileSystemModel *model)
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
*ViewContainer::model() const
{
    return m_fsModel;
}

QItemSelectionModel
*ViewContainer::selectionModel() const
{
    return m_selectModel;
}

void
ViewContainer::setView(View view)
{
    m_myView = view;
    if (view == Icon)
        m_viewStack->setCurrentWidget(m_iconView);
    else if (view == Details)
        m_viewStack->setCurrentWidget(m_detailsView);
    else if (view == Columns)
        m_viewStack->setCurrentWidget(m_columnsView);
    else if (view == Flow)
        m_viewStack->setCurrentWidget(m_flowView);
    emit viewChanged();
}

void
ViewContainer::addActions(QList<QAction *> actions)
{
    VIEWS(addActions(actions));
    Configuration::settings()->beginGroup("Scripts");
    foreach ( const QString &string, Configuration::settings()->childKeys())
    {
        QStringList actions = Configuration::settings()->value(string).toStringList(); //0 == name, 1 == script, 2 == keysequence
        QAction *action = new QAction(actions[0], this);
        action->setData(actions[1]);
        if ( actions.count() > 2 )
            action->setShortcut(QKeySequence(actions[2]));
        connect (action, SIGNAL(triggered()), this, SLOT(scriptTriggered()));
        VIEWS(addAction(action));
    }
    Configuration::settings()->endGroup();
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
                QProcess::startDetached(app, QStringList() << action << m_fsModel->filePath( index ));
        else if ( selectionModel()->selectedIndexes().count() )
            foreach ( const QModelIndex &index, selectionModel()->selectedIndexes() )
                QProcess::startDetached(app, QStringList() << action << m_fsModel->filePath( index ));
    }
    else
    {
        QProcess::startDetached(app, QStringList() << action << m_fsModel->rootPath());
    }
}

void
ViewContainer::scriptTriggered()
{
    QStringList action(static_cast<QAction *>(sender())->data().toString().split(" "));
    const QString &app = action.takeFirst();
    qDebug() << "trying to launch" << app << "in" << m_fsModel->rootPath();
    QProcess::startDetached(app, QStringList() << m_fsModel->rootPath());
}

void
ViewContainer::activate(const QModelIndex &index)
{
    if ( !index.isValid() )
        return;

    if ( !m_fsModel->fileInfo(index).exists() )
        return;

    QProcess process;
    if (m_fsModel->isDir(index))
    {
        m_fsModel->setRootPath(m_fsModel->filePath(index));
        m_forwardList.clear();
    }
    else if(m_fsModel->fileInfo(index).isExecutable() && (m_fsModel->fileInfo(index).suffix().isEmpty() ||
                                                         m_fsModel->fileInfo(index).suffix() == "sh" ||
                                                         m_fsModel->fileInfo(index).suffix() == "exe")) // windows executable
    {
        QString file;
#ifdef Q_WS_X11 //linux
        file = m_fsModel->filePath(index);
#else
        file = m_fsModel->fileName(index);
#endif
        process.startDetached(file);
    }
    else
    {
        QStringList list;
        list << m_fsModel->filePath(index);

#ifdef Q_WS_X11 //linux
        process.startDetached("xdg-open", list);
#else //windows
        process.startDetached("cmd /c start " + list.at(0)); //todo: fix, this works but shows a cmd window real quick
#endif
    }
}

void
ViewContainer::rootPathChanged(QString index)
{
    emit currentPathChanged(index);
    m_selectModel->clearSelection();

    if(!m_back && m_fsModel->index(index).isValid())
        m_backList.append(index);

    m_back = false;

//    setRootIndex(m_fsModel->index(index));
    m_iconView->updateLayout();

    QStringList compList;
    QString separator = index == "/" ? 0L : "/";

    foreach(QString cEntry,m_fsModel->rootDirectory().entryList())
    {
        cEntry.prepend(index + separator);
        if(QFileInfo(cEntry).isDir())
            compList.append(cEntry);
    }

//    flowView->flowView()->setCurrentIndex(fsModel->index(0,0,fsModel->index(fsModel->rootPath())));
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
    m_forwardList << m_backList.last();
    m_backList.removeLast();
    m_fsModel->setRootPath(m_backList.last());
}

void
ViewContainer::goForward()
{
    if(m_forwardList.isEmpty())
        return;

    m_fsModel->setRootPath(m_forwardList.last());
    m_forwardList.removeLast();
}

bool
ViewContainer::goUp()
{
    if (m_breadCrumbs->currentPath() == "/")
        return false;
    m_fsModel->setRootPath(QFileInfo(m_fsModel->rootPath()).dir().path());
    return true;
}

void
ViewContainer::goHome()
{
    m_fsModel->setRootPath(QDir::homePath());
}

void
ViewContainer::refresh()
{
    m_fsModel->blockSignals(true);
    m_fsModel->setRootPath("");
    m_fsModel->setRootPath(m_breadCrumbs->currentPath());
    m_fsModel->blockSignals(false);
    setRootIndex(m_fsModel->index(m_breadCrumbs->currentPath()));
}

void
ViewContainer::rename()
{
    switch (m_myView)
    {
    case Icon : m_iconView->edit(m_iconView->currentIndex()); break;
    case Details : m_detailsView->edit(m_detailsView->currentIndex()); break;
    case Columns : m_columnsView->edit(m_columnsView->currentIndex()); break;
    case Flow : m_flowView->detailsView()->edit(m_flowView->detailsView()->currentIndex()); break;
    default: break;
    }
}

void
ViewContainer::setFilter(QString filter)
{
    VIEWS(setFilter(filter));
    m_dirFilter = filter;
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
        QFileInfo file(m_fsModel->filePath(delUs.at(i)));
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
        m_columnsView->scrollToTop();
        m_flowView->detailsView()->scrollToTop();
    }
    else
    {
        m_iconView->scrollTo(m_selectModel->selectedIndexes().first());
        m_detailsView->scrollTo(m_selectModel->selectedIndexes().first());
        m_columnsView->scrollTo(m_selectModel->selectedIndexes().first());
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
    args << (m_fsModel->filePath(idx).isNull() ? m_fsModel->rootPath() : m_fsModel->filePath(idx));
    QString program = args.takeFirst();
    QProcess process;
    process.startDetached(program,args);
}

QSize
ViewContainer::iconSize()
{
    return m_iconView->iconSize();
}

void
ViewContainer::createDirectory()
{
    m_fsModel->mkdir(m_fsModel->index(m_fsModel->rootPath()),"new_directory");
    QModelIndex newFolder = m_fsModel->index(m_fsModel->rootPath() + QDir::separator() + "new_directory");

    switch (m_myView)
    {
    case Icon : m_iconView->edit(newFolder); m_iconView->scrollTo(newFolder); break;
    case Details : m_detailsView->edit(newFolder); m_detailsView->scrollTo(newFolder); break;
    case Columns : m_columnsView->edit(newFolder); m_columnsView->scrollTo(newFolder); break;
    case Flow : m_flowView->detailsView()->edit(newFolder); m_flowView->detailsView()->scrollTo(newFolder); break;
    default: break;
    }
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
    emit newTabRequest(m_fsModel->filePath(index));
}

void
ViewContainer::setRootPath(QString rootPath)
{
    m_fsModel->setRootPath(rootPath);
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

void
ViewContainer::settingsChanged()
{
    if ( Configuration::config.views.singleClick )
    {
        disconnect( m_iconView, SIGNAL(activated(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)));
        disconnect( m_detailsView, SIGNAL(activated(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)));
        disconnect( m_columnsView, SIGNAL(activated(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)));
        disconnect( m_flowView->detailsView(), SIGNAL(activated(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)));
        connect( m_iconView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)) );
        connect( m_detailsView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)));
        connect( m_columnsView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)));
        connect( m_flowView->detailsView(), SIGNAL(clicked(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)));
    }
    else
    {
        disconnect( m_iconView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)) );
        disconnect( m_detailsView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)));
        disconnect( m_columnsView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)));
        disconnect( m_flowView->detailsView(), SIGNAL(clicked(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)));
        connect( m_iconView, SIGNAL(activated(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)));
        connect( m_detailsView, SIGNAL(activated(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)));
        connect( m_columnsView, SIGNAL(activated(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)));
        connect( m_flowView->detailsView(), SIGNAL(activated(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)));
    }
}
