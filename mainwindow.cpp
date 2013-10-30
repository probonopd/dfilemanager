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

using namespace DFM;

static MainWindow *s_currentWindow = 0;
static QList<MainWindow *> s_openWindows;

MainWindow::MainWindow(QStringList arguments)
    : QMainWindow()
    , m_sortButton(0)
{ 
    addActions(Store::customActions());
    s_currentWindow = this;
    s_openWindows << this;
    QFrame *center = new QFrame(this);
    center->setFrameStyle(0/*QFrame::StyledPanel|QFrame::Sunken*/);
    m_tabWin = new QMainWindow(this);
    m_toolBar = new QToolBar(tr("Show ToolBar"), this);
    m_statusBar = statusBar();
    m_filterBox = new SearchBox(m_toolBar);
    m_toolBarSpacer = new QWidget(m_toolBar);
    m_dockLeft = new Docks::DockWidget(m_tabWin, tr("Bookmarks"), Qt::SubWindow, Docks::Left);
    m_dockRight = new Docks::DockWidget(m_tabWin, tr("Recent Folders"), Qt::SubWindow, Docks::Right);
    m_dockBottom = new Docks::DockWidget(m_tabWin, tr("Information"), Qt::SubWindow, Docks::Bottom);
    m_placesView = new PlacesView(m_dockLeft);
    m_infoWidget = new InfoWidget(m_dockBottom);
    m_recentFoldersView = new RecentFoldersView(m_dockRight);
    m_iconSizeSlider = new QSlider(m_statusBar);

    if ( Store::config.behaviour.gayWindow )
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

//    connect( m_placesView, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(rootFromPlaces(QTreeWidgetItem*,int)));
//    connect( m_placesView, SIGNAL(itemActivated(QTreeWidgetItem*,int)), this, SLOT(rootFromPlaces(QTreeWidgetItem*,int)));
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

    if ( Store::config.behaviour.gayWindow )
        menuBar()->hide();

    m_tabBar->setDocumentMode(true);
    m_tabWin->setCentralWidget(m_stackedWidget);

    QString startPath = Store::config.startPath;
    if ( !arguments.isEmpty() )
    {
        m_appPath = arguments[0];
        if ( QFileInfo(arguments.at(0)).isDir() )
            startPath = arguments.at(0);
        else if (arguments.count() > 1 && QFileInfo(arguments.at(1)).isDir())
            startPath = arguments.at(1);
    }
    addTab(startPath);

    QVBoxLayout *vBox = new QVBoxLayout();
    vBox->setMargin(0);
    vBox->setSpacing(0);
    if ( !Store::config.behaviour.gayWindow )
        vBox->addWidget(m_tabBar);
    vBox->addWidget(m_tabWin);
    center->setLayout(vBox);

    m_placesView->addActions(QList<QAction *>() << m_placeAct << m_renPlaceAct << m_rmPlaceAct << m_placeContAct << m_placeIconAct );

    setCentralWidget(center);

    setUnifiedTitleAndToolBarOnMac(true);

    setStatusBar(m_statusBar);
    m_statusBar->installEventFilter(this);
    toggleMenuVisible();

    m_pathVisibleAct->setChecked(Store::settings()->value("pathVisible", true).toBool());
    hidePath();

    m_placesView->installEventFilter(this);
    m_activeContainer->setFocus();
    m_statusBar->setVisible(m_statAct->isChecked());
    QTimer::singleShot(200, m_toolBar, SLOT(update())); //should be superfluos but make sure toolbar is painted correctly....
}

