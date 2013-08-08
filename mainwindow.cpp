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
#include "iojob.h"
#include "settingsdialog.h"
#include "propertiesdialog.h"
#include "application.h"
#include "thumbsloader.h"
#include <QToolTip>
#include <QClipboard>
#include <QMenuBar>
#include <QMessageBox>

using namespace DFM;
Config MainWindow::config;

MainWindow::MainWindow(QStringList arguments)
{ 
#ifdef Q_WS_X11
    m_settings = new QSettings("dfm", "dfm"); //folder and conf file name in .config
    APP->setMainWindow(this);
#else
    settings = new QSettings(QDir::homePath() + QDir::separator() + "Documents/dfm.ini",QSettings::IniFormat);
#endif
    readConfig();
//    qDebug() << ThumbsLoader::instance();
//    if ( config.views.showThumbs )
//        connect ( this, SIGNAL(viewChanged(QAbstractItemView*)), ThumbsLoader::instance(), SLOT(setCurrentView(QAbstractItemView*)) );
    QWidget *center = new QWidget(this);
    m_tabWin = new QMainWindow(this);
    m_navToolBar = new QToolBar(tr("Show NavBar"), this);
    m_viewToolBar = new QToolBar(tr("Show ViewBar"), this);
    m_statusBar = statusBar();
    m_filterBox = new SearchBox(m_viewToolBar);
    m_toolBarSpacer = new QWidget(m_viewToolBar);
    m_toolBarSpacer->setFixedHeight(1);
    m_dockLeft = new Docks::DockWidget(m_tabWin, tr("Bookmarks"), Qt::SubWindow, Docks::Left);
    m_dockRight = new Docks::DockWidget(m_tabWin, tr("Recent Folders"), Qt::SubWindow, Docks::Right);
    m_dockBottom = new Docks::DockWidget(m_tabWin, tr("Information"), Qt::SubWindow, Docks::Bottom);
    m_placesView = new PlacesView(m_dockLeft);
    m_infoWidget = new InfoWidget(m_dockBottom);
    m_recentFoldersView = new RecentFoldersView(m_dockRight);
    m_iconSizeSlider = new QSlider(m_statusBar);
    m_tabBar = new TabBar(center);
    m_stackedWidget = new QStackedWidget(this);
    m_activeContainer = 0;
    m_fsModel = 0;
    int is = 16;
    QSize tbs(QSize(is, is));
    m_navToolBar->setIconSize(tbs);
    m_viewToolBar->setIconSize(tbs);
    m_dockBottom->setWidget(m_infoWidget);

    m_tabWin->setWindowFlags(0);
    m_dockLeft->setWidget(m_placesView);
    m_dockLeft->setObjectName(tr("Bookmarks"));
    m_dockRight->setWidget(m_recentFoldersView);
    m_dockRight->setObjectName(tr("Recent Folders"));
    m_dockBottom->setObjectName(tr("Information"));

    m_cut = false;

    addToolBar(m_navToolBar);
    addToolBar(m_viewToolBar);
    m_navToolBar->setObjectName("navToolBar");
    m_viewToolBar->setObjectName("viewToolBar");

    setWindowIcon(QIcon::fromTheme("folder", QIcon(":/trolltech/styles/commonstyle/images/diropen-128.png")));

    connect( m_placesView, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(rootFromPlaces(QTreeWidgetItem*,int)));
    connect( m_placesView, SIGNAL(itemActivated(QTreeWidgetItem*,int)), this, SLOT(rootFromPlaces(QTreeWidgetItem*,int)));
    connect( m_filterBox, SIGNAL(textChanged(QString)),this,SLOT(filterCurrentDir(QString)));
    connect( m_tabBar, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
    connect( m_tabBar, SIGNAL(newTabRequest()), this, SLOT(newTab()));
    connect( m_placesView, SIGNAL(newTabRequest(QString)), this, SLOT(addTab(QString)));
    connect( m_tabBar,SIGNAL(tabCloseRequested(int)), this, SLOT(tabClosed(int)));
    connect( m_tabBar, SIGNAL(tabMoved(int,int)), this, SLOT(tabMoved(int,int)));
    connect( m_stackedWidget, SIGNAL(currentChanged(int)), this, SLOT(stackChanged(int)));
    connect( m_recentFoldersView, SIGNAL(recentFolderClicked(QString)), this, SLOT(activateRecentFolder(QString)));

    createActions();
    readSettings();
    createMenus();
    createToolBars();
    createSlider();

    m_tabBar->setDocumentMode(true);
    m_tabWin->setCentralWidget(m_stackedWidget);

    QString startPath = config.startPath;
    m_appPath = arguments[0];
    if(arguments.count() > 1 && QFileInfo(arguments.at(1)).isDir())
        startPath = arguments.at(1);
    addTab(startPath);

    QVBoxLayout *vBox = new QVBoxLayout();
    vBox->setMargin(0);
    vBox->setSpacing(0);
    vBox->addWidget(m_tabBar);
    vBox->addWidget(m_tabWin);
    center->setLayout(vBox);

    m_placesView->addActions(QList<QAction *>() << m_placeAct << m_renPlaceAct << m_rmPlaceAct << m_placeContAct << m_placeIconAct );

    setCentralWidget(center);

    setUnifiedTitleAndToolBarOnMac(true);

    setStatusBar(m_statusBar);
    m_statusBar->installEventFilter(this);
    toggleMenuVisible();

    m_pathVisibleAct->setChecked(m_settings->value("pathVisible", true).toBool());
    hidePath();

    m_placesView->installEventFilter(this);
    m_activeContainer->setFocus();
    m_statusBar->setVisible(m_statAct->isChecked());
}

void
MainWindow::filterCurrentDir(QString filter)
{
#if 0
    QString f = filter;
    f.replace(" ","*");
    viewContainer->model()->setNameFilters(viewContainer->model()->rootDirectory().nameFiltersFromString("*" + f + "*"));
#endif
    m_activeContainer->setFilter(filter);  //temporary solution
}

void
MainWindow::createSlider()
{
    m_iconSizeSlider->setFixedWidth(80);
    m_iconSizeSlider->setOrientation(Qt::Horizontal);
    m_iconSizeSlider->setRange(1,16);
    m_iconSizeSlider->setSingleStep(1);
    m_iconSizeSlider->setPageStep(1);
    QHBoxLayout *l = qobject_cast<QHBoxLayout*>(m_statusBar->layout());
    l->insertWidget(l->count()-1, m_iconSizeSlider);
    Docks::FloatButton *fl = new Docks::FloatButton(m_statusBar);
    fl->setFixedSize(16, 16);
    l->insertWidget(0, fl);
    fl->setFloating(m_dockLeft->isVisible());
    Docks::FloatButton *fb = new Docks::FloatButton(m_statusBar, Docks::Bottom);
    fb->setFixedSize(16, 16);
    l->insertWidget(1, fb);
    fb->setFloating(m_dockBottom->isVisible());
    Docks::FloatButton *fr = new Docks::FloatButton(m_statusBar, Docks::Right);
    fr->setFixedSize(16, 16);
    l->insertWidget(2, fr);
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
    connect ( m_iconSizeSlider, SIGNAL(valueChanged(int)),this,SLOT(setViewIconSize(int)));
}

void
MainWindow::setSliderPos(int size)
{
    m_iconSizeSlider->setValue(size/16);
}

void
MainWindow::setViewIconSize(int size)
{
//    activeCont->setIconSize(size);
    m_activeContainer->animateIconSize(m_activeContainer->iconSize().width(),size*16);
    m_iconSizeSlider->setToolTip("Size: " + QString::number(size*16) + " px" );
    QToolTip *tip;
    QPoint pt;
    pt.setX(mapToGlobal(m_iconSizeSlider->pos()).x());
    pt.setY(mapToGlobal(m_statusBar->pos()).y());
    if(m_statusBar->isVisible())
        tip->showText(pt,m_iconSizeSlider->toolTip());
}


void
MainWindow::mainSelectionChanged(QItemSelection selected,QItemSelection notselected)
{
    QModelIndexList selectedItems;
    if(m_activeContainer->selectionModel()->selectedRows(0).count())
        selectedItems = m_activeContainer->selectionModel()->selectedRows(0);
    else
        selectedItems = m_activeContainer->selectionModel()->selectedIndexes();

    QString selectedItem;
    if(selectedItems.count())
        selectedItem = m_fsModel->fileName(selectedItems.at(0));

    if(selectedItems.count() == 1)
    {
        slctnMessage = " ::  " + QString::fromAscii("\'") + selectedItem + QString::fromAscii("\'") + " Selected";

    }
    else if(selectedItems.count() > 1)
        slctnMessage =  " ::  " + QString::number(selectedItems.count()) + " Items Selected";

    QString newMessage = m_activeContainer->selectionModel()->selection().isEmpty() ? statusMessage : statusMessage + slctnMessage;
    m_statusBar->showMessage(newMessage);
}

void
MainWindow::rename()
{
    m_activeContainer->rename();
}

void
MainWindow::createDirectory()
{
    m_activeContainer->createDirectory();
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
    m_activeContainer->setPathEditable(m_pathEditAct->isChecked());
}

void
MainWindow::setClipBoard()
{
    QModelIndexList selectedItems;
    if(m_activeContainer->selectionModel()->selectedRows(0).count())
        selectedItems = m_activeContainer->selectionModel()->selectedRows(0);
    else
        selectedItems = m_activeContainer->selectionModel()->selectedIndexes();

    QApplication::clipboard()->setMimeData(m_fsModel->mimeData(selectedItems));
}

void
MainWindow::pasteSelection()
{
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();

    if(!mimeData->hasUrls())
        return;

    QStringList copyFiles;
    foreach(QUrl url,QList<QUrl>(mimeData->urls()))
        copyFiles << url.toLocalFile();

    IO::Job::copy(copyFiles, m_fsModel->rootPath(), m_cut);

    QApplication::clipboard()->clear();

    m_cut = false;
}

void
MainWindow::rootFromPlaces(QTreeWidgetItem* item,int col)
{
    if(item->parent())
        m_fsModel->setRootPath(item->text(1));
}

void
MainWindow::rootPathChanged(QString index)
{
    setWindowTitle(m_fsModel->rootDirectory().dirName());
    m_filterBox->clear();
    m_activeContainer->setFilter("");
    m_placesView->activateAppropriatePlace(index);
    m_tabBar->setTabText(m_tabBar->currentIndex(),m_fsModel->rootDirectory().dirName());
}

void
MainWindow::directoryLoaded(QString index)
{
    updateStatusBar(index);
    setActions();
}

void
MainWindow::updateStatusBar(QString index)
{
    QDir crnt = m_fsModel->rootDirectory();

    crnt.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    int folders = crnt.count();

    crnt.setFilter(QDir::Dirs | QDir::NoDotAndDotDot | QDir::AllEntries);
    int allfiles = crnt.count();
    int files = allfiles - folders;

    qreal size = 0;
    foreach(QString entry, crnt.entryList())
    {
        QFileInfo *file = new QFileInfo(index + "/" + entry);
        if(!file->isDir())
            size += file->size();
    }
    int sizeType = 0;
    while(size > 1024)
    {
        size = size/1024;
        sizeType++;
    }
    QString statSize;
    switch (sizeType)
    {
    case 0 : statSize = " Byte"; break;
    case 1 : statSize = " KiB"; break;
    case 2 : statSize = " MiB"; break;
    case 3 : statSize = " GiB"; break;
    default: statSize = " TiB"; //I doubt if we need any higher then this...
    }

    QString sz = QString::number(size);

    QStringList realSize = sz.split(".");
    sz = realSize.at(0);
    QString szEnd;
    if(realSize.count() == 2)
        szEnd = realSize.at(1);
    QString end;
    if(szEnd.count() >= 3)
    {
        QString endSize = szEnd;
        endSize.truncate(3);
        endSize.remove(0,2);
        bool ok = true;
        int endEnd = endSize.toInt(&ok,10);
        QString startSize = szEnd;
        startSize.truncate(2);
        int endStart = startSize.toInt(&ok,10);
        if(endEnd >= 5)
            endStart++;
        end = QString::number(endStart);
    }
    else if(szEnd.count() == 1)
        end = szEnd + "0";
    else if(szEnd.count() == 2)
        end = szEnd;

    if(!end.isEmpty())
        sz = sz + "." + end;


    QString messaage;
    QString _folder = folders > 1 ? " Folders" : " Folder";
    QString _file = files > 1 ? " Files " : " File ";

    if(folders >= 1 && files >= 1)
        messaage = QString::number(folders) + _folder + ", " + QString::number(files) + _file + "(" + sz + statSize + ")";
    else if(folders >= 1 && files == 0)
        messaage = QString::number(folders) + _folder;
    else if(folders == 0 && files >= 1)
        messaage = QString::number(files) + _file + sz + " " + statSize;

    statusMessage = messaage;
    m_statusBar->showMessage(messaage);
}

void
MainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();
    APP->setMainWindow(0);
    event->accept();
}

