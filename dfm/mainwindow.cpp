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


#include "mainwindow.h"
#include "application.h"
#include "iojob.h"
#include "settingsdialog.h"
#include "propertiesdialog.h"
#include "tabbar.h"
#include <QToolTip>
#include <QClipboard>
#include <QMenuBar>
#include <QMessageBox>
#include <QList>
#include "iconprovider.h"

using namespace DFM;

MainWindow *MainWindow::s_currentWindow = 0;
QList<MainWindow *> MainWindow::s_openWindows;

MainWindow::MainWindow(const QStringList &arguments, bool autoTab)
    : QMainWindow()
    , m_sortButton(0)
    , m_activeContainer(0)
    , m_menuButton(0)
    , m_menuAction(0)
{ 
    addActions(Store::customActions());
    s_currentWindow = this;
    s_openWindows << this;
    QFrame *center = new QFrame(this);
    center->setFrameStyle(0/*QFrame::StyledPanel|QFrame::Sunken*/);
    m_tabWin = new QMainWindow(this);
    m_toolBar = new QToolBar(tr("Show ToolBar"), this);
    m_statusBar = new StatusBar(this);
    m_filterBox = new SearchBox(m_toolBar);
    m_toolBarSpacer = new QWidget(m_toolBar);
    m_dockLeft = new Docks::DockWidget(m_tabWin, tr("Bookmarks"), Qt::SubWindow, Docks::Left);
    m_dockRight = new Docks::DockWidget(m_tabWin, tr("Recent Folders"), Qt::SubWindow, Docks::Right);
    m_dockBottom = new Docks::DockWidget(m_tabWin, tr("Information"), Qt::SubWindow, Docks::Bottom);
    m_placesView = new PlacesView(m_dockLeft);
    m_infoWidget = new InfoWidget(m_dockBottom);
    m_recentFoldersView = new RecentFoldersView(m_dockRight);
    m_iconSizeSlider = new QSlider(m_statusBar);

    if (Store::config.behaviour.gayWindow)
    {
        FooBar *fooBar = new FooBar(this);
        fooBar->setVisible(true);
        m_tabBar = new TabBar(fooBar);
        fooBar->setTabBar(m_tabBar);
        Store::config.behaviour.hideTabBarWhenOnlyOneTab = false;
    }
    else
        m_tabBar = new TabBar(center);

    m_stackedWidget = new QStackedWidget(this);
    m_activeContainer = 0;
    m_model = 0;

    int is = 16;
    QSize tbs(QSize(is, is));
    m_toolBar->setIconSize(tbs);
    m_dockBottom->setWidget(m_infoWidget);

    m_tabWin->setWindowFlags(0);
    m_dockLeft->setWidget(m_placesView);
    m_dockLeft->setObjectName(tr("Bookmarks"));
    m_dockRight->setWidget(m_recentFoldersView);
    m_dockRight->setObjectName(tr("Recent Folders"));
    m_dockBottom->setObjectName(tr("Information"));

    m_cut = false;

    addToolBar(m_toolBar);
    m_toolBar->setObjectName("viewToolBar");

    setWindowIcon(QIcon::fromTheme("folder", QIcon(":/trolltech/styles/commonstyle/images/diropen-128.png")));

//    connect(m_placesView, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(rootFromPlaces(QTreeWidgetItem*,int)));
//    connect(m_placesView, SIGNAL(itemActivated(QTreeWidgetItem*,int)), this, SLOT(rootFromPlaces(QTreeWidgetItem*,int)));
    connect(m_filterBox, SIGNAL(textChanged(QString)),this,SLOT(filterCurrentDir(QString)));
    connect(m_tabBar, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
    connect(m_tabBar, SIGNAL(newTabRequest()), this, SLOT(newTab()));
    connect(m_placesView, SIGNAL(newTabRequest(QUrl)), this, SLOT(addTab(QUrl)));
    connect(m_placesView, SIGNAL(placeActivated(QUrl)), this, SLOT(activateUrl(QUrl)));
    connect(m_tabBar,SIGNAL(tabCloseRequested(int)), this, SLOT(tabClosed(int)));
    connect(m_tabBar, SIGNAL(tabMoved(int,int)), this, SLOT(tabMoved(int,int)));
    connect(m_stackedWidget, SIGNAL(currentChanged(int)), this, SLOT(stackChanged(int)));
    connect(m_recentFoldersView, SIGNAL(recentFolderClicked(QUrl)), this, SLOT(activateUrl(QUrl)));

    createActions();
    readSettings();
    createMenus();
    createToolBars();
    setupStatusBar();

    if (Store::config.behaviour.gayWindow)
        menuBar()->hide();

    m_tabBar->setDocumentMode(true);
    m_tabWin->setCentralWidget(m_stackedWidget);

    bool needTab = true;
    if (autoTab)
    {
        for (int i = 0; i<arguments.count(); ++i)
        {
            const QString &argument(arguments.at(i));
            if (argument == Application::applicationFilePath())
                continue;
            if (QFileInfo(Ops::sanityChecked(argument)).isDir())
            {
                addTab(QUrl::fromLocalFile(Ops::sanityChecked(argument)));
                needTab = false;
            }
        }
    }
    if (autoTab&&needTab)
        addTab(QUrl::fromLocalFile(Store::config.startPath));

    QVBoxLayout *vBox = new QVBoxLayout();
    vBox->setMargin(0);
    vBox->setSpacing(0);
    if (!Store::config.behaviour.gayWindow)
        vBox->addWidget(m_tabBar);
    vBox->addWidget(m_tabWin);
    center->setLayout(vBox);

    m_placesView->addActions(QList<QAction *>() << m_actions[AddPlace] << m_actions[RenamePlace] << m_actions[RemovePlace] << m_actions[AddPlaceContainer] << m_actions[SetPlaceIcon]);

    setCentralWidget(center);

    setUnifiedTitleAndToolBarOnMac(true);

    setStatusBar(m_statusBar);
    m_statusBar->installEventFilter(this);
    toggleMenuVisible();

    m_actions[ShowPathBar]->setChecked(Store::settings()->value("pathVisible", true).toBool());
    hidePath();

    m_placesView->installEventFilter(this);
    if (m_activeContainer)
        m_activeContainer->setFocus();
    m_statusBar->setVisible(m_actions[ShowStatusBar]->isChecked());
//    foreach (QAction *a, m_toolBar->actions())
//        connect(a, SIGNAL(changed()), this, SLOT(updateIcons()));
    QTimer::singleShot(0, this, SLOT(updateIcons()));
}

void
MainWindow::receiveMessage(const QStringList &message)
{
    bool isSearchPar = false, isStatus = false, isIoProgress = false;
    QString statusMsg;
    foreach (const QString &msg, message)
        if (isSearchPar)
        {
            m_model->search(msg);
        }
        else if (isStatus)
        {
            if (!statusMsg.isEmpty())
                statusMsg.append(" ");
            statusMsg.append(msg);
        }
        else if (isIoProgress)
        {
            bool ok;
            int ioProgress = msg.toInt(&ok);
            if (ok)
            {
                if (ioProgress == -1) //busy...
                {
                    m_ioProgress->setVisible(true);
                    m_ioProgress->setValue(0);
                    m_ioProgress->setRange(0, 0);
                }
                else
                {
                    if (ioProgress == 0)
                        m_ioProgress->reset();
                    m_ioProgress->setRange(0, 100);
                    m_ioProgress->setVisible(ioProgress < 100);
                    m_ioProgress->setValue(ioProgress);
                }
            }
            else
            {
                QString format;
                if (m_ioProgress->maximum())
                    format = QString("%1 %p%").arg(msg);
                else
                    format = msg;
                m_ioProgress->setFormat(format);
            }
        }
        else if (QFileInfo(msg).isDir())
        {
            addTab(msg);
            m_tabBar->setCurrentIndex(m_tabBar->count()-1);
        }
        else if (msg == "--getKdePlaces")
        {
            if (!m_placesView->getKdePlaces())
                QMessageBox::warning(this, "Getting places from kde Failed!", "was unable to load kde places... sry.");
        }
        else if (msg == "--search")
        {
            isSearchPar = true;
        }
        else if (msg == "--status")
        {
            isStatus = true;
        }
        else if (msg == "--ioProgress")
        {
            isIoProgress = true;
        }
        else
        {
            setWindowState(windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
        }
}

void
MainWindow::filterCurrentDir(const QString &filter)
{
    if (m_filterBox->mode() == Filter)
        m_activeContainer->setFilter(filter);
}

void
MainWindow::setupStatusBar()
{
    m_iconSizeSlider->setFixedWidth(80);
    m_iconSizeSlider->setOrientation(Qt::Horizontal);
    m_iconSizeSlider->setRange(1,16);
    m_iconSizeSlider->setSingleStep(1);
    m_iconSizeSlider->setPageStep(1);
    m_statusBar->addRightWidget(m_iconSizeSlider);
    Docks::FloatButton *fl = new Docks::FloatButton(m_statusBar);
    fl->setFixedSize(16, 16);
    m_statusBar->addLeftWidget(fl);
    fl->setFloating(m_dockLeft->isVisible());
    Docks::FloatButton *fb = new Docks::FloatButton(m_statusBar, Docks::Bottom);
    fb->setFixedSize(16, 16);
    m_statusBar->addLeftWidget(fb);
    fb->setFloating(m_dockBottom->isVisible());
    Docks::FloatButton *fr = new Docks::FloatButton(m_statusBar, Docks::Right);
    fr->setFixedSize(16, 16);
    m_statusBar->addLeftWidget(fr);
    fr->setFloating(m_dockRight->isVisible());

    connect (fl, SIGNAL(clicked()), m_dockLeft, SLOT(toggleVisibility()));
    connect (fr, SIGNAL(clicked()), m_dockRight, SLOT(toggleVisibility()));
    connect (fb, SIGNAL(clicked()), m_dockBottom, SLOT(toggleVisibility()));

    connect (fl, SIGNAL(rightClicked()), m_dockLeft, SLOT(toggleLock()));
    connect (fr, SIGNAL(rightClicked()), m_dockRight, SLOT(toggleLock()));
    connect (fb, SIGNAL(rightClicked()), m_dockBottom, SLOT(toggleLock()));

    connect (m_dockLeft, SIGNAL(visibilityChanged(bool)), fl, SLOT(setFloating(bool)));
    connect (m_dockRight, SIGNAL(visibilityChanged(bool)), fr, SLOT(setFloating(bool)));
    connect (m_dockBottom, SIGNAL(visibilityChanged(bool)), fb, SLOT(setFloating(bool)));

    connect (m_iconSizeSlider, SIGNAL(sliderMoved(int)),this,SLOT(setViewIconSize(int)));
    connect (m_iconSizeSlider, SIGNAL(valueChanged(int)),this,SLOT(setViewIconSize(int)));

    m_ioProgress = new QProgressBar(m_statusBar);
    m_ioProgress->setRange(0, 100);
    m_ioProgress->setFixedWidth(92);
    m_statusBar->addRightWidget(m_ioProgress);
    m_ioProgress->setTextVisible(true);
    m_ioProgress->setVisible(false);
}

void
MainWindow::setSliderPos(int size)
{
    m_iconSizeSlider->setValue(size/16);
}

void
MainWindow::setViewIconSize(int size)
{
    m_activeContainer->animateIconSize(m_activeContainer->iconSize().width(), size*16);
    m_iconSizeSlider->setToolTip(QString("Size: %1 px").arg(QString::number(size*16)));
    QToolTip *tip;
    QPoint pt;
    pt.setX(mapToGlobal(m_iconSizeSlider->pos()).x());
    pt.setY(mapToGlobal(m_statusBar->pos()).y());
    if (m_statusBar->isVisible())
        tip->showText(pt, m_iconSizeSlider->toolTip());
}

void
MainWindow::mainSelectionChanged()
{
    const QModelIndexList &selected = m_activeContainer->selectionModel()->selectedRows(0);
    if (selected.count() == 1)
        m_slctnMessage = QString(" :: \'%1\' Selected").arg(selected.first().data().toString());
    else if (selected.count() > 1)
        m_slctnMessage = QString(" :: %1 Items Selected").arg(QString::number(selected.count()));

    const QString &newMessage = selected.isEmpty() ? m_statusMessage : m_statusMessage + m_slctnMessage;
    m_statusBar->showMessage(newMessage);
}

void
MainWindow::cutSelection()
{
    setClipBoard();
    m_cut = true;
}

void
MainWindow::copySelection()
{
    setClipBoard();
    m_cut = false;
}

void
MainWindow::togglePath()
{
    m_activeContainer->setPathEditable(m_actions[EditPath]->isChecked());
}

void
MainWindow::setClipBoard()
{
    QModelIndexList selectedItems;
    if(m_activeContainer->selectionModel()->selectedRows(0).count())
        selectedItems = m_activeContainer->selectionModel()->selectedRows(0);
    else
        selectedItems = m_activeContainer->selectionModel()->selectedIndexes();

    QApplication::clipboard()->setMimeData(m_model->mimeData(selectedItems));
}

void
MainWindow::pasteSelection()
{
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    const QString &filePath = m_model->url(m_activeContainer->currentView()->rootIndex()).toLocalFile();

    if (QFileInfo(filePath).exists() && mimeData->hasUrls())
        IO::Manager::copy(mimeData->urls(), filePath, m_cut, m_cut);

    QApplication::clipboard()->clear();
    m_cut = false;
}

void
MainWindow::urlChanged(const QUrl &url)
{
    if (sender() != m_model)
        return;
    const QString &title = m_model->title(url);
    setWindowTitle(title);
    if (m_filterBox->mode() == Filter)
        m_filterBox->clear();
    if (url.isLocalFile())
        m_placesView->activateAppropriatePlace(url.toLocalFile());
    m_tabBar->setTabText(m_tabBar->currentIndex(), title);
    if (Store::config.behaviour.gayWindow)
        m_tabBar->setTabIcon(m_tabBar->currentIndex(), m_model->fileIcon(m_model->index(url)));
    setActions();
}

void
MainWindow::updateStatusBar(const QUrl &url)
{
    m_statusMessage = Ops::getStatusBarMessage(url, m_model);
    m_statusBar->showMessage(m_statusMessage);
}

void
MainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();
    s_openWindows.removeOne(this);
    event->accept();
}

void
MainWindow::keyPressEvent(QKeyEvent *ke)
{
    if (ke->key() == Qt::Key_I && (ke->modifiers() & Qt::CTRL))
        m_filterBox->setFocus();
    if (focusWidget() == m_filterBox && ke->key() == Qt::Key_Escape)
    {
        m_filterBox->clear();
        m_filterBox->clearFocus();
        filterCurrentDir("");
    }
//    if(ke->key() == Qt::Key_Return)
//        m_activeContainer->doubleClick(m_activeContainer->selectionModel()->selectedIndexes().first());
    QMainWindow::keyPressEvent(ke);
}

bool
MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_activeContainer && event->type() == QEvent::HoverLeave)
    {
        QString newMessage = (m_activeContainer->selectionModel()->selection().isEmpty() ||
                               m_activeContainer->selectionModel()->currentIndex().parent() !=
                m_model->index(m_model->rootUrl())) ? m_statusMessage : m_statusMessage + m_slctnMessage;
        if(m_statusBar->currentMessage() != newMessage)
            m_statusBar->showMessage(newMessage);
        return false;
    }

    if (obj == m_placesView && (event->type() == QEvent::Resize || event->type() == QEvent::Show) && !m_dockLeft->isFloating())
        updateToolbarSpacer();

