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

#include <QProcess>
#include <QMap>
#include <QInputDialog>
#include <QCompleter>
#include <QMenu>
#include <QDesktopServices>
#include <QAbstractItemView>

#include "viewcontainer.h"
#include "operations.h"
#include "iojob.h"
#include "deletedialog.h"
#include "mainwindow.h"

using namespace DFM;

#define VIEWS(RUN)\
    m_iconView->RUN;\
    m_detailsView->RUN;\
    m_columnsWidget->RUN;\
    m_flowView->RUN

ViewContainer::ViewContainer(QWidget *parent)
    : QFrame(parent)
    , m_model(new FS::Model(this))
    , m_viewStack(new QStackedWidget(this))
    , m_iconView(new IconView(this))
    , m_columnsWidget(new ColumnsWidget(this))
    , m_detailsView(new DetailsView(this))
    , m_flowView(new FlowView(this))
    , m_navBar(new NavBar(this, m_model))
    , m_layout(new QVBoxLayout())
{
    connect(m_model, SIGNAL(finishedWorking()), m_navBar, SLOT(stopAnimating()));
    connect(m_model, SIGNAL(startedWorking()), m_navBar, SLOT(startAnimating()));

    m_navBar->setVisible(Store::settings()->value("pathVisible", true).toBool());
    m_myView = Icon;
    m_back = false;

    m_viewStack->layout()->setSpacing(0);
    setFrameStyle(0/*QFrame::StyledPanel | QFrame::Sunken*/);
    setAutoFillBackground(false);

    setModel(m_model);
    setSelectionModel(new QItemSelectionModel(m_model));

    connect(m_model, SIGNAL(urlChanged(QUrl)), this, SIGNAL(urlChanged(QUrl)));
    connect(m_model, SIGNAL(urlLoaded(QUrl)), this, SIGNAL(urlLoaded(QUrl)));
    connect(selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SIGNAL(selectionChanged()));
    connect(m_model, SIGNAL(hiddenVisibilityChanged(bool)), this, SIGNAL(hiddenVisibilityChanged(bool)));
    connect(m_model, SIGNAL(urlChanged(QUrl)), this, SLOT(setUrl(QUrl)));
    connect(m_model, SIGNAL(urlLoaded(QUrl)), this, SLOT(loadedUrl(QUrl)));
    connect(m_iconView, SIGNAL(iconSizeChanged(int)), this, SIGNAL(iconSizeChanged(int)));
    connect(m_iconView, SIGNAL(newTabRequest(QModelIndex)), this, SLOT(genNewTabRequest(QModelIndex)));
    connect(m_detailsView, SIGNAL(newTabRequest(QModelIndex)), this, SLOT(genNewTabRequest(QModelIndex)));
    connect(m_flowView->detailsView(), SIGNAL(newTabRequest(QModelIndex)), this, SLOT(genNewTabRequest(QModelIndex)));
    connect(m_flowView->flow(), SIGNAL(centerIndexChanged(QModelIndex)), this, SIGNAL(entered(QModelIndex)));
    connect(m_model, SIGNAL(sortingChanged(int,int)), this, SIGNAL(sortingChanged(int,int)));

    foreach (QAbstractItemView *v, QList<QAbstractItemView *>() << m_iconView << m_detailsView /*<< m_columnsView*/ << m_flowView->detailsView())
    {
        v->setMouseTracking(true);
        connect(v, SIGNAL(entered(QModelIndex)), this, SIGNAL(entered(QModelIndex)));
        connect(v, SIGNAL(viewportEntered()), this, SIGNAL(viewportEntered()));
        connect(v, SIGNAL(opened(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)));
    }

    m_viewStack->addWidget(m_iconView);
    m_viewStack->addWidget(m_detailsView);
    m_viewStack->addWidget(m_columnsWidget);
    m_viewStack->addWidget(m_flowView);

    m_layout->addWidget(m_viewStack, 1);
    m_layout->addWidget(m_navBar);
    m_layout->setContentsMargins(0,0,0,0);
    m_layout->setSpacing(0);
    setLayout(m_layout);

    setView((View)Store::config.behaviour.view, false);
    loadSettings();
    emit iconSizeChanged(Store::config.views.iconView.iconSize*16);
}

ViewContainer::~ViewContainer()
{
//    qDebug() << "deleting container" << m_model->rootUrl() << this;
    delete m_iconView;
    delete m_detailsView;
    delete m_columnsWidget;
    delete m_flowView;
    delete m_navBar;
    delete m_model;
}

void
ViewContainer::loadSettings()
{
    QLayoutItem *m = m_layout->takeAt(m_layout->indexOf(m_navBar));
    QWidget *w = m->widget();
    delete m;
    m_layout->insertWidget(Store::config.behaviour.pathBarPlace, w);
    emit settingsChanged();
}

PathNavigator *ViewContainer::pathNav() { return m_navBar->pathNav(); }

void ViewContainer::setModel(FS::Model *model) { VIEWS(setModel(model)); }

QList<QAbstractItemView *>
ViewContainer::views()
{
    return QList<QAbstractItemView *>() << m_iconView << m_detailsView << m_columnsWidget->currentView() << m_flowView->detailsView();
}

void
ViewContainer::setSelectionModel(QItemSelectionModel *selectionModel)
{
    m_selectModel = selectionModel;
    VIEWS(setSelectionModel(selectionModel));
}

FS::Model *ViewContainer::model() { return m_model; }

QItemSelectionModel *ViewContainer::selectionModel() { return m_selectModel; }

void
ViewContainer::setView(const View view, bool store)
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
    if (Store::config.views.dirSettings && store && m_model->rootUrl().isLocalFile())
        Ops::writeDesktopValue<int>(QDir(m_model->rootUrl().toLocalFile()), "view", (int)view);