void
MainWindow::keyPressEvent(QKeyEvent *ke)
{
    if ( ke->key() == Qt::Key_I && (ke->modifiers() & Qt::CTRL) )
        m_filterBox->setFocus();
    if (focusWidget() == m_filterBox && ke->key() == Qt::Key_Escape)
    {
        m_filterBox->clear();
        m_filterBox->clearFocus();
        filterCurrentDir("");
    }
//    if( ke->key() == Qt::Key_Return)
//        m_activeContainer->doubleClick(m_activeContainer->selectionModel()->selectedIndexes().first());
    QMainWindow::keyPressEvent(ke);
}

bool
MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_activeContainer && event->type() == QEvent::HoverLeave )
    {
        QString newMessage = ( m_activeContainer->selectionModel()->selection().isEmpty() ||
                               m_activeContainer->selectionModel()->currentIndex().parent() !=
                m_fsModel->index(m_fsModel->rootPath())) ? statusMessage : statusMessage + slctnMessage;
        if(m_statusBar->currentMessage() != newMessage)
            m_statusBar->showMessage(newMessage);
        return false;
    }

    if (obj == m_placesView && (event->type() == QEvent::Resize || event->type() == QEvent::Show) && !m_dockLeft->isFloating())
        if (m_toolBarSpacer->width() != m_placesView->viewport()->width()-(m_navToolBar->geometry().width() * m_navToolBar->isVisible()) )
        {
            const int &width = qMax(0, m_placesView->viewport()->width()-(m_navToolBar->geometry().width() * m_navToolBar->isVisible()) + style()->pixelMetric(QStyle::PM_DockWidgetSeparatorExtent)/2);
            m_toolBarSpacer->setFixedWidth(width);
        }

    if (obj == m_statusBar && event->type() == QEvent::Paint)
    {
        QPainter p(m_statusBar);
        p.setPen(m_statusBar->palette().color(m_statusBar->foregroundRole()));
        p.drawText(m_statusBar->rect(), Qt::AlignCenter, m_statusBar->currentMessage());
        return true;
    }
    return QMainWindow::eventFilter(obj, event);
}