//    if (obj == m_statusBar && event->type() == QEvent::Paint)
//    {
//        QPainter p(m_statusBar);
//        p.setPen(m_statusBar->palette().color(m_statusBar->foregroundRole()));
//        p.drawText(m_statusBar->rect(), Qt::AlignCenter, m_statusBar->currentMessage());
//        return true;
//    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::rename() { m_activeContainer->rename(); }
void MainWindow::createDirectory() { m_activeContainer->createDirectory(); }

void MainWindow::goHome() { m_model->setUrl(QUrl::fromLocalFile(QDir::homePath())); }
void MainWindow::goBack() { m_activeContainer->goBack(); }
void MainWindow::goUp() { m_activeContainer->goUp(); }
void MainWindow::goForward() { m_activeContainer->goForward(); }

void MainWindow::setViewIcons() { m_activeContainer->setView(ViewContainer::Icon); }
void MainWindow::setViewDetails() { m_activeContainer->setView(ViewContainer::Details); }
void MainWindow::setViewCols() { m_activeContainer->setView(ViewContainer::Columns); }
void MainWindow::flowView() { m_activeContainer->setView(ViewContainer::Flow); }

void
MainWindow::genPlace()
{
    if (!m_model->rootUrl().isLocalFile())
        return;
    QFileInfo file(m_model->rootUrl().toLocalFile());
    QString path = file.filePath();
    QString name = file.fileName();
    QIcon icon = QIcon::fromTheme("folder");
    m_placesView->addPlace(name, path, icon);
}

