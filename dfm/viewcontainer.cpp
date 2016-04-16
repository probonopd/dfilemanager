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
#include <QStackedLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QStyledItemDelegate>
#include <QMenu>

#include "viewcontainer.h"
#include "iconview.h"
#include "detailsview.h"
#include "flowview.h"
#include "flow.h"
#include "fsmodel.h"
#include "pathnavigator.h"
#include "operations.h"
#include "iojob.h"
#include "deletedialog.h"
#include "mainwindow.h"
#include "config.h"
#include "columnview.h"
#include "commanddialog.h"

using namespace DFM;

#define D_VIEW(_TYPE_, _METHOD_) _TYPE_##View *ViewContainer::_METHOD_##View() { return static_cast<_TYPE_##View *>(m_view[_TYPE_]); }
    D_VIEW(Icon, icon) D_VIEW(Details, details) D_VIEW(Column, column) D_VIEW(Flow, flow)
#undef D_VIEW

ViewContainer::ViewContainer(QWidget *parent)
    : QWidget(parent)
    , m_model(0)
    , m_viewStack(0)
    , m_navBar(0)
    , m_layout(0)
    , m_currentView(Icon)
    , m_back(false)
    , m_selectModel(0)
{
    m_view[Icon] = new IconView(this);
    m_view[Details] = new DetailsView(this);
    m_view[Column] = new ColumnView(this);
    m_view[Flow] = new FlowView(this);

    m_model = new FS::Model(this);
    m_selectModel = new QItemSelectionModel(m_model);

    m_navBar = new NavBar(this, m_model);
    connect(m_model, SIGNAL(finishedWorking()), m_navBar, SLOT(stopAnimating()));
    connect(m_model, SIGNAL(startedWorking()), m_navBar, SLOT(startAnimating()));

    m_navBar->setVisible(Store::settings()->value("pathVisible", true).toBool());

//    setFrameStyle(0/*QFrame::StyledPanel | QFrame::Sunken*/);
    setAutoFillBackground(false);

    connect(m_model, SIGNAL(urlChanged(QUrl)), this, SIGNAL(urlChanged(QUrl)));
    connect(m_model, SIGNAL(urlLoaded(QUrl)), this, SIGNAL(urlLoaded(QUrl)));
    connect(m_model, SIGNAL(hiddenVisibilityChanged(bool)), this, SIGNAL(hiddenVisibilityChanged(bool)));
    connect(m_model, SIGNAL(urlChanged(QUrl)), this, SLOT(setUrl(QUrl)));
    connect(m_model, SIGNAL(urlLoaded(QUrl)), this, SLOT(loadedUrl(QUrl)));
    connect(m_model, SIGNAL(sortingChanged(int,int)), this, SIGNAL(sortingChanged(int,int)));

    connect(m_selectModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SIGNAL(selectionChanged()));

    connect(iconView(), SIGNAL(iconSizeChanged(int)), this, SIGNAL(iconSizeChanged(int)));
    connect(flowView()->flow(), SIGNAL(centerIndexChanged(QModelIndex)), this, SIGNAL(entered(QModelIndex)));

    connect(iconView(), SIGNAL(newTabRequest(QModelIndex)), this, SLOT(genNewTabRequest(QModelIndex)));
    connect(detailsView(), SIGNAL(newTabRequest(QModelIndex)), this, SLOT(genNewTabRequest(QModelIndex)));
    connect(columnView(), SIGNAL(newTabRequest(QModelIndex)), this, SLOT(genNewTabRequest(QModelIndex)));
    connect(flowView(), SIGNAL(newTabRequest(QModelIndex)), this, SLOT(genNewTabRequest(QModelIndex)));

    m_viewStack = new QStackedLayout();
    m_viewStack->setSpacing(0);
    m_viewStack->setContentsMargins(0,0,0,0);
    for (int i = 0; i < NViews; ++i)
    {
        connect(m_view[i], SIGNAL(entered(QModelIndex)), this, SIGNAL(entered(QModelIndex)));
        connect(m_view[i], SIGNAL(viewportEntered()), this, SIGNAL(viewportEntered()));
        connect(m_view[i], SIGNAL(opened(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)));
        m_viewStack->addWidget(m_view[i]);
        m_view[i]->setMouseTracking(true);
        m_view[i]->setModel(m_model);
        m_view[i]->setSelectionModel(m_selectModel);
    }
    m_layout = new QVBoxLayout();
    m_layout->setSpacing(0);
    m_layout->setContentsMargins(0,0,0,0);
    m_layout->addLayout(m_viewStack, 1);
    m_layout->addWidget(m_navBar);

    setLayout(m_layout);

    setView((View)Store::config.behaviour.view, false);
    loadSettings();
}