void
MainWindow::goHome()
{  
    m_fsModel->setRootPath(QDir::homePath());
}

void
MainWindow::goBack()
{
    m_activeContainer->goBack();
}

void
MainWindow::goUp()
{
    m_activeContainer->goUp();
}

void
MainWindow::goForward()
{  
    m_activeContainer->goForward();
}

void
MainWindow::genPlace()
{
    QFileInfo file(m_fsModel->rootPath());
    QString path = file.filePath();
    QString name = file.fileName();
    QIcon icon = QIcon::fromTheme("folder");
    m_placesView->addPlace(name, path, icon);
}

void
MainWindow::setViewIcons()
{
    m_activeContainer->setView(ViewContainer::Icon);
}

void
MainWindow::setViewDetails()
{
    m_activeContainer->setView(ViewContainer::Details);
}

void
MainWindow::setViewCols()
{
    m_activeContainer->setView(ViewContainer::Columns);
}

void
MainWindow::flowView()
{
    m_activeContainer->setView(ViewContainer::Flow);
}

void
MainWindow::checkViewAct()
{
    m_iconViewAct->setChecked(m_activeContainer->currentViewType() == ViewContainer::Icon);
    m_listViewAct->setChecked(m_activeContainer->currentViewType() == ViewContainer::Details);
    m_colViewAct->setChecked(m_activeContainer->currentViewType() == ViewContainer::Columns);
    m_flowAct->setChecked(m_activeContainer->currentViewType() == ViewContainer::Flow);
    m_iconSizeSlider->setVisible(m_activeContainer->currentViewType() == ViewContainer::Icon);
    emit viewChanged( m_activeContainer->currentView() );
}