MainWindow::Actions
MainWindow::viewAction(const ViewContainer::View view)
{
    switch (view)
    {
    case ViewContainer::Icon: return IconView;
    case ViewContainer::Details: return DetailView;
    case ViewContainer::Columns: return ColumnView;
    case ViewContainer::Flow: return FlowView;
    default: return IconView;
    }
}

void
MainWindow::checkViewAct()
{
    m_actions[viewAction(m_activeContainer->currentViewType())]->setChecked(true);
    m_iconSizeSlider->setVisible(m_activeContainer->currentViewType() == ViewContainer::Icon);
    emit viewChanged(m_activeContainer->currentView());
}

void
MainWindow::about()
{
    QMessageBox::about(this, tr("About dfm"),
                       tr("<b>dfm</b> is a file manager "
                          "that really is a file browser, "
                          "that can manage files."));
}

void MainWindow::deleteCurrentSelection() { m_activeContainer->deleteCurrentSelection(); }

void MainWindow::toggleHidden() { m_model->setHiddenVisible(m_actions[ShowHidden]->isChecked()); }

void MainWindow::refreshView() { m_activeContainer->refresh(); }

void MainWindow::customCommand() { m_activeContainer->customCommand(); }

void
MainWindow::toggleMenuVisible()
{
    menuBar()->setVisible(m_actions[ShowMenuBar]->isChecked());
    m_menuAction->setVisible(!m_actions[ShowMenuBar]->isChecked());
}

