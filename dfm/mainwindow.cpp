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

#include <QFileSystemWatcher>
#include <QProcess>
#include <QProgressBar>
#include <QMimeData>
#include <QToolTip>
#include <QClipboard>
#include <QMenuBar>
#include <QMessageBox>
#include <QList>
#include "viewcontainer.h"
#include "fsmodel.h"
#include "placesview.h"
#include "iconview.h"
#include "detailsview.h"
#include "flowview.h"
#include "searchbox.h"
#include "tabbar.h"
#include "config.h"
#include "infowidget.h"
#include "dockwidget.h"
#include "titlewidget.h"
#include "recentfoldersview.h"
#include "mainwindow.h"
#include "application.h"
#include "iojob.h"
#include "settingsdialog.h"
#include "propertiesdialog.h"
#include "tabbar.h"
#include "iconprovider.h"
#include "pathnavigator.h"
#include "dataloader.h"

using namespace DFM;

MainWindow *MainWindow::s_currentWindow = 0;
QList<MainWindow *> MainWindow::s_openWindows;

MainWindow::MainWindow(const QStringList &arguments, bool autoTab)
    : QMainWindow()
    , m_sortButton(0)
    , m_menuButton(0)
    , m_menuAction(0)
    , m_tabBar(0)
    , m_tabWin(0)
    , m_toolBar(0)
    , m_statusBar(0)
    , m_cut(false)
{ 
    s_openWindows << this;
    s_currentWindow = this;
    m_tabWin = new QMainWindow(this);

    QFrame *center = new QFrame(this);
    center->setFrameStyle(0);
    m_statusBar = new StatusBar(this);
    m_toolBar = new QToolBar(tr("Show ToolBar"), this);
    m_filterBox = new SearchBox(m_toolBar);
    m_toolBarSpacer = new QWidget(m_toolBar);
    m_sortButton = new QToolButton(m_toolBar);
    m_menuButton = new QToolButton(m_toolBar);
    m_placesDock = new Docks::DockWidget(m_tabWin, tr("Bookmarks"), Qt::SubWindow, Docks::Left);
    m_recentDock = new Docks::DockWidget(m_tabWin, tr("Recent Folders"), Qt::SubWindow, Docks::Right);
    m_infoDock = new Docks::DockWidget(m_tabWin, tr("Information"), Qt::SubWindow, Docks::Bottom);
    m_placesView = new PlacesView(m_placesDock);
    m_infoWidget = new InfoWidget(m_infoDock);
    m_recentFoldersView = new RecentFoldersView(m_recentDock);
    m_iconSizeSlider = new QSlider(m_statusBar);

    if (Store::config.behaviour.gayWindow)
    {
        FooBar *fooBar(new FooBar(this));
        fooBar->setVisible(true);
        m_tabBar = new TabBar(fooBar);
        fooBar->setTabBar(m_tabBar);
        Store::config.behaviour.hideTabBarWhenOnlyOneTab = false;
    }
    else
        m_tabBar = new TabBar(center);

    m_tabManager = new TabManager(this, m_tabWin);
    m_tabManager->setTabBar(m_tabBar);
    m_toolBar->setIconSize(QSize(16, 16));
    m_infoDock->setWidget(m_infoWidget);
    m_tabWin->setWindowFlags(0);
    m_placesDock->setWidget(m_placesView);
    m_placesDock->setObjectName(tr("Bookmarks"));
    m_recentDock->setWidget(m_recentFoldersView);
    m_recentDock->setObjectName(tr("Recent Folders"));
    m_infoDock->setObjectName(tr("Information"));

    addToolBar(m_toolBar);
    m_toolBar->setObjectName("viewToolBar");

    setWindowIcon(QIcon::fromTheme("folder", QIcon(":/trolltech/styles/commonstyle/images/diropen-128.png")));

    connect(m_placesView, SIGNAL(newTabRequest(QUrl)), this, SLOT(addTab(QUrl)));
    connect(m_placesView, SIGNAL(placeActivated(QUrl)), this, SLOT(activateUrl(QUrl)));
    connect(m_recentFoldersView, SIGNAL(recentFolderClicked(QUrl)), this, SLOT(activateUrl(QUrl)));
    connect(m_filterBox, SIGNAL(textChanged(QString)),this,SLOT(filterCurrentDir(QString)));

    connect(m_tabManager, SIGNAL(newTabRequest()), this, SLOT(newTab()));
    connect(m_tabManager, SIGNAL(currentTabChanged(int)), this, SLOT(currentTabChanged(int)));
    connect(m_tabManager, SIGNAL(tabCloseRequested(int)), this, SLOT(tabCloseRequest(int)));

    addActions(Store::customActions());
    createActions();
    createMenus();
    createToolBars();
    readSettings();
    setupStatusBar();

    if (Store::config.behaviour.gayWindow)
        menuBar()->hide();

    m_tabBar->setDocumentMode(true);
    m_tabWin->setCentralWidget(m_tabManager);

    bool needTab(true);
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
    if (autoTab && needTab)
    {
        addTab(QUrl::fromLocalFile(Store::config.startPath));
        int sort(0);
        Qt::SortOrder asc(Qt::AscendingOrder);
        Ops::getSorting(Store::config.startPath, sort, asc);
        sortingChanged(sort, (int)asc);
    }

    QVBoxLayout *vBox(new QVBoxLayout());
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
    toggleMenuVisible();

    m_actions[ShowPathBar]->setChecked(Store::settings()->value("pathVisible", true).toBool());
    hidePath();

    m_placesView->installEventFilter(this);
    if (activeContainer())
        activeContainer()->setFocus();
    m_statusBar->setVisible(m_actions[ShowStatusBar]->isChecked());
    m_toolBar->installEventFilter(this);
    QTimer::singleShot(0, this, SLOT(updateIcons()));
}