void
MainWindow::about()
{
    QMessageBox::about(this, tr("About dfm"),
                       tr("<b>dfm</b> is a file manager "
                          "that really is a file browser, "
                          "that can manage files."));
}

void
MainWindow::deleteCurrentSelection()
{
    m_activeContainer->deleteCurrentSelection();
}

void
MainWindow::toggleHidden()
{
    if(m_showHiddenAct->isChecked())
        m_fsModel->setFilter(QDir::NoDotAndDotDot | QDir::AllEntries | QDir::System | QDir::Hidden);
    else
        m_fsModel->setFilter(QDir::NoDotAndDotDot | QDir::AllEntries | QDir::System);
}


void
MainWindow::refreshView()
{
    m_activeContainer->refresh();
}

void
MainWindow::customCommand()
{
    m_activeContainer->customCommand();
}

void
MainWindow::toggleMenuVisible()
{
    menuBar()->setVisible(m_menuAct->isChecked());
}

void
MainWindow::toggleStatusVisible()
{
    m_statusBar->setVisible(m_statAct->isChecked());
}

void
MainWindow::addTab(const QString &path)
{
    QString newPath = path;
    if (!QFileInfo(path).isDir())
        newPath = QDir::homePath();
    ViewContainer *container = new ViewContainer(this, newPath);
    container->installEventFilter(this);
    connect( container, SIGNAL(currentPathChanged(QString)), this, SLOT(rootPathChanged(QString)));
    connect( container, SIGNAL(currentPathChanged(QString)), m_recentFoldersView, SLOT(folderEntered(QString)));
    connect( container, SIGNAL(viewChanged()), this, SLOT(checkViewAct()));
    connect( container->model(), SIGNAL(directoryLoaded(QString)), this, SLOT(directoryLoaded(QString)));
    connect( container->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(mainSelectionChanged(QItemSelection,QItemSelection)));
    connect( container, SIGNAL(iconSizeChanged(int)), this, SLOT(setSliderPos(int)));
    connect( container, SIGNAL(viewportEntered()), this, SLOT(viewClearHover()));
    connect( container, SIGNAL(leftView()), this, SLOT(viewClearHover()));
    connect( container, SIGNAL(newTabRequest(QString)), this, SLOT(addTab(QString)));
    connect( container, SIGNAL(entered(QModelIndex)), m_infoWidget, SLOT(hovered(QModelIndex)));
    connect( container, SIGNAL(entered(QModelIndex)), this, SLOT(viewItemHovered(QModelIndex)));

    QList<QAction *> actList;
    actList << m_openInTabAct << m_mkDirAct << m_pasteAct << m_copyAct << m_cutAct
            << m_delCurrentSelectionAct << m_renameAct << m_cstCmdAct << m_goUpAct
            << m_goBackAct << m_goForwardAct << m_propertiesAct;

    container->addActions(actList);
    m_stackedWidget->addWidget(container);
    m_tabBar->addTab(QFileInfo(newPath).fileName());

    if (config.behaviour.hideTabBarWhenOnlyOneTab)
        m_tabBar->setVisible(m_tabBar->count() > 1);
}