void MainWindow::toggleStatusVisible() { m_statusBar->setVisible(m_actions[ShowStatusBar]->isChecked()); }

void
MainWindow::connectContainer(ViewContainer *container)
{
    container->installEventFilter(this);
    connect(container->model(), SIGNAL(urlChanged(QUrl)), this, SLOT(urlChanged(QUrl)));
    connect(container->model(), SIGNAL(urlLoaded(QUrl)), this, SLOT(updateStatusBar(QUrl)));
    connect(container->model(), SIGNAL(urlChanged(QUrl)), m_recentFoldersView, SLOT(folderEntered(QUrl)));
    connect(container, SIGNAL(viewChanged()), this, SLOT(checkViewAct()));
    connect(container->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(mainSelectionChanged()));
    connect(container, SIGNAL(iconSizeChanged(int)), this, SLOT(setSliderPos(int)));
    connect(container, SIGNAL(viewportEntered()), this, SLOT(viewClearHover()));
    connect(container, SIGNAL(leftView()), this, SLOT(viewClearHover()));
    connect(container, SIGNAL(newTabRequest(QUrl)), this, SLOT(addTab(QUrl)));
    connect(container, SIGNAL(entered(QModelIndex)), m_infoWidget, SLOT(hovered(QModelIndex)));
    connect(container, SIGNAL(entered(QModelIndex)), this, SLOT(viewItemHovered(QModelIndex)));
    connect(container, SIGNAL(sortingChanged(int,int)), this, SLOT(sortingChanged(int,int)));
    connect(container->model(), SIGNAL(hiddenVisibilityChanged(bool)), m_actions[ShowHidden], SLOT(setChecked(bool)));

    QList<QAction *> actList;
    actList << m_actions[OpenInTab] << m_actions[MkDir] << m_actions[Paste] << m_actions[Copy] << m_actions[Cut]
            << m_actions[DeleteSelection] << m_actions[Rename] << m_actions[CustomCommand] << m_actions[GoUp]
            << m_actions[GoBack] << m_actions[GoForward] << m_actions[Properties];

    container->addActions(actList);
}