ViewContainer::~ViewContainer()
{
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

FS::Model *ViewContainer::model() { return m_model; }

QItemSelectionModel *ViewContainer::selectionModel() { return m_selectModel; }

void
ViewContainer::setView(const View view, bool store)
{
    m_currentView = view;
    m_viewStack->setCurrentWidget(m_view[view]);
    emit viewChanged();

#if defined(ISUNIX)
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
        detailsView()->setItemsExpandable(false);
        m_selectModel->clearSelection();
    }
}

void
ViewContainer::loadedUrl(const QUrl &url)
{
    detailsView()->setItemsExpandable(true);
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
    m_view[m_currentView]->edit(m_view[m_currentView]->currentIndex());
}

void
ViewContainer::setFilter(const QString &filter)
{
    m_dirFilter = filter;
    if (QAbstractItemView *v = currentView())
    {
        if (v->rootIndex().data(FS::UrlRole).toUrl().isLocalFile())
            m_model->setFilter(filter, v->rootIndex().data(FS::FilePathRole).toString());
    }
    else
        m_model->setFilter(filter);
    emit filterChanged();
}

QString
ViewContainer::currentFilter() const
{
    if (QAbstractItemView *v = currentView())
    {
        if (v->rootIndex().data(FS::UrlRole).toUrl().isLocalFile())
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

    if (DeleteDialog(delUs).result() == QDialog::Rejected)
        return;

    if (!delList.isEmpty())
        IO::Manager::remove(delList);
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
    const QString file(m_model->url(idx).toLocalFile().isEmpty() ? m_model->rootUrl().toLocalFile() : m_model->url(idx).toLocalFile());
//    QString text = QInputDialog::getText(this, tr("Open With"), tr("Custom Command:"), QLineEdit::Normal, QString(), &ok);
    QString text = CommandDialog::getCommand(this, file, &ok);
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
    return m_view[m_currentView];
}

void
ViewContainer::setRootIndex(const QModelIndex &index)
{
    for (int i = 0; i < NViews; ++i)
        m_view[i]->setRootIndex(index);
}

void
ViewContainer::genNewTabRequest(const QModelIndex &index)
{
    const QFileInfo &fi = m_model->fileInfo(index);
    if (fi.exists())
        emit newTabRequest(QUrl::fromLocalFile(fi.filePath()));
}

Action
ViewContainer::viewAction(const View view)
{
    switch (view)
    {
    case ViewContainer::Icon: return Views_Icon;
    case ViewContainer::Details: return Views_Detail;
    case ViewContainer::Column: return Views_Column;
    case ViewContainer::Flow: return Views_Flow;
    default: return Views_Icon;
    }
}

QModelIndex ViewContainer::indexAt(const QPoint &p) const { return currentView()->indexAt(mapFromParent(p)); }

void ViewContainer::setPathVisible(bool visible) { m_navBar->setVisible(visible); }
bool ViewContainer::pathVisible() { return m_navBar->isVisible(); }

bool ViewContainer::canGoBack() { return m_model->canGoBack(); }
bool ViewContainer::canGoForward() { return m_model->canGoForward(); }

NavBar *ViewContainer::breadCrumbs() { return m_navBar; }

void ViewContainer::setPathEditable(const bool editable) { m_navBar->setEditable(editable); }

void ViewContainer::setIconSize(int stop)
{
    iconView()->setNewSize(stop);
    for (int i = 1; i < NViews; ++i)
    {
        m_view[i]->setIconSize(QSize(stop, stop));
        QList<QAbstractItemView *> kids(m_view[i]->findChildren<QAbstractItemView *>());
        for (int i = 0; i < kids.count(); ++i)
            kids.at(i)->setIconSize(QSize(stop, stop));
    }
}

const QSize ViewContainer::iconSize() const { return m_view[Icon]->iconSize(); }