void
MainWindow::receiveMessage(const QStringList &message)
{
    setWindowState(windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
    foreach ( const QString &msg, message )
        if ( QFileInfo(msg).isDir() )
        {
            addTab(msg);
            m_tabBar->setCurrentIndex(m_tabBar->count()-1);
        }
}

void
MainWindow::filterCurrentDir(const QString &filter)
{
#if 0 //for some reason this is 'glitchy'... some folders are not hidden when should
    QString f = filter;
    f.replace(" ", "*");
    f.prepend("*");
    f.append("*");
    m_activeContainer->model()->setNameFilters(QStringList() << f);
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
        selectedItem = m_model->fileName(selectedItems.at(0));

    if(selectedItems.count() == 1)
    {
        slctnMessage = " ::  " + QString::fromLatin1("\'") + selectedItem + QString::fromLatin1("\'") + " Selected";

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

    QApplication::clipboard()->setMimeData(m_model->mimeData(selectedItems));
}

void
MainWindow::pasteSelection()
{
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();

    if (!mimeData->hasUrls())
        return;

    IO::Job::copy(mimeData->urls(), m_model->filePath(m_activeContainer->currentView()->rootIndex()), m_cut, m_cut);

    QApplication::clipboard()->clear();

    m_cut = false;
}

void
MainWindow::rootPathChanged(QString index)
{
    setWindowTitle(m_model->rootDirectory().dirName());
    m_filterBox->clear();
    m_activeContainer->setFilter("");
    m_placesView->activateAppropriatePlace(index);
    m_tabBar->setTabText(m_tabBar->currentIndex(), m_model->rootDirectory().dirName());
    if ( Store::config.behaviour.gayWindow )
        m_tabBar->setTabIcon(m_tabBar->currentIndex(), m_model->iconProvider()->icon(QFileInfo(m_model->rootPath())));
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
    QDir crnt = m_model->rootDirectory();

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
    if (realSize.count() == 2)
        szEnd = realSize.at(1);
    QString end;
    if (szEnd.count() >= 3)
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
    else if (szEnd.count() == 1)
        end = szEnd + "0";
    else if (szEnd.count() == 2)
        end = szEnd;

    if (!end.isEmpty())
        sz = sz + "." + end;


    QString messaage;
    QString _folder = folders > 1 ? " Folders" : " Folder";
    QString _file = files > 1 ? " Files " : " File ";

    if (folders >= 1 && files >= 1)
        messaage = QString::number(folders) + _folder + ", " + QString::number(files) + _file + "(" + sz + statSize + ")";
    else if (folders >= 1 && files == 0)
        messaage = QString::number(folders) + _folder;
    else if (folders == 0 && files >= 1)
        messaage = QString::number(files) + _file + sz + " " + statSize;

    statusMessage = messaage;
    m_statusBar->showMessage(messaage);
}

void
MainWindow::closeEvent(QCloseEvent *event)
{
    for ( int i = 0; i < m_stackedWidget->count(); ++i )
        static_cast<ViewContainer *>(m_stackedWidget->widget(i))->deleteLater();
    writeSettings();
    s_openWindows.removeOne(this);
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
                m_model->index(m_model->rootPath())) ? statusMessage : statusMessage + slctnMessage;
        if(m_statusBar->currentMessage() != newMessage)
            m_statusBar->showMessage(newMessage);
        return false;
    }

    if (obj == m_placesView && (event->type() == QEvent::Resize || event->type() == QEvent::Show) && !m_dockLeft->isFloating())
    {
        int w = m_placesView->viewport()->width();
        w -= m_toolBar->widgetForAction(m_goBackAct)->width();
        w -= m_toolBar->widgetForAction(m_goForwardAct)->width();
        w -= style()->pixelMetric(QStyle::PM_ToolBarSeparatorExtent);
        if (m_toolBarSpacer->width() != w )
        {
            const int width = qMax(0, w + style()->pixelMetric(QStyle::PM_DockWidgetSeparatorExtent)/2);
            m_toolBarSpacer->setFixedWidth(width);
        }
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
    m_model->setPath(QDir::homePath());
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
    QFileInfo file(m_model->rootPath());
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
        m_model->setFilter(QDir::NoDotAndDotDot | QDir::AllEntries | QDir::System | QDir::Hidden);
    else
        m_model->setFilter(QDir::NoDotAndDotDot | QDir::AllEntries | QDir::System);
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
    connect( container->model(), SIGNAL(rootPathChanged(QString)), this, SLOT(rootPathChanged(QString)));
    connect( container->model(), SIGNAL(rootPathChanged(QString)), m_recentFoldersView, SLOT(folderEntered(QString)));
    connect( container, SIGNAL(viewChanged()), this, SLOT(checkViewAct()));
    connect( container->model(), SIGNAL(directoryLoaded(QString)), this, SLOT(directoryLoaded(QString)));
    connect( container->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(mainSelectionChanged(QItemSelection,QItemSelection)));
    connect( container, SIGNAL(iconSizeChanged(int)), this, SLOT(setSliderPos(int)));
    connect( container, SIGNAL(viewportEntered()), this, SLOT(viewClearHover()));
    connect( container, SIGNAL(leftView()), this, SLOT(viewClearHover()));
    connect( container, SIGNAL(newTabRequest(QString)), this, SLOT(addTab(QString)));
    connect( container, SIGNAL(entered(QModelIndex)), m_infoWidget, SLOT(hovered(QModelIndex)));
    connect( container, SIGNAL(entered(QModelIndex)), this, SLOT(viewItemHovered(QModelIndex)));
    connect( container, SIGNAL(sortingChanged(int,Qt::SortOrder)), this, SLOT(sortingChanged(int,Qt::SortOrder)));

    QList<QAction *> actList;
    actList << m_openInTabAct << m_mkDirAct << m_pasteAct << m_copyAct << m_cutAct
            << m_delCurrentSelectionAct << m_renameAct << m_cstCmdAct << m_goUpAct
            << m_goBackAct << m_goForwardAct << m_propertiesAct;

    container->addActions(actList);
    m_stackedWidget->addWidget(container);
    if (!m_model)
        m_model = container->model();
    static_cast<FileIconProvider *>(m_model->iconProvider())->loadThemedFolders(m_model->rootPath());

    if ( Store::config.behaviour.gayWindow )
        m_tabBar->addTab(m_model->iconProvider()->icon(QFileInfo(container->model()->rootPath())), QFileInfo(container->model()->rootPath()).fileName());
    else
        m_tabBar->addTab(QFileInfo(container->model()->rootPath()).fileName());

    if (Store::config.behaviour.hideTabBarWhenOnlyOneTab)
        m_tabBar->setVisible(m_tabBar->count() > 1);
}

void
MainWindow::tabChanged(int currentIndex)
{
    if ( currentIndex < 0 )
        return;
    m_stackedWidget->setCurrentIndex(currentIndex);
    m_activeContainer = static_cast<ViewContainer*>(m_stackedWidget->currentWidget());
    sortingChanged(m_activeContainer->model()->sortingColumn(), m_activeContainer->model()->sortingOrder());
    emit viewChanged( m_activeContainer->currentView() );
    m_model = m_activeContainer->model();
    m_recentFoldersView->folderEntered(m_model->rootPath());
    setWindowTitle(m_tabBar->tabText(currentIndex));
    m_activeContainer->setPathEditable(m_pathEditAct->isChecked());
    m_iconSizeSlider->setValue(m_activeContainer->iconSize().width()/16);
    m_iconSizeSlider->setToolTip("Size: " + QString::number(m_activeContainer->iconSize().width()) + " px");
    m_placesView->activateAppropriatePlace(m_model->rootPath());
    updateStatusBar(m_model->rootPath());
    m_filterBox->setText(m_activeContainer->currentFilter());
    setActions();
}

void
MainWindow::tabMoved(int from, int to)
{
    QWidget *w = m_stackedWidget->widget(from);

    m_stackedWidget->removeWidget(w);
    m_stackedWidget->insertWidget(to, w);
    if ( to == m_tabBar->currentIndex() )
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
    if ( m_activeContainer->selectionModel()->hasSelection() )
        foreach ( const QModelIndex &index, m_activeContainer->selectionModel()->selectedIndexes() )
        {
            if ( index.column() > 0 )
                continue;
            addTab(m_model->filePath(index));
        }
}

void
MainWindow::newTab()
{
    QString rootPath;
    rootPath = m_model->rootPath();
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
    const QString &file = m_model->filePath( m_model->index( m_model->filePath(index) ) );
    m_statusBar->showMessage( file );
}

void
MainWindow::viewClearHover()
{
    QString newMessage = ( m_activeContainer->selectionModel()->selection().isEmpty()
                           ||  m_activeContainer->selectionModel()->currentIndex().parent() != m_model->index(m_model->rootPath()))
            ? statusMessage : statusMessage + slctnMessage;
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
    foreach ( const QModelIndex &index, m_activeContainer->selectionModel()->selectedIndexes() )
    {
        if ( index.column() != 0 )
            continue;
        files << m_model->filePath(index);
    }
    PropertiesDialog::forFiles(files);
}

void
MainWindow::hidePath()
{
    foreach ( BreadCrumbs *bc, findChildren<BreadCrumbs *>() )
        bc->setVisible(m_pathVisibleAct->isChecked());
    Store::settings()->setValue("pathVisible", m_pathVisibleAct->isChecked());
}

void
MainWindow::sortingChanged(const int column, const Qt::SortOrder order)
{
//    qDebug() << "sortingChanged" << col << order;
    Store::config.behaviour.sortingCol=column;
    Store::config.behaviour.sortingOrd=order;
    m_sortNameAct->setChecked(column == 0);
    m_sortSizeAct->setChecked(column == 1);
    m_sortTypeAct->setChecked(column == 2);
    m_sortDateAct->setChecked(column == 3);
    m_sortDescAct->setChecked(order == Qt::DescendingOrder);

#ifdef Q_WS_X11
    if ( Store::config.views.dirSettings )
    {
        QString dirPath = m_model->rootPath();
        if ( !dirPath.endsWith(QDir::separator()) )
            dirPath.append(QDir::separator());
        dirPath.append(".directory");
        QSettings settings(dirPath, QSettings::IniFormat);
        settings.beginGroup("DFM");
        QVariant varCol = settings.value("sortCol");
        if ( varCol.isValid() )
        {
            int col = varCol.value<int>();
            if ( col != column )
                settings.setValue("sortCol", column);
        }
        else
            settings.setValue("sortCol", column);
        QVariant varOrd = settings.value("sortOrd");
        if ( varCol.isValid() )
        {
            Qt::SortOrder ord = (Qt::SortOrder)varOrd.value<int>();
            if ( ord != order )
                settings.setValue("sortOrd", order);
        }
        else
            settings.setValue("sortOrd", order);
        settings.endGroup();
    }
#endif
}

void
MainWindow::setSorting()
{
    int i = 0;
    if ( m_sortNameAct->isChecked() )
        i = 0;
    else if ( m_sortSizeAct->isChecked() )
        i = 1;
    else if ( m_sortTypeAct->isChecked() )
        i = 2;
    else if ( m_sortDateAct->isChecked() )
        i = 3;
    m_activeContainer->sort(i, (Qt::SortOrder)m_sortDescAct->isChecked());
}

void
MainWindow::stackChanged(int)
{
    if ( m_activeContainer )
        m_filterBox->setText(m_activeContainer->currentFilter());
}

void
MainWindow::showEvent(QShowEvent *e)
{
    QMainWindow::showEvent(e);
    int w = m_placesView->viewport()->width();
    w -= m_toolBar->widgetForAction(m_goBackAct)->width();
    w -= m_toolBar->widgetForAction(m_goForwardAct)->width();
    w -= style()->pixelMetric(QStyle::PM_ToolBarSeparatorExtent);
    if (m_toolBarSpacer->width() != w )
    {
        const int width = qMax(0, w + style()->pixelMetric(QStyle::PM_DockWidgetSeparatorExtent)/2);
        m_toolBarSpacer->setFixedWidth(width);
    }
}

void
MainWindow::windowActivationChange(bool wasActive)
{
    if ( s_currentWindow )
        disconnect(APP, SIGNAL(lastMessage(QStringList)), s_currentWindow, SLOT(receiveMessage(QStringList)));
    if ( !wasActive )
    {
        s_currentWindow = this;
        emit viewChanged(m_activeContainer->currentView());
    }
    if ( s_currentWindow )
        connect(APP, SIGNAL(lastMessage(QStringList)), s_currentWindow, SLOT(receiveMessage(QStringList)));
    if ( Store::config.behaviour.gayWindow )
        foreach ( WinButton *btn, findChildren<WinButton *>() )
        {
            btn->setProperty("isActive", isActiveWindow());
            btn->setStyleSheet(qApp->styleSheet());
            btn->update();
        }
}

MainWindow *MainWindow::currentWindow() { return s_currentWindow; }
QList<MainWindow *> MainWindow::openWindows() { return s_openWindows; }
ViewContainer *MainWindow::currentContainer() { return s_currentWindow->activeContainer(); }
PlacesView *MainWindow::places() { return s_currentWindow->placesView(); }