void
MainWindow::addTab(const QUrl &url)
{
    ViewContainer *container = new ViewContainer(this, url);
    connectContainer(container);
    m_stackedWidget->addWidget(container);
    if (!m_model)
        m_model = container->model();

    const QString &title = container->model()->title(url);

    if (Store::config.behaviour.gayWindow)
        m_tabBar->addTab(m_model->fileIcon(m_model->index(url)), title);
    else
        m_tabBar->addTab(title);

    if (Store::config.behaviour.hideTabBarWhenOnlyOneTab)
        m_tabBar->setVisible(m_tabBar->count() > 1);
}

void
MainWindow::addTab(ViewContainer *container, int index)
{
    if (index == -1)
        index = m_stackedWidget->count();
    connectContainer(container);
    m_stackedWidget->insertWidget(index, container);
    if (!m_model)
        m_model = container->model();
    if (!m_activeContainer)
        m_activeContainer = container;


    FS::Model *model = container->model();
    const QUrl &url = model->rootUrl();
    const QString &title = model->title(url);
    const QModelIndex &idx = model->index(url);

    if (Store::config.behaviour.gayWindow)
        m_tabBar->insertTab(index, model->fileIcon(idx), title);
    else
        m_tabBar->insertTab(index, title);

    if (Store::config.behaviour.hideTabBarWhenOnlyOneTab)
        m_tabBar->setVisible(m_tabBar->count() > 1);
}