void
MainWindow::tabChanged(int currentIndex)
{
    if ( currentIndex < 0 )
        return;
    m_stackedWidget->setCurrentIndex(currentIndex);
    m_activeContainer = static_cast<ViewContainer*>(m_stackedWidget->currentWidget());
    emit viewChanged( m_activeContainer->currentView() );
    m_fsModel = m_activeContainer->model();
    m_recentFoldersView->folderEntered(m_fsModel->rootPath());
    setWindowTitle(m_tabBar->tabText(currentIndex));
    m_activeContainer->setPathEditable(m_pathEditAct->isChecked());
    m_iconSizeSlider->setValue(m_activeContainer->iconSize().width()/16);
    m_iconSizeSlider->setToolTip("Size: " + QString::number(m_activeContainer->iconSize().width()) + " px");
    m_placesView->activateAppropriatePlace(m_fsModel->rootPath());
    updateStatusBar(m_fsModel->rootPath());
    m_filterBox->setText(m_activeContainer->currentFilter());
    setActions();
}

void
MainWindow::tabMoved(int from, int to)
{
    QWidget *w = m_stackedWidget->widget(from);
    m_stackedWidget->removeWidget(w);
    m_stackedWidget->insertWidget(to, w);
}

void
MainWindow::tabClosed(int tab)
{
    QWidget *removedWidget = m_stackedWidget->widget(tab);
    m_stackedWidget->removeWidget(removedWidget); //somewhy it's important to first remove from stack, then tabbar
    m_tabBar->removeTab(tab);
    removedWidget->deleteLater();
    m_activeContainer->setFocus();
    if (config.behaviour.hideTabBarWhenOnlyOneTab)
        m_tabBar->setVisible(m_tabBar->count() > 1);
}