#endif
}

void
ViewContainer::activate(const QModelIndex &index)
{
    m_model->exec(index);
}

void
ViewContainer::setUrl(const QUrl &url)
{
    setRootIndex(m_model->index(url));
    const QFileInfo &file = url.toLocalFile();
    if (url.isLocalFile() && file.exists())
    {
        if (Store::config.views.dirSettings)
        {
            bool ok;
            View view = (View)Ops::getDesktopValue<int>(QDir(file.filePath()), "view", &ok);
            if (ok)
                setView(view, false);
        }
        m_detailsView->setItemsExpandable(false);
        m_selectModel->clearSelection();
    }
}

void
ViewContainer::loadedUrl(const QUrl &url)
{
    m_detailsView->setItemsExpandable(true);
}

void
ViewContainer::goBack()
{
    m_model->goBack();
}

void
ViewContainer::goForward()
{
    m_model->goForward();
}

bool
ViewContainer::goUp()
{
    const QModelIndex &index = m_model->index(m_navBar->url());
    if (!index.parent().isValid())
        return false;

    m_model->setUrl(m_model->url(index.parent()));
    return true;
}

void ViewContainer::goHome() { m_model->setUrl(QUrl::fromLocalFile(QDir::homePath())); }

void ViewContainer::refresh() { m_model->refresh(); }

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
ViewContainer::setFilter(const QString &filter)
{
    m_dirFilter = filter;
    if (QAbstractItemView *v = currentView())
    {
        if (v->rootIndex().data(FS::Url).toUrl().isLocalFile())
            m_model->setFilter(filter, v->rootIndex().data(FS::FilePathRole).toString());
    }
    else
        m_model->setFilter(filter);
    emit filterChanged();
}

QString ViewContainer::currentFilter() const
{
    if (QAbstractItemView *v = currentView())
    {
        if (v->rootIndex().data(FS::Url).toUrl().isLocalFile())
            return m_model->filter(v->rootIndex().data(FS::FilePathRole).toString());
    }
    else
        return m_model->filter();
    return QString();
}


void
ViewContainer::deleteCurrentSelection()
{
    QModelIndexList delUs;
    delUs = m_selectModel->selectedIndexes();

    QStringList delList;
    foreach (const QModelIndex &index, delUs)
    {
        if (index.column())
        {
            delUs.removeOne(index);
            continue;
        }
        const QFileInfo &file = m_model->fileInfo(index);
        if (file.isWritable())
            delList << file.filePath();
        else
            delUs.removeOne(index);
    }

    if (delUs.isEmpty())
        return;

    if (DeleteDialog(delUs).result() == 0)
        return;

    if (!delList.isEmpty())
        IO::Manager::remove(delList);
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
    if (m_selectModel->selectedRows(0).count())
        selectedItems = m_selectModel->selectedRows(0);
    else
        selectedItems = m_selectModel->selectedIndexes();

    QModelIndex idx;
    if (selectedItems.count() == 1)
        idx = selectedItems.first();
    bool ok;
    QString text = QInputDialog::getText(this, tr("Open With"), tr("Custom Command:"), QLineEdit::Normal, QString(), &ok);
    if (!ok)
        return;

    QStringList args(text.split(" "));
    args << (m_model->url(idx).toLocalFile().isEmpty() ? m_model->rootUrl().toLocalFile() : m_model->url(idx).toLocalFile());
    QString program = args.takeFirst();
    QProcess::startDetached(program, args);
}

void
ViewContainer::sort(const int column, const Qt::SortOrder order)
{
    if (column == m_model->sortColumn() && order == m_model->sortOrder())
        return;
    m_model->sort(column, order);
}

void
ViewContainer::createDirectory()
{
    if (QAbstractItemView *cv = currentView())
    {
        const QModelIndex &newFolder = m_model->mkdir(cv->rootIndex(), "new_directory");
        cv->scrollTo(newFolder);
        cv->edit(newFolder);
    }
}

QAbstractItemView
*ViewContainer::currentView() const
{
    if (m_myView == Columns)
        return m_columnsWidget->currentView();
    if (m_myView != Flow)
        return static_cast<QAbstractItemView *>(m_viewStack->currentWidget());
    return static_cast<QAbstractItemView *>(m_flowView->detailsView());
}

//void
//ViewContainer::resizeEvent(QResizeEvent *e)
//{
//    QWidget::resizeEvent(e);
//    m_searchIndicator->move(width()-256, height()-256);
//}

void
ViewContainer::setRootIndex(const QModelIndex &index)
{
    VIEWS(setRootIndex(index));
}

void
ViewContainer::genNewTabRequest(const QModelIndex &index)
{
    const QFileInfo &fi = m_model->fileInfo(index);
    if (fi.exists())
        emit newTabRequest(QUrl::fromLocalFile(fi.filePath()));
}

QModelIndex ViewContainer::indexAt(const QPoint &p) const { return currentView()->indexAt(mapFromParent(p)); }

bool ViewContainer::setPathVisible(bool visible) { m_navBar->setVisible(visible); }
bool ViewContainer::pathVisible() { return m_navBar->isVisible(); }

bool ViewContainer::canGoBack() { return m_model->canGoBack(); }
bool ViewContainer::canGoForward() { return m_model->canGoForward(); }

NavBar *ViewContainer::breadCrumbs() { return m_navBar; }

void ViewContainer::setPathEditable(const bool editable) { m_navBar->setEditable(editable); }

void ViewContainer::animateIconSize(int start, int stop) { m_iconView->setNewSize(stop); }
QSize ViewContainer::iconSize() { return m_iconView->iconSize(); }