void
MainWindow::tabChanged(int currentIndex)
{
    if (currentIndex < 0 || !m_stackedWidget->count())
        return;

    DataLoader::clearQueue();
    m_stackedWidget->setCurrentIndex(currentIndex);
    m_activeContainer = static_cast<ViewContainer *>(m_stackedWidget->currentWidget());
    emit viewChanged(m_activeContainer->currentView());
    m_model = m_activeContainer->model();

    sortingChanged(m_activeContainer->model()->sortColumn(), (int)m_activeContainer->model()->sortOrder());
    m_recentFoldersView->folderEntered(m_model->rootUrl());
    setWindowTitle(m_tabBar->tabText(currentIndex));
    m_activeContainer->setPathEditable(m_actions[EditPath]->isChecked());
    m_iconSizeSlider->setValue(m_activeContainer->iconSize().width()/16);
    m_iconSizeSlider->setToolTip("Size: " + QString::number(m_activeContainer->iconSize().width()) + " px");
    m_placesView->activateAppropriatePlace(m_model->rootUrl().toLocalFile()); //TODO: placesview url based
    updateStatusBar(m_model->rootUrl());
    m_filterBox->setText(m_model->currentSearchString());
    m_actions[ShowHidden]->setChecked(m_model->showHidden());
    setActions();
    if (m_activeContainer)
        m_activeContainer->setFocus();
}

void
MainWindow::tabMoved(int from, int to)
{
    QWidget *w = m_stackedWidget->widget(from);

    m_stackedWidget->removeWidget(w);
    m_stackedWidget->insertWidget(to, w);
    if (to == m_tabBar->currentIndex())
        m_stackedWidget->setCurrentWidget(m_stackedWidget->widget(to));
}

void
MainWindow::tabClosed(int tab)
{
    QWidget *removedWidget = m_stackedWidget->widget(tab);
    m_stackedWidget->removeWidget(removedWidget); //somewhy it's important to first remove from stack, then tabbar
    m_tabBar->removeTab(tab);
    removedWidget->deleteLater();
    m_activeContainer->setFocus();
    if (Store::config.behaviour.hideTabBarWhenOnlyOneTab)
        m_tabBar->setVisible(m_tabBar->count() > 1);
}

void
MainWindow::openTab()
{
    if (m_activeContainer->selectionModel()->hasSelection())
        foreach (const QModelIndex &index, m_activeContainer->selectionModel()->selectedIndexes())
        {
            if (index.column() > 0)
                continue;
            const QFileInfo &file = m_model->fileInfo(index);
            if (file.exists())
                addTab(QUrl::fromLocalFile(file.filePath()));
        }
}

void MainWindow::newTab() { addTab(m_model->rootUrl()); }

ViewContainer
*MainWindow::takeContainer(int tab)
{
    ViewContainer *container = qobject_cast<ViewContainer *>(m_stackedWidget->widget(tab));
    if (!container)
        return 0;

    if (m_activeContainer == container)
    {
        m_activeContainer = 0;
        m_model = 0;
    }
    m_stackedWidget->removeWidget(container);
    m_tabBar->removeTab(tab);

    container->removeEventFilter(this);
    container->disconnect(this);
    container->model()->disconnect(this);
    container->selectionModel()->disconnect(this);
    container->setParent(0);
    foreach (QAction *a, container->actions())
        container->removeAction(a);

    if (!m_stackedWidget->count())
    {
        hide();
        QTimer::singleShot(250, this, SLOT(deleteLater()));
    }
    return container;
}

void
MainWindow::setActions()
{
    m_actions[GoBack]->setEnabled(m_activeContainer->canGoBack());
    m_actions[GoForward]->setEnabled(m_activeContainer->canGoForward());
//    m_pathVisibleAct->setChecked(m_activeContainer);
    checkViewAct();
}

void
MainWindow::viewItemHovered(const QModelIndex &index)
{
    if (!m_model || m_model->isWorking() || !index.isValid() || index.row() > 100000 || index.column() > 3)
        return;
    const QFileInfo &file = m_model->fileInfo(index);
    if (!file.exists())
        return;
    QString message = file.filePath();
    if (file.isSymLink())
        message.append(QString(" -> %1").arg(file.symLinkTarget()));
    m_statusBar->showMessage(message);
}

void
MainWindow::viewClearHover()
{
    if (!m_activeContainer)
        return;
    QString newMessage = (m_activeContainer->selectionModel()->selection().isEmpty()
                           ||  m_activeContainer->selectionModel()->currentIndex().parent() != m_model->index(m_model->rootUrl()))
            ? m_statusMessage : m_statusMessage + m_slctnMessage;
    if(m_statusBar->currentMessage() != newMessage)
        m_statusBar->showMessage(newMessage);
}

void
MainWindow::showSettings()
{
    SettingsDialog().exec();
}

void
MainWindow::fileProperties()
{
    QStringList files;
    foreach (const QModelIndex &index, m_activeContainer->selectionModel()->selectedIndexes())
    {
        if (index.column() != 0)
            continue;
        const QFileInfo &fi = m_model->fileInfo(index);
        if (fi.exists())
            files << fi.filePath();
    }
    PropertiesDialog::forFiles(files);
}