MainWindow::~MainWindow()
{
    s_openWindows.removeOne(this);
    if (s_currentWindow == this && !s_openWindows.isEmpty())
        s_currentWindow = 0;
}

void
MainWindow::receiveMessage(const QStringList &message)
{
    bool isSearchPar = false, isStatus = false, isIoProgress = false;
    QString statusMsg;
    foreach (const QString &msg, message)
        if (isSearchPar)
        {
            model()->search(msg);
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
                    m_ioProgress->setRange(0, 0);
                    m_ioProgress->setValue(0);
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
MainWindow::setRootPath(const QString &path)
{
    model()->setUrl(QUrl::fromLocalFile(path));
}

void
MainWindow::filterCurrentDir(const QString &filter)
{
    if (m_filterBox->mode() == Filter)
        activeContainer()->setFilter(filter);
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
    Docks::FloatButton *fl = new Docks::FloatButton(m_statusBar, Docks::Left);
    fl->setFixedSize(16, 16);
    m_statusBar->addLeftWidget(fl);
    fl->setFloating(m_placesDock->isVisible());
    Docks::FloatButton *fb = new Docks::FloatButton(m_statusBar, Docks::Right);
    fb->setFixedSize(16, 16);
    m_statusBar->addLeftWidget(fb);
    fb->setFloating(m_infoDock->isVisible());
    Docks::FloatButton *fr = new Docks::FloatButton(m_statusBar, Docks::Bottom);
    fr->setFixedSize(16, 16);
    m_statusBar->addLeftWidget(fr);
    fr->setFloating(m_recentDock->isVisible());

    connect (fl, SIGNAL(clicked()), m_placesDock, SLOT(toggleVisibility()));
    connect (fr, SIGNAL(clicked()), m_recentDock, SLOT(toggleVisibility()));
    connect (fb, SIGNAL(clicked()), m_infoDock, SLOT(toggleVisibility()));

    connect (fl, SIGNAL(rightClicked()), m_placesDock, SLOT(toggleLock()));
    connect (fr, SIGNAL(rightClicked()), m_recentDock, SLOT(toggleLock()));
    connect (fb, SIGNAL(rightClicked()), m_infoDock, SLOT(toggleLock()));

    connect (m_placesDock, SIGNAL(visibilityChanged(bool)), fl, SLOT(setFloating(bool)));
    connect (m_recentDock, SIGNAL(visibilityChanged(bool)), fr, SLOT(setFloating(bool)));
    connect (m_infoDock, SIGNAL(visibilityChanged(bool)), fb, SLOT(setFloating(bool)));

//    connect (m_iconSizeSlider, SIGNAL(sliderMoved(int)),this,SLOT(setViewIconSize(int)));
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
    m_iconSizeSlider->blockSignals(true);
    m_iconSizeSlider->setValue(size/16);
    m_iconSizeSlider->blockSignals(false);
}

void
MainWindow::setViewIconSize(int size)
{
    activeContainer()->setIconSize(size*16);
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
    const QModelIndexList &selected = activeContainer()->selectionModel()->selectedRows(0);
    if (selected.count() == 1)
        m_slctnMessage = QString(" :: \'%1\' Selected").arg(selected.first().data().toString());
    else if (selected.count() > 1)
        m_slctnMessage = QString(" :: %1 Items Selected").arg(QString::number(selected.count()));

    const QString &newMessage = selected.isEmpty() ? m_statusMessage : m_statusMessage + m_slctnMessage;
    m_statusBar->setMessage(newMessage);
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
    activeContainer()->setPathEditable(m_actions[EditPath]->isChecked());
}

void
MainWindow::setClipBoard()
{
    QModelIndexList selectedItems;
    if(activeContainer()->selectionModel()->selectedRows(0).count())
        selectedItems = activeContainer()->selectionModel()->selectedRows(0);
    else
        selectedItems = activeContainer()->selectionModel()->selectedIndexes();

    QApplication::clipboard()->setMimeData(model()->mimeData(selectedItems));
}

void
MainWindow::pasteSelection()
{
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    const QString &filePath = activeContainer()->currentView()->rootIndex().data(FS::FilePathRole).toString();

    if (QFileInfo(filePath).exists() && mimeData->hasUrls())
        IO::Manager::copy(mimeData->urls(), filePath, m_cut, m_cut);

    QApplication::clipboard()->clear();
    m_cut = false;
}

void
MainWindow::urlChanged(const QUrl &url)
{
    if (!sender())
        return;
    ViewContainer *c(static_cast<ViewContainer *>(sender()));
    const int tab(m_tabManager->indexOf(c));
    const QString &title = c->model()->title(url);
    if (c == activeContainer())
    {
        setWindowTitle(title);
        if (url.isLocalFile())
            m_placesView->activateAppropriatePlace(url.toLocalFile());
    }
    updateFilterBox();
    m_tabBar->setTabText(tab, title);
    m_tabBar->setTabIcon(tab, c->model()->index(url).data(FS::FileIconRole).value<QIcon>());
    setActions();
}

void
MainWindow::updateStatusBar(const QUrl &url)
{
    if (!model())
        return;
    m_statusMessage = Ops::getStatusBarMessage(url, model());
    m_statusBar->setMessage(m_statusMessage);
}

void
MainWindow::closeEvent(QCloseEvent *event)
{
    if (s_openWindows.count() == 1)  //this is last...
        writeSettings();
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
    QMainWindow::keyPressEvent(ke);
}

bool
MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == activeContainer() && event->type() == QEvent::HoverLeave)
    {
        const QString &newMessage = (activeContainer()->selectionModel()->selection().isEmpty() ||
                               activeContainer()->selectionModel()->currentIndex().parent() !=
                model()->index(model()->rootUrl())) ? m_statusMessage : m_statusMessage + m_slctnMessage;
        if (m_statusBar->currentMessage() != newMessage)
            m_statusBar->setMessage(newMessage);
    }
    else if (obj == m_placesView && (event->type() == QEvent::Resize || event->type() == QEvent::Show) && !m_placesDock->isFloating())
        updateToolbarSpacer();
//    else if (obj == m_toolBar && event->type() == QEvent::Resize && m_sortButton)
//    {
//        unsigned short area(m_toolBar->width()-m_sortButton->geometry().right());
//        m_filterBox->setFixedWidth(qMax(area>>1, ));
//    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::rename() { activeContainer()->rename(); }
void MainWindow::createDirectory() { activeContainer()->createDirectory(); }

void MainWindow::goHome() { model()->setUrl(QUrl::fromLocalFile(QDir::homePath())); }
void MainWindow::goBack() { activeContainer()->goBack(); }
void MainWindow::goUp() { activeContainer()->goUp(); }
void MainWindow::goForward() { activeContainer()->goForward(); }

void MainWindow::setViewIcons() { activeContainer()->setView(ViewContainer::Icon); }
void MainWindow::setViewDetails() { activeContainer()->setView(ViewContainer::Details); }
void MainWindow::setViewCols() { activeContainer()->setView(ViewContainer::Column); }
void MainWindow::flowView() { activeContainer()->setView(ViewContainer::Flow); }

void
MainWindow::genPlace()
{
    if (!model()->rootUrl().isLocalFile())
        return;
    const QFileInfo file(model()->rootUrl().toLocalFile());
    m_placesView->addPlace(file.fileName(), file.filePath(), FS::FileIconProvider::fileIcon(file));
}

void
MainWindow::updateFilterBox()
{
    if (!model() || !activeContainer() || !activeContainer()->currentView())
        return;
    m_filterBox->setMode(model()->isSearching() ? Search : Filter);
    m_filterBox->setText(model()->isSearching() ? model()->searchString() : model()->filter(activeContainer()->currentView()->rootIndex().data(FS::FilePathRole).toString()));
}

void
MainWindow::checkViewAct()
{
    m_actions[ViewContainer::viewAction(activeContainer()->currentViewType())]->setChecked(true);
//    m_iconSizeSlider->setVisible(activeContainer()->currentViewType() == ViewContainer::Icon);
    emit viewChanged(activeContainer()->currentView());
    updateFilterBox();
}

void
MainWindow::about()
{
    QMessageBox::about(this, tr("About dfm"),
                       tr("<b>dfm</b> is a file manager "
                          "that really is a file browser, "
                          "that can manage files."));
}

void MainWindow::deleteCurrentSelection() { activeContainer()->deleteCurrentSelection(); }

void MainWindow::toggleHidden() { model()->setHiddenVisible(m_actions[ShowHidden]->isChecked()); }

void MainWindow::refreshView() { activeContainer()->refresh(); }

void MainWindow::customCommand() { activeContainer()->customCommand(); }

void
MainWindow::toggleMenuVisible()
{
    if (sender() == m_actions[ShowToolBar])
    {
        m_toolBar->setVisible(m_actions[ShowToolBar]->isChecked());
    }
    else
    {
        menuBar()->setVisible(m_actions[ShowMenuBar]->isChecked());
        m_menuAction->setVisible(!m_actions[ShowMenuBar]->isChecked()&&!Store::config.behaviour.gayWindow);
        m_menuSep->setVisible(!m_actions[ShowMenuBar]->isChecked()&&!Store::config.behaviour.gayWindow);
    }
}

void MainWindow::toggleStatusVisible() { m_statusBar->setVisible(m_actions[ShowStatusBar]->isChecked()); }

void
MainWindow::connectContainer(ViewContainer *container)
{
    container->installEventFilter(this);
    connect(container, SIGNAL(urlChanged(QUrl)), this, SLOT(urlChanged(QUrl)));
    connect(container, SIGNAL(urlLoaded(QUrl)), this, SLOT(urlChanged(QUrl)));
    connect(container, SIGNAL(urlLoaded(QUrl)), this, SLOT(updateStatusBar(QUrl)));
    connect(container, SIGNAL(urlChanged(QUrl)), m_recentFoldersView, SLOT(folderEntered(QUrl)));
    connect(container, SIGNAL(viewChanged()), this, SLOT(checkViewAct()));
    connect(container, SIGNAL(selectionChanged()), this, SLOT(mainSelectionChanged()));
    connect(container, SIGNAL(iconSizeChanged(int)), this, SLOT(setSliderPos(int)));
    connect(container, SIGNAL(viewportEntered()), this, SLOT(viewClearHover()));
    connect(container, SIGNAL(leftView()), this, SLOT(viewClearHover()));
    connect(container, SIGNAL(newTabRequest(QUrl)), this, SLOT(addTab(QUrl)));
    connect(container, SIGNAL(entered(QModelIndex)), m_infoWidget, SLOT(hovered(QModelIndex)));
//    connect(container, SIGNAL(entered(QModelIndex)), this, SLOT(viewItemHovered(QModelIndex)));
    connect(container, SIGNAL(sortingChanged(int,int)), this, SLOT(sortingChanged(int,int)));
    connect(container, SIGNAL(hiddenVisibilityChanged(bool)), m_actions[ShowHidden], SLOT(setChecked(bool)));
    connect(this, SIGNAL(settingsChanged()), container, SLOT(loadSettings()));
}

void
MainWindow::addTab(const QUrl &url)
{
    ViewContainer *container = new ViewContainer(this);
    container->setIconSize(Store::config.views.iconView.iconSize*16);
    connectContainer(container);
    FS::Model *model = container->model();
    model->setUrl(url);
    const QString &title = model->title(url);
    const QModelIndex &idx = model->index(url);
    m_tabManager->addTab(container, idx.data(FS::FileIconRole).value<QIcon>(), title);
}

void
MainWindow::addTab(ViewContainer *container, int index)
{
    connectContainer(container);
    FS::Model *model = container->model();
    const QUrl &url = model->rootUrl();
    const QString &title = model->title(url);
    const QModelIndex &idx = model->index(url);
    m_tabManager->insertTab(index, container, idx.data(FS::FileIconRole).value<QIcon>(), title);
}

void
MainWindow::currentTabChanged(int tab)
{
    if (!m_tabManager->count())
        return;

    tab = qMax(tab, 0);
    DDataLoader::clearQueue();
    emit viewChanged(activeContainer()->currentView());
    sortingChanged(model()->sortColumn(), (int)model()->sortOrder());
    m_recentFoldersView->folderEntered(model()->rootUrl());
    setWindowTitle(m_tabBar->tabText(tab));
    activeContainer()->setPathEditable(m_actions[EditPath]->isChecked());
    m_iconSizeSlider->setValue(activeContainer()->iconSize().width()/16);
    m_iconSizeSlider->setToolTip(QString("Size: %1 px").arg(QString::number(activeContainer()->iconSize().width())));
    m_placesView->activateAppropriatePlace(model()->rootUrl().toLocalFile()); //TODO: placesview url based
    updateStatusBar(model()->rootUrl());
    m_actions[ShowHidden]->setChecked(model()->showHidden());
    setActions();
    activeContainer()->setFocus();
    updateFilterBox();
}

void
MainWindow::tabCloseRequest(int tab)
{
    m_tabManager->deleteTab(tab);
//    if (activeContainer())
//        activeContainer()->setFocus();
}

void
MainWindow::openTab()
{
    if (activeContainer() && activeContainer()->selectionModel()->hasSelection())
        foreach (const QModelIndex &index, activeContainer()->selectionModel()->selectedIndexes())
        {
            if (index.column() > 0)
                continue;
            const QFileInfo &file = model()->fileInfo(index);
            if (file.exists())
                addTab(QUrl::fromLocalFile(file.filePath()));
        }
}

void MainWindow::newTab() { addTab(model()->rootUrl()); }

ViewContainer
*MainWindow::takeContainer(int tab)
{
    ViewContainer *container = m_tabManager->takeTab(tab);
    if (!container)
        return 0;

    container->removeEventFilter(this);

    container->disconnect(this);
    disconnect(container);

    container->disconnect(m_infoWidget);
    container->disconnect(m_actions[ShowHidden]);

    foreach (QAction *a, container->actions())
        container->removeAction(a);

    container->setParent(0);
    container->close();

    if (!m_tabManager->count())
    {
        close();
        deleteLater();
    }
    return container;
}

ViewContainer *MainWindow::activeContainer() { return static_cast<ViewContainer *>(m_tabManager->currentWidget()); }
ViewContainer *MainWindow::containerForTab(int tab)  { return static_cast<ViewContainer *>(m_tabManager->widget(tab)); }

void
MainWindow::setActions()
{
    m_actions[GoBack]->setEnabled(activeContainer()->canGoBack());
    m_actions[GoForward]->setEnabled(activeContainer()->canGoForward());
    checkViewAct();
}

void
MainWindow::viewItemHovered(const QModelIndex &index)
{
    if (!model() || model()->isWorking() || !index.isValid() || index.row() > 100000 || index.column() > 3)
        return;
    const QFileInfo &file = model()->fileInfo(index);
    if (!file.exists())
        return;
    QString message = file.filePath();
    if (file.isSymLink())
        message.append(QString(" -> %1").arg(file.symLinkTarget()));
    m_statusBar->setMessage(message);
}

void
MainWindow::viewClearHover()
{
    if (!activeContainer())
        return;
    QString newMessage = (activeContainer()->selectionModel()->selection().isEmpty()
                           ||  activeContainer()->selectionModel()->currentIndex().parent() != model()->index(model()->rootUrl()))
            ? m_statusMessage : m_statusMessage + m_slctnMessage;
    if (m_statusBar->currentMessage() != newMessage)
        m_statusBar->setMessage(newMessage);
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
    foreach (const QModelIndex &index, activeContainer()->selectionModel()->selectedIndexes())
    {
        if (index.column() != 0)
            continue;
        const QFileInfo &fi = model()->fileInfo(index);
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
    Store::config.behaviour.sortingCol = column;
    Store::config.behaviour.sortingOrd = (Qt::SortOrder)order;

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

    activeContainer()->sort(i, (Qt::SortOrder)m_actions[SortDescending]->isChecked());
}

void
MainWindow::showEvent(QShowEvent *e)
{
    QMainWindow::showEvent(e);
//    int w = m_placesView->viewport()->width();
//    if (QWidget *b = m_toolBar->widgetForAction(m_actions[GoBack]))
//        w -= b->width();
//    if (QWidget *b = m_toolBar->widgetForAction(m_actions[GoForward]))
//        w -= b->width();
//    w -= style()->pixelMetric(QStyle::PM_ToolBarSeparatorExtent);
//    if (m_toolBarSpacer->width() != w)
//    {
//        const int width = qMax(0, w + style()->pixelMetric(QStyle::PM_DockWidgetSeparatorExtent)/2);
//        m_toolBarSpacer->setFixedWidth(width);
//    }
}

void
MainWindow::windowActivationChange(bool wasActive)
{
    if (s_currentWindow)
        disconnect(dApp, SIGNAL(lastMessage(QStringList)), s_currentWindow, SLOT(receiveMessage(QStringList)));
    if (!wasActive)
    {
        s_currentWindow = this;
        if (activeContainer())
            emit viewChanged(activeContainer()->currentView());
    }
    if (s_currentWindow)
        connect(dApp, SIGNAL(lastMessage(QStringList)), s_currentWindow, SLOT(receiveMessage(QStringList)));
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

    m_tabWin->addDockWidget(Qt::LeftDockWidgetArea, m_placesDock);
    m_tabWin->addDockWidget(Qt::RightDockWidgetArea, m_recentDock);
    m_tabWin->addDockWidget(Qt::BottomDockWidgetArea, m_infoDock);

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
    int w = m_placesView->width();
//    if (QWidget *b = m_toolBar->widgetForAction(m_actions[GoBack]))
//        w -= b->geometry().right();
    if (QWidget *b = m_toolBar->widgetForAction(m_actions[GoForward]))
        w -= b->geometry().right();
//    w -= style()->pixelMetric(QStyle::PM_ToolBarSeparatorExtent);
    if (m_toolBarSpacer->width() != w)
    {
        w = qMax(0, w + style()->pixelMetric(QStyle::PM_DockWidgetSeparatorExtent)/2);
        m_toolBarSpacer->setFixedWidth(w);
    }
    QTimer::singleShot(0, m_toolBar, SLOT(update()));
}

void
MainWindow::updateIcons()
{
    QColor fg = m_toolBar->palette().color(m_toolBar->foregroundRole());
    if (QWidget *firstTB = m_toolBar->widgetForAction(m_actions[GoBack]))
    {
        QPixmap pix(firstTB->size());
        pix.fill(Qt::transparent);
        firstTB->render(&pix);
        QImage img = pix.toImage();
        QRgb *rgbPx = reinterpret_cast<QRgb *>(img.bits());
        const int size = img.width()*img.height();
        unsigned int value = 0;
        for (int i=0; i<size; ++i)
            value += QColor(rgbPx[i]).value();


        if (qAbs((value/size)-fg.value()) < 127) //not enough contrast...
        {
            qDebug() << "not enough contrast... calculating new icons color for toolbuttons";
            //fg.setHsv(fg.hue(), fg.saturation(), 255-fg.value());
            fg = m_toolBar->palette().color(m_toolBar->backgroundRole());
        }
    }


#define SETICON(_ICON_) setIcon(IconProvider::icon(_ICON_, m_toolBar->iconSize().height(), fg, Store::config.behaviour.systemIcons))
    m_actions[GoBack]->SETICON(IconProvider::GoBack);
    m_actions[GoForward]->SETICON(IconProvider::GoForward);
    m_actions[Views_Icon]->SETICON(IconProvider::IconView);
    m_actions[Views_Detail]->SETICON(IconProvider::DetailsView);
    m_actions[Views_Column]->SETICON(IconProvider::ColumnsView);
    m_actions[Views_Flow]->SETICON(IconProvider::FlowView);
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
    QTimer::singleShot(0, this, SLOT(updateToolbarSpacer()));
}

void
MainWindow::updateConfig()
{
    m_tabBar->setVisible(m_tabBar->count() > 1 || !Store::config.behaviour.hideTabBarWhenOnlyOneTab);
    if (activeContainer())
        activeContainer()->refresh();

    m_placesView->viewport()->update();
    emit settingsChanged();
    QTimer::singleShot(0, this, SLOT(updateIcons()));
}

void
MainWindow::newWindow()
{
    const QFileInfo &fi = model()->rootUrl().toLocalFile();
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
    if (model())
    {
        Store::config.behaviour.sortingCol = model()->sortColumn();
        Store::config.behaviour.sortingOrd = model()->sortOrder();
    }
    Store::writeConfig();
    m_placesView->store();
}

void
MainWindow::activateUrl(const QUrl &url)
{
    if (!activeContainer())
        return;
    if (!activeContainer()->model())
        return;
    activeContainer()->model()->setUrl(url);
}

void
MainWindow::trash()
{
    if (sender())
    if (ViewContainer *vc = activeContainer())
    if (vc->selectionModel()->hasSelection())
    {
        QStringList files;
        const QModelIndexList indexes = vc->selectionModel()->selectedIndexes();
        for (int i = 0; i < indexes.count(); ++i)
        if (!indexes.at(i).column())
            files << indexes.at(i).data(FS::FilePathRole).toString();
        if (sender() == m_actions[MoveToTrashAction])
            DTrash::moveToTrash(files);
        else if (sender() == m_actions[RestoreFromTrashAction])
            DTrash::restoreFromTrash(files);
        else if (sender() == m_actions[RemoveFromTrashAction])
            DTrash::removeFromTrash(files);
    }
}

MainWindow *MainWindow::currentWindow() { return s_currentWindow; }
QList<MainWindow *> MainWindow::openWindows() { return s_openWindows; }
ViewContainer *MainWindow::currentContainer() { return s_currentWindow->activeContainer(); }
PlacesView *MainWindow::places() { return s_currentWindow->placesView(); }
FS::Model *MainWindow::model() { return activeContainer()->model(); }