void
MainWindow::openTab()
{
    addTab(m_fsModel->filePath(m_activeContainer->selectionModel()->selectedIndexes().first()));
}

void
MainWindow::newTab()
{
    QString rootPath;
    rootPath = m_fsModel->rootPath();
    addTab(rootPath);
}

void
MainWindow::setActions()
{
    m_goBackAct->setEnabled(m_activeContainer->canGoBack());
    m_goForwardAct->setEnabled(m_activeContainer->canGoForward());
//    m_pathVisibleAct->setChecked(m_activeContainer);
    checkViewAct();
}

void
MainWindow::viewItemHovered( const QModelIndex &index )
{
    const QString &file = m_fsModel->filePath( m_fsModel->index( m_fsModel->filePath(index) ) );
    m_statusBar->showMessage( file );
}

void
MainWindow::viewClearHover()
{
    QString newMessage = ( m_activeContainer->selectionModel()->selection().isEmpty() ||  m_activeContainer->selectionModel()->currentIndex().parent() != m_fsModel->index(m_fsModel->rootPath())) ? statusMessage : statusMessage + slctnMessage;
    if(m_statusBar->currentMessage() != newMessage)
        m_statusBar->showMessage(newMessage);
}

void
MainWindow::showSettings()
{
    static SettingsDialog *settingsDialog = 0;
    if ( !settingsDialog )
    {
        settingsDialog = new SettingsDialog(this);
        connect( settingsDialog, SIGNAL(settingsChanged()), this, SLOT(updateConfig()) );
    }
    settingsDialog->exec();
}

void
MainWindow::fileProperties()
{
    QModelIndex fileIndex = m_activeContainer->selectionModel()->currentIndex();
    QString file = m_fsModel->filePath(fileIndex);
    static PropertiesDialog *propertiesDialog = 0;
    if (!propertiesDialog)
        propertiesDialog = new PropertiesDialog(this);
    propertiesDialog->setFile(file);
    propertiesDialog->show();
}

void
MainWindow::hidePath()
{
    foreach ( BreadCrumbs *bc, findChildren<BreadCrumbs *>() )
        bc->setVisible(m_pathVisibleAct->isChecked());
    m_settings->setValue("pathVisible", m_pathVisibleAct->isChecked());
}

void
MainWindow::stackChanged(int)
{
    if (m_activeContainer)
        m_filterBox->setText(m_activeContainer->currentFilter());
}

void
MainWindow::moveEvent(QMoveEvent *)
{
//    qDebug() << "moveevent" << pos();
}

bool
MainWindow::x11Event(XEvent *xe)
{
//    qDebug() << xe->type;
    return QMainWindow::x11Event(xe);
}