void
MainWindow::hidePath()
{
    foreach (NavBar *bc, findChildren<NavBar *>())
        bc->setVisible(m_actions[ShowPathBar]->isChecked());
    Store::settings()->setValue("pathVisible", m_actions[ShowPathBar]->isChecked());
}

void
MainWindow::sortingChanged(const int column, const int order)
{
    Store::config.behaviour.sortingCol=column;
    Store::config.behaviour.sortingOrd=(Qt::SortOrder)order;

    m_actions[SortName]->setChecked(column == 0);
    m_actions[SortSize]->setChecked(column == 1);
    m_actions[SortType]->setChecked(column == 2);
    m_actions[SortDate]->setChecked(column == 3);
    m_actions[SortDescending]->setChecked((Qt::SortOrder)order == Qt::DescendingOrder);
}

void
MainWindow::setSorting()
{
    int i = 0;
    if (m_actions[SortName]->isChecked())
        i = 0;
    else if (m_actions[SortSize]->isChecked())
        i = 1;
    else if (m_actions[SortType]->isChecked())
        i = 2;
    else if (m_actions[SortDate]->isChecked())
        i = 3;

    m_activeContainer->sort(i, (Qt::SortOrder)m_actions[SortDescending]->isChecked());
}

void
MainWindow::stackChanged(int)
{
    if (m_activeContainer)
        m_filterBox->setText(m_model->currentSearchString());
}

void
MainWindow::showEvent(QShowEvent *e)
{
    QMainWindow::showEvent(e);
    int w = m_placesView->viewport()->width();
    w -= m_toolBar->widgetForAction(m_actions[GoBack])->width();
    w -= m_toolBar->widgetForAction(m_actions[GoForward])->width();
    w -= style()->pixelMetric(QStyle::PM_ToolBarSeparatorExtent);
    if (m_toolBarSpacer->width() != w)
    {
        const int width = qMax(0, w + style()->pixelMetric(QStyle::PM_DockWidgetSeparatorExtent)/2);
        m_toolBarSpacer->setFixedWidth(width);
    }
}

void
MainWindow::windowActivationChange(bool wasActive)
{
    if (s_currentWindow)
        disconnect(APP, SIGNAL(lastMessage(QStringList)), s_currentWindow, SLOT(receiveMessage(QStringList)));
    if (!wasActive)
    {
        s_currentWindow = this;
        if (m_activeContainer)
            emit viewChanged(m_activeContainer->currentView());
    }
    if (s_currentWindow)
        connect(APP, SIGNAL(lastMessage(QStringList)), s_currentWindow, SLOT(receiveMessage(QStringList)));
    if (Store::config.behaviour.gayWindow)
        foreach (WinButton *btn, findChildren<WinButton *>())
        {
            btn->setProperty("isActive", isActiveWindow());
            btn->setStyleSheet(qApp->styleSheet());
            btn->update();
        }
}

void
MainWindow::readSettings()
{
    restoreState(Store::settings()->value("windowState").toByteArray(), 1);
    m_tabWin->restoreState(Store::settings()->value("tabWindowState").toByteArray(), 1);
    resize(Store::settings()->value("size", QSize(800, 300)).toSize());
    move(Store::settings()->value("pos", QPoint(200, 200)).toPoint());

    m_tabWin->addDockWidget(Qt::LeftDockWidgetArea, m_dockLeft);
    m_tabWin->addDockWidget(Qt::RightDockWidgetArea, m_dockRight);
    m_tabWin->addDockWidget((Qt::DockWidgetArea)Store::config.docks.infoArea, m_dockBottom);

    m_actions[ShowStatusBar]->setChecked(Store::settings()->value("statusVisible", true).toBool());
    m_actions[ShowPathBar]->setChecked(Store::settings()->value("pathVisible", true).toBool());
    m_actions[EditPath]->setChecked(Store::settings()->value("pathEditable", false).toBool());
    m_actions[ShowMenuBar]->setChecked(Store::settings()->value("menuVisible", true).toBool());
    m_placesView->populate();
    updateConfig();
}

void
MainWindow::updateToolbarSpacer()
{
    int w = m_placesView->viewport()->width();
    w -= m_toolBar->widgetForAction(m_actions[GoBack])->width();
    w -= m_toolBar->widgetForAction(m_actions[GoForward])->width();
    w -= style()->pixelMetric(QStyle::PM_ToolBarSeparatorExtent);
    if (m_toolBarSpacer->width() != w)
    {
        const int width = qMax(0, w + style()->pixelMetric(QStyle::PM_DockWidgetSeparatorExtent)/2);
        m_toolBarSpacer->setFixedWidth(width);
    }
    QTimer::singleShot(0, m_toolBar, SLOT(update()));
}

void
MainWindow::updateIcons()
{
    if (!m_sortButton)
        return;

    QWidget *firstTB = m_toolBar->widgetForAction(m_actions[GoBack]);
    QPixmap pix(firstTB->size());
    pix.fill(Qt::transparent);
    firstTB->render(&pix);
    QImage img = pix.toImage();
    QRgb *rgbPx = reinterpret_cast<QRgb *>(img.bits());
    const int size = img.width()*img.height();
    unsigned int value = 0;
    for (int i=0; i<size; ++i)
        value += QColor(rgbPx[i]).value();

    QColor fg = m_toolBar->palette().color(m_toolBar->foregroundRole());
    if (qAbs((value/size)-fg.value()) < 127) //not enough contrast...
    {
        qDebug() << "not enough contrast... calculating new icons color for toolbuttons";
//        fg.setHsv(fg.hue(), fg.saturation(), 255-fg.value());
        fg = m_toolBar->palette().color(m_toolBar->backgroundRole());
    }


#define SETICON(_ICON_) setIcon(IconProvider::icon(_ICON_, m_toolBar->iconSize().height(), fg, Store::config.behaviour.systemIcons))
    m_actions[GoBack]->SETICON(IconProvider::GoBack);
    m_actions[GoForward]->SETICON(IconProvider::GoForward);
    m_actions[IconView]->SETICON(IconProvider::IconView);
    m_actions[DetailView]->SETICON(IconProvider::DetailsView);
    m_actions[ColumnView]->SETICON(IconProvider::ColumnsView);
    m_actions[FlowView]->SETICON(IconProvider::FlowView);
    m_actions[Configure]->SETICON(IconProvider::Configure);
    m_actions[GoHome]->SETICON(IconProvider::GoHome);
    m_actions[ShowHidden]->SETICON(IconProvider::Hidden);

    m_sortButton->setMinimumWidth(32);
    m_sortButton->SETICON(IconProvider::Sort);
    m_menuButton->SETICON(IconProvider::Configure);
#undef SETICON

    m_toolBar->widgetForAction(m_actions[ShowHidden])->setMinimumWidth(32);
    m_toolBar->widgetForAction(m_actions[GoHome])->setMinimumWidth(32);
    m_toolBar->widgetForAction(m_actions[Configure])->setMinimumWidth(32);

    m_dockLeft->setContentsMargins(0, 0, 0, 0);
    m_dockRight->setContentsMargins(0, 0, 0, 0);
    m_dockBottom->setContentsMargins(0, 0, 0, 0);

    QTimer::singleShot(0, this, SLOT(updateToolbarSpacer()));
}

void
MainWindow::updateConfig()
{
    m_tabBar->setVisible(m_tabBar->count() > 1 ? true : !Store::config.behaviour.hideTabBarWhenOnlyOneTab);
    if (m_activeContainer)
        m_activeContainer->refresh();
    updateIcons();
    m_placesView->viewport()->update();
    emit settingsChanged();
}

void
MainWindow::newWindow()
{
    const QFileInfo &fi = m_model->rootUrl().toLocalFile();
    if (fi.exists())
        (new MainWindow(QStringList() << fi.filePath()))->show();
}

void
MainWindow::writeSettings()
{
    Store::settings()->setValue("pos", pos());
    Store::settings()->setValue("size", size());
    Store::settings()->setValue("windowState", saveState(1));
    Store::settings()->setValue("tabWindowState", m_tabWin->saveState(1));
    Store::settings()->setValue("statusVisible", m_actions[ShowStatusBar]->isChecked());
    Store::settings()->setValue("pathVisible", m_actions[ShowPathBar]->isChecked());
    Store::settings()->setValue("pathEditable", m_actions[EditPath]->isChecked());
    Store::settings()->setValue("menuVisible", m_actions[ShowMenuBar]->isChecked());
    Store::config.behaviour.sortingCol = m_model->sortColumn();
    Store::config.behaviour.sortingOrd = m_model->sortOrder();
    Store::writeConfig();
    m_placesView->store();
}

void
MainWindow::activateUrl(const QUrl &url)
{
    if (!m_activeContainer)
        return;
    if (!m_activeContainer->model())
        return;
    m_activeContainer->model()->setUrl(url);
}

MainWindow *MainWindow::currentWindow() { return s_currentWindow; }
QList<MainWindow *> MainWindow::openWindows() { return s_openWindows; }
ViewContainer *MainWindow::currentContainer() { return s_currentWindow->activeContainer(); }
PlacesView *MainWindow::places() { return s_currentWindow->placesView(); }

