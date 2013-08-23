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


#include "placesview.h"
#include "iojob.h"
#include "icondialog.h"
#include "viewanimator.h"
#include "mainwindow.h"

#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMetaObject>
#include <QMetaProperty>

using namespace DFM;

inline static QLinearGradient simple( QRect rect, QColor color, int strength )
{
    QColor black( Qt::black ), white( Qt::white );
    black.setAlpha( color.alpha() );
    white.setAlpha( color.alpha() );

    QLinearGradient s( rect.topLeft(), rect.bottomLeft() );
    s.setColorAt( 0, Operations::colorMid( color, white, 100, strength ) );
    s.setColorAt( 1, Operations::colorMid( color, black, 100, strength ) );
    return s;
}

inline static QPolygon arrow( QRect rect, bool expanded )
{
    QPolygon a;
    if ( expanded )
    {
        rect.adjust( 0, 1, 0, -1);
        a << rect.topLeft() << rect.topRight() << QPoint( rect.center().x(), rect.bottom() );
    }
    else
    {
        rect.adjust( 1, 0, -1, 0);
        a << rect.topLeft() << QPoint( rect.right(), rect.center().y() ) << rect.bottomLeft();
    }
    return a;
}

inline static void renderArrow( QRect rect, QPainter *painter, QColor color, bool expanded )
{
    painter->save();
    QPainterPath path;
    path.addPolygon( arrow( rect, expanded ) );
    painter->setBrush( color );
    painter->setPen( Qt::NoPen );
    painter->drawPath( path );
    painter->restore();
}

inline void renderFrame( QRect rect, QPainter *painter, QColor color, uint pos = 15 )
{
    QRect r( rect );
    painter->save();
    painter->translate( 0.5, 0.5 );
    painter->setPen( color );
    if ( pos & 1 ) //top
    {
        painter->drawLine( r.topLeft(), r.topRight() );
        r.setTop( r.top()+1 );
    }
    if ( pos & 2 ) //bottom
    {
        painter->drawLine( r.bottomLeft(), r.bottomRight() );
        r.setBottom( r.bottom()-1 );
    }
    if ( pos & 4 ) //left
        painter->drawLine( r.topLeft(), r.bottomLeft() );
    if ( pos & 8 ) //right
        painter->drawLine( r.topRight(), r.bottomRight() );
    painter->restore();
}

#define TEXT index.data().toString()
#define RECT option.rect
#define FM option.fontMetrics
#define PAL option.palette
#define DECOSIZE option.decorationSize

inline static void drawDeviceUsage( const int &usage, QPainter *painter, const QStyleOptionViewItem &option )
{
    painter->save();
    QRect rect = RECT;
    painter->setPen( Qt::NoPen );
    rect.setTop( rect.top()+1 );
    rect.setBottom( rect.bottom()-1 );
    if ( !( option.state & ( QStyle::State_MouseOver | QStyle::State_Selected ) ) )
        renderFrame( rect, painter, QColor( 0, 0, 0, 32 ), 15 );
    rect.setWidth( rect.width()*usage/100 );
    QColor progress(Operations::colorMid(Qt::green, Qt::red, 100-usage, usage));
    progress.setAlpha(64);
    painter->fillRect( rect, simple( rect, progress, 80 ) );
    renderFrame( rect.adjusted(1, 1, -1, -1), painter, QColor(255, 255, 255, 64));
    painter->restore();
}

void
PlacesViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save(); //must save... have to restore at end
    painter->setRenderHint( QPainter::Antialiasing );

    const bool &underMouse = option.state & QStyle::State_MouseOver,
            &selected = option.state & QStyle::State_Selected,
            &hasFocus = option.state & QStyle::State_HasFocus;

    QColor baseColor = m_placesView->palette().color( QPalette::Base );
    if ( underMouse )
        painter->fillRect( RECT, baseColor );

    int step = selected ? 8 : ViewAnimator::hoverLevel(m_placesView, index);

    int textFlags = Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine;
    int indent = DECOSIZE.height(), textMargin = indent + ( isHeader( index ) ? 0 : DECOSIZE.width() );
    QRect textRect( RECT.adjusted( textMargin, 0, 0, 0 ) );

    QColor mid = Operations::colorMid( PAL.color( QPalette::Base ), PAL.color( QPalette::Text ) );
    QColor fg = isHeader( index ) ? Operations::colorMid( PAL.color( QPalette::Highlight ), mid, 1, 5 ) : PAL.color( QPalette::Text );
    QPalette pal(PAL);
    pal.setColor(QPalette::Text, fg);
    const QStyleOptionViewItem &copy(option);
    const_cast<QStyleOptionViewItem *>(&copy)->palette = pal;

    QFont font( option.font );
    font.setBold( selected || isHeader( index ) );
    painter->setFont( font );

    if ( step )
    {
        const_cast<QStyleOptionViewItem *>(&copy)->state |= QStyle::State_MouseOver; //grrrrr, must trick styles....
        painter->setOpacity((1.0f/8.0f)*step);
        QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &copy, painter, m_placesView);
        painter->setOpacity(1);
    }

#ifdef Q_WS_X11
    if ( index.parent().isValid() && Configuration::config.behaviour.devUsage )
        if ( m_placesView->deviceManager()->isDevice(m_placesView->itemFromIndex<QStandardItem *>(index)) )
            if ( DeviceItem *d = m_placesView->itemFromIndex<DeviceItem *>(index) )
                if ( d->isMounted() )
                    drawDeviceUsage(d->used(), painter, option);
#endif

    QApplication::style()->drawItemText(painter, textRect, textFlags, pal, true, TEXT, selected ? QPalette::HighlightedText : QPalette::Text);
    if ( isHeader( index ) )
    {
        QRect arrowRect( 0, 0, 8, 8 );
        arrowRect.moveCenter( QPoint( RECT.x()+4, RECT.center().y() ) );
        renderArrow( arrowRect, painter, selected ? PAL.color(QPalette::HighlightedText) : fg, bool( option.state & QStyle::State_Open ) );
    }
    QRect iconRect( QPoint( RECT.x()+( indent/2 ), RECT.y() + ( RECT.height() - DECOSIZE.height() )/2 ), DECOSIZE );
    QPixmap iconPix = qvariant_cast<QIcon>( index.data( Qt::DecorationRole ) ).pixmap( DECOSIZE.height() );

    if ( selected )
    {
        QImage img = iconPix.toImage();
        img = img.convertToFormat(QImage::Format_ARGB32);
        int size = img.width() * img.height();
        QRgb *pixel = reinterpret_cast<QRgb *>(img.bits());
        int r = 0, g = 0, b = 0;
        PAL.color( QPalette::HighlightedText ).getRgb(&r, &g, &b); //foregroundcolor
        for ( int i = 0; i < size; ++i )
            pixel[i] = qRgba( r, g, b, qBound(0, qAlpha(pixel[i]) - qGray(pixel[i]), 255) );

        iconPix = QPixmap::fromImage(img);
    }


//    if ( qMax(iconPix.width(), iconPix.height()) > 16 )
//        iconPix = iconPix.scaled(QSize(16, 16),Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QApplication::style()->drawItemPixmap(painter, iconRect, Qt::AlignCenter, iconPix);
    if ( selected ) QApplication::style()->drawItemPixmap(painter, iconRect, Qt::AlignCenter, iconPix);
    painter->restore();
}

void
PlacesViewDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    const QString &newName = editor->metaObject()->userProperty().read(editor).toString();
    if ( Place *p = m_placesView->itemFromIndex<Place *>(index) )
        p->setName(newName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef Q_WS_X11
DeviceItem::DeviceItem(DeviceManager *parentItem, PlacesView *view, Solid::Device solid )
    : Place(QStringList())
    , m_view(view)
    , m_manager(parentItem)
    , m_tb(new QToolButton(m_view))
    , m_timer(new QTimer(this))
    , m_solid(solid)
{
    m_tb->setIcon(mountIcon(isMounted(), 16, m_view->palette().color(m_view->foregroundRole())));
    m_tb->setVisible(true);
    m_tb->setFixedSize(16, 16);
    m_tb->setToolTip( isMounted() ? "Unmount" : "Mount" );
    connect (m_tb, SIGNAL(clicked()), this, SLOT(toggleMount()));
    connect (m_timer, SIGNAL(timeout()), this, SLOT(updateSpace()));
    m_timer->start(1000);
    connect (m_solid.as<Solid::StorageAccess>(), SIGNAL(accessibilityChanged(bool, const QString &)), this, SLOT(changeState()));
    updateSpace();
    setDragEnabled(false);
    connect( m_view, SIGNAL(expanded(QModelIndex)), this, SLOT(updateTb()) );
    connect( m_view, SIGNAL(changed()), this, SLOT(updateTb()) );
    connect( m_view, SIGNAL(collapsed(QModelIndex)), this, SLOT(updateTb()) );
    connect( m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(updateTb()) );
    QTimer::singleShot(200, this, SLOT(updateTb()));
}

void
DeviceItem::updateTb()
{
    QRect rect = m_view->visualRect(index());
    m_tb->setVisible(m_view->isExpanded(m_manager->index()));
    const int &add = (rect.height()/2.0f)-(m_tb->height()/2.0f);
    const int &x = 8, &y = rect.y()+add;
    m_tb->move(x, y);
}

void
DeviceItem::setMounted( const bool &mount )
{
    if ( !m_solid.isValid() )
        return;

    if ( mount )
        m_solid.as<Solid::StorageAccess>()->setup();
    else
        m_solid.as<Solid::StorageAccess>()->teardown();
}

void
DeviceItem::updateSpace()
{
    if ( isMounted() )
    {
        if ( Configuration::config.behaviour.devUsage )
            m_view->update( index() );
        int t = 0;
        QString free(QString::number(realSize((float)freeBytes(), &t)));
        setToolTip(free + spaceType[t] + " Free");
    }
}

void
DeviceItem::changeState()
{
    if (isMounted())
    {
        int t = 0;
        setToolTip(QString::number(realSize((float)freeBytes(), &t)) + spaceType[t] + " Free");
    }
    else
    {
        setToolTip(QString());
    }
    m_tb->setIcon(mountIcon(isMounted(), 16, m_view->palette().color(m_view->foregroundRole())));
    m_tb->setToolTip( isMounted() ? "Unmount" : "Mount" );
}

DeviceManager::DeviceManager(const QStringList &texts, QObject *parent)
    : Container(QString("Devices"))
    , QObject(parent)
    , m_view(static_cast<PlacesView *>(parent))
{
    connect (Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(QString)), this, SLOT(deviceAdded(QString)));
    connect (Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(QString)), this, SLOT(deviceRemoved(QString)));
    populate();
    setDragEnabled(false);
    setDropEnabled(false);
}

void
DeviceManager::deviceAdded(const QString &dev)
{
    Solid::Device device = Solid::Device(dev);
    if ( device.is<Solid::StorageAccess>() )
    {
        DeviceItem *d = new DeviceItem(this, m_view, device);
        appendRow(d);
        m_items.insert(dev, d);
    }
}

void
DeviceManager::deviceRemoved(const QString &dev)
{
    if ( m_items.contains(dev) )
    {
        QStandardItem *i = m_items.take(dev);
        removeRow(i->row());
    }
}

void
DeviceManager::populate()
{
    foreach ( Solid::Device dev, Solid::Device::listFromType(Solid::DeviceInterface::StorageAccess) )
    {
        DeviceItem *d = new DeviceItem( this, m_view, dev );
        appendRow(d);
        m_items.insert(dev.udi(), d);
    }
}

DeviceItem
*DeviceManager::deviceItemForFile(const QString &file)
{
    QString s(file.isEmpty() ? "/" : file);
    foreach ( DeviceItem *item, m_items.values() )
        if ( s == item->mountPath() )
            return item;
    return deviceItemForFile(s.mid(0, s.lastIndexOf("/")));
}
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QVariant
PlacesModel::data(const QModelIndex &index, int role) const
{
    if ( role == Qt::DisplayRole || role == Qt::DecorationRole )
        if ( Place *p = m_places->itemFromIndex<Place *>(index) )
            if ( role == Qt::DisplayRole )
                return p->name();
            else if ( role == Qt::DecorationRole )
                return QIcon::fromTheme( p->iconName() );
    return QStandardItemModel::data(index, role);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PlacesView::PlacesView( QWidget *parent )
    : QTreeView( parent )
    , m_timer(new QTimer(this))
    , m_devManager(0L)
{
    ViewAnimator::manage(this);
    setModel(m_model = new PlacesModel(this));
    m_timer->setInterval( 1000 );

    setUniformRowHeights( false );
    setAllColumnsShowFocus( true );
    setItemDelegate( new PlacesViewDelegate( this ) );
    setHeaderHidden( true );
    setItemsExpandable( true );
    setIndentation( 0 );
    setIconSize( QSize( 16, 16 ) );
    setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    setDragDropMode( QAbstractItemView::DragDrop );
    setAcceptDrops(true);
    setDragEnabled(true);
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDefaultDropAction( Qt::MoveAction );
//    setEditTriggers( QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed );
    setMouseTracking( true );
    setExpandsOnDoubleClick( false );
    setAnimated( true );

    //base color... slight hihglight tint
    QPalette pal = palette();
    QColor midC = Operations::colorMid( pal.color( QPalette::Base ), pal.color( QPalette::Highlight ), 10, 1 );
    pal.setColor( QPalette::Base, Operations::colorMid( Qt::black, midC, 1, 10 ) );
    setPalette( pal );

    connect ( this, SIGNAL(changed()), this, SLOT(store()) );
    connect ( this, SIGNAL(clicked(QModelIndex)), this, SLOT(emitPath(QModelIndex)) );
    connect ( this, SIGNAL(placeActivated(QString)), MainWindow::currentWindow(), SLOT(setRootPath(QString)) );
    connect ( m_timer, SIGNAL(timeout()), this, SLOT(updateAllWindows()) );
}

#define RETURN \
    event->setDropAction(Qt::IgnoreAction); \
    event->accept(); \
    QTreeView::dropEvent(event); \
    return

void
PlacesView::dropEvent( QDropEvent *event )
{
    Container *cont = itemAt<Container *>(event->pos());
    Place *place = itemAt<Place *>(event->pos());

    QStringList files;
    foreach ( const QUrl &url, event->mimeData()->urls() )
    {
        const QString &file = url.toLocalFile();
        if ( QFileInfo(file).isReadable() )
            files << file;
    }

    if ( dropIndicatorPosition() == OnViewport )
    {
        if ( event->source() == this ) { RETURN; }

        Container *places = 0L;
        foreach ( Container *c, containers() )
            if ( c->name() == "Places" )
                places = c;
        if ( !places )
        {
            places = new Container("Places");
            m_model->insertRow(0, places);
        }
        foreach ( const QString &file, files )
        {
            const QFileInfo &f(file);
            if ( !f.isDir() )
                continue;
            QSettings settings( QString("%1%2.directory").arg(f.filePath()).arg(QDir::separator()), QSettings::IniFormat );
            const QIcon &icon = QIcon::fromTheme(settings.value( "Desktop Entry/Icon" ).toString(), QIcon::fromTheme("inode-directory", QIcon::fromTheme("folder")));
            places->appendRow(new Place(f.fileName(), f.filePath(), icon.name()));
        }
        expand(indexFromItem(places));
        emit changed();
        RETURN;
    }

#ifdef Q_WS_X11
    if ( itemAt<DeviceItem *>(event->pos())
         ||  ( dynamic_cast<DeviceItem *>(m_lastClicked)
               && dropIndicatorPosition() != OnItem
               && event->source() == this )
         || dynamic_cast<DeviceManager *>(m_lastClicked)
         || ( itemAt<DeviceManager *>(event->pos())
              && dropIndicatorPosition() != AboveItem ) )
    {
        RETURN;
    }
#endif

    const bool &below = bool(dropIndicatorPosition() == BelowItem);

    if ( event->source() != this )
    {
        if ( files.isEmpty() ) { RETURN; }

        if ( dropIndicatorPosition() == OnItem )
        {

            if ( cont ) //dropping files on a container
            {
                foreach ( const QString &file, files )
                {
                    const QFileInfo &f(file);
                    if ( !f.isDir() )
                        continue;
                    QSettings settings( QString("%1%2.directory").arg(f.filePath()).arg(QDir::separator()), QSettings::IniFormat );
                    const QIcon &icon = QIcon::fromTheme(settings.value( "Desktop Entry/Icon" ).toString(), QIcon::fromTheme("inode-directory", QIcon::fromTheme("folder")));
                    addPlace( f.fileName(), f.filePath(), icon, cont );
                }
                emit changed();
                RETURN;
            }

            if ( place ) //dropping files on a place
            {
                QApplication::restoreOverrideCursor();
                int ret = QMessageBox::question(MainWindow::currentWindow(), tr("Are you sure?"), tr("you are about to move some stuff..."), QMessageBox::Yes, QMessageBox::No);
                if ( ret == QMessageBox::Yes )
                    IO::Job::copy( files, place->path(), true );
                RETURN;
            }

        }
        else if ( dropIndicatorPosition() != QAbstractItemView::OnViewport )
        {
            foreach ( const QString &file, files )
            {
                const QFileInfo &f(file);
                if ( !f.isDir() )
                    continue;
                QSettings settings( QString("%1%2.directory").arg(f.filePath()).arg(QDir::separator()), QSettings::IniFormat );
                const QIcon &icon = QIcon::fromTheme(settings.value( "Desktop Entry/Icon" ).toString(), QIcon::fromTheme("inode-directory", QIcon::fromTheme("folder")));
                Place *item = new Place(f.fileName(), f.filePath(), icon.name());
                Container *c = container( indexAt( event->pos() ).parent().row() );
                if ( c )
                    c->insertRow( indexAt( event->pos() ).row()+below, item );
                else
                    cont->insertRow( 0, item );
                emit changed();
            }
            RETURN;
        }
    }
    else
    {
        if ( cont )
            if ( Container *movedCont = dynamic_cast<Container *>(m_lastClicked) ) //moving container
            {
                if ( dropIndicatorPosition() == AboveItem )
                {
                    Container *newCont = new Container(movedCont);
                    m_model->insertRow( cont->row(), newCont );
                    expand(indexFromItem(newCont));
                    m_model->removeRow(movedCont->row());
                    emit changed();
                }
                RETURN;
            }
            else if ( Place *movedPlace = dynamic_cast<Place *>(m_lastClicked) )
            {
                if ( dropIndicatorPosition() == OnItem )
                    cont->appendRow( new Place( movedPlace ) );
                else if ( dropIndicatorPosition() == BelowItem )
                    cont->insertRow( 0, new Place( movedPlace ) );
                movedPlace->parent()->removeRow(movedPlace->row());
                emit changed();
                RETURN;
            }

        if ( place )
            if ( Place *movedPlace = dynamic_cast<Place *>(m_lastClicked) )
            {
                if ( Container *c = dynamic_cast<Container *>(place->parent()) )
                {
                    c->insertRow( place->row() + below, new Place( movedPlace ) );
                    movedPlace->parent()->removeRow(movedPlace->row());
                    emit changed();
                }
                RETURN;
            }
    }
    RETURN;
}

void
PlacesView::mouseReleaseEvent( QMouseEvent *event )
{
//    m_lastClicked = itemAt( event->pos() );
    if ( event->button() == Qt::MiddleButton )
    {
        event->ignore();
        return;
    }
    if ( !indexAt( event->pos() ).parent().isValid() && indexAt( event->pos() ).isValid() )
        setExpanded(indexAt( event->pos() ), !isExpanded(indexAt( event->pos() )));
    QTreeView::mouseReleaseEvent( event );
}

void
PlacesView::mousePressEvent( QMouseEvent *event )
{
    m_lastClicked = itemAt( event->pos() );
    if ( event->button() == Qt::MiddleButton )
    {
        event->ignore();
        if ( indexAt( event->pos() ).isValid() && indexAt( event->pos() ).parent().isValid() )
            emit newTabRequest( static_cast<Place *>(itemAt( event->pos() ))->path() );
        return;
    }
    else
        QTreeView::mousePressEvent(event);
}

void
PlacesView::keyPressEvent( QKeyEvent *event )
{
    if ( event->key() == Qt::Key_Delete )
        removePlace();
    if ( event->key() == Qt::Key_F2 )
        renPlace();
    QTreeView::keyPressEvent( event );
}

void
PlacesView::removePlace()
{
    if ( currentItem() )
#ifdef Q_WS_X11
        if ( currentItem() != m_devManager
             && !m_devManager->isDevice(currentItem()))
#endif
    {
            delete currentItem();
            if ( currentItem()->parent() )
                currentItem()->parent()->removeRow(currentIndex().row());
            else
                m_model->removeRow(currentIndex().row());
            emit changed();
        }
}

void
PlacesView::renPlace()
{
    if ( Place *place = dynamic_cast<Place *>(currentItem()) )
    {
        edit(place->index());
        emit changed();
    }
}

void
PlacesView::addPlace(QString name, QString path, QIcon icon, QStandardItem *parent, const bool &storeSettings)
{
    Container *cont = dynamic_cast<Container *>(parent);
    if ( !cont )
        cont = dynamic_cast<Container *>(currentItem());
    if ( !cont )
        return;
    Place *place = new Place(name, path, icon.name());
    place->setFlags(place->flags() | Qt::ItemIsEditable);
    cont->appendRow(place);
    if ( storeSettings )
    {
        store();
        emit changed();
    }
}

void
PlacesView::addPlaceCont()
{
    Container *item = new Container("New_Container");
    m_model->insertRow(0, item);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    edit(item->index());
    expand(indexFromItem(item));
    emit changed();
}

void
PlacesView::setPlaceIcon()
{
    if ( !m_lastClicked )
        return;
    const QString &i = IconDialog::icon();
    if ( i.isEmpty() || !m_lastClicked->parent() )
        return;
    if ( Place *place = dynamic_cast<Place *>(m_lastClicked) )
    {
        place->setIconName( i );
        emit changed();
    }
}

void
PlacesView::activateAppropriatePlace( const QString &path )
{
    for ( int i = 0; i < containerCount(); i++ )
        for( int a = 0; a < container( i )->rowCount(); a++ )
        {
            Place *place = container(i)->place(a);
            Place *current = static_cast<Place *>(currentItem());
            if ( place->path() == path )
            {
                setCurrentItem( place );
                return;
            }
            else if ( current && path.contains( place->path() ) )
                if ( current->path().length() < place->path().length() )
                    setCurrentItem( place );
        }
}

void
PlacesView::contextMenuEvent( QContextMenuEvent *event )
{
    if ( indexAt( event->pos() ).isValid() && indexAt( event->pos() ).parent().isValid() )
        m_lastClicked = itemAt( event->pos() );

    QMenu popupMenu;
    popupMenu.addActions( actions() );
    popupMenu.exec( event->globalPos() );
}

QMenu
*PlacesView::containerAsMenu(const int &cont)
{
    QMenu *menu = new QMenu(container(cont)->name());
    for ( int i = 0; i < container(cont)->rowCount(); ++i )
    {
        QAction *action = new QAction(container(cont)->place(i)->name(), qApp);
        action->setData(container(cont)->place(i)->path());
        connect (action, SIGNAL(triggered()), Operations::instance(), SLOT(setRootPath()));
        menu->addAction(action);
    }
    return menu;
}

void
PlacesView::updateAllWindows()
{
    if ( !m_devManager )
        return;

    m_timer->stop();

    if ( MainWindow::openWindows().count() == 1 )
        return;

    foreach ( MainWindow *mw, MainWindow::openWindows() )
        if ( mw != MainWindow::currentWindow() )
            mw->placesView()->populate();
}

void
PlacesView::populate()
{
    m_model->clear();
    m_devManager = 0L;
#ifdef Q_WS_X11
    QSettings s("dfm", "bookmarks");
#else
    QSettings s(QSettings::IniFormat, QSettings::UserScope, "dfm", "bookmarks");
#endif
    foreach ( const QString &container, s.value("Containers").toStringList() )
    {
        Container *cont = new Container(container);
        m_model->appendRow(cont);
        s.beginGroup(container);
        foreach ( const QString &itemString, s.childKeys() )
        {
            const QStringList &item = s.value(itemString).toStringList();
            if ( item.count() == 3 )
                addPlace(item[Place::Name], item[Place::Path], QIcon::fromTheme(item[Place::Icon]), cont, false);
        }
        s.beginGroup("Options");
        setExpanded(cont->index(), s.value("Expanded", false).toBool());
        s.endGroup();
        s.endGroup();
    }
#ifdef Q_WS_X11
    m_model->appendRow(m_devManager = new DeviceManager(QStringList() << tr("Devices"), this));
    expand(m_devManager->index());
#endif
}

void
PlacesView::store()
{
#ifdef Q_WS_X11
    QSettings s("dfm", "bookmarks");
#else
    QSettings s(QSettings::IniFormat, QSettings::UserScope, "dfm", "bookmarks");
#endif
    s.clear();

    QStringList containers;
    for ( int i = 0; i < containerCount(); ++i )
        if ( container(i) != (Container*)m_devManager )
            containers << container(i)->name();
    s.setValue("Containers", containers);

    for ( int c = 0; c < containerCount(); ++c )
    {
        if ( container(c) == (Container*)m_devManager )
            continue;
        s.beginGroup(container(c)->name());
        for ( int i = 0; i < container(c)->rowCount(); ++i )
        {
            Place *p = container(c)->place(i);
            if ( container(c) != (Container*)m_devManager )
                s.setValue(QString::number(i), QStringList() << p->name() << p->path() << p->iconName());
        }
        if ( container(c)->rowCount() )
        {
            s.beginGroup("Options");
            s.setValue("Expanded", isExpanded( indexFromItem( container(c) ) ));
            s.endGroup();
        }
        s.endGroup();
    }
    m_timer->start();
}

void
PlacesView::emitPath(const QModelIndex &index)
{
    if ( Place *p = itemFromIndex<Place *>(index) )
        if ( !dynamic_cast<Container *>(p) )
            emit placeActivated(p->path());
}

void
PlacesView::drawBranches( QPainter *painter,
                              const QRect &rect,
                              const QModelIndex &index ) const
{
#if 0 //we want this to be empty.... we dont want any branch.. at least not atm
    if ( !index.parent().isValid() )
    {
        painter->save();
        QRect r/*( rect )*/;
        r.setSize( QSize( 8, 8 ) );
        r.moveCenter( QPoint( rect.right()+iconSize().height()/2, rect.center().y() ) );
        QPainterPath triangle;
        if ( itemFromIndex( index )->isExpanded() )
        {
            r.translate( 0, 1 );
            triangle.moveTo( r.topLeft() );
            triangle.lineTo( r.topRight() );
            triangle.lineTo( r.adjusted( 0, 0, -r.width()/2, 0 ).bottomRight() );
        }
        else
        {
            triangle.moveTo( r.topLeft() );
            triangle.lineTo( r.adjusted( 0, r.height()/2, 0, 0 ).topRight() );
            triangle.lineTo( r.bottomLeft() );
        }
        triangle.closeSubpath();

        painter->setRenderHint( QPainter::Antialiasing );
        painter->setPen( Qt::NoPen );
        painter->setBrush( palette().color( QPalette::WindowText ) );
        painter->drawPath( triangle );
        painter->restore();
    }
#endif
}

#if 0
void
PlacesView::mtabChanged( QString mtabPath )
{
    takeTopLevelItem( topLevelItemCount()-1 );
    QFile mtab( mtabPath );
    mtab.open( QFile::ReadOnly );
    QStringList mtabCont;
    QTextStream stream( &mtab );
    do mtabCont.append( stream.readLine() );
    while ( !stream.atEnd() );
    mtab.close();

    QFileSystemWatcher *fsWatcher = new QFileSystemWatcher( QStringList() << "/etc/mtab", this );
    connect( fsWatcher, SIGNAL( fileChanged( QString ) ), this, SLOT( mtabChanged( QString ) ) );

    QTreeWidgetItem *item = new QTreeWidgetItem( this );
    item->setText( 0, tr( "Devices" ) );

    foreach( QString deviceString, mtabCont )
        if ( deviceString[0] == '/' )
        {
            const QString ds = deviceString;
            QString devString = ds.split( " " ).at( 1 ); //split the string on " ", which makes in this case about 6 strings, the second is the path, it's what we use.
            QTreeWidgetItem *device = new QTreeWidgetItem( item );
            device->setText( 0, devString );
            device->setText( 1, devString );
            device->setText( 2, "Devices" );
            device->setText( 3, ds.split( " " ).at( 0 ) );  //store the devicepath /dev/sdX here
            device->setIcon( 0, QIcon::fromTheme( "inode-directory" ) );
        }
    item->setExpanded( true );
}

QList<QStringList> getKdePlaces()
{
    QFile* file = new QFile( QDir::homePath() + QDir::separator() + ".kde4/share/apps/kfileplaces/bookmarks.xml" );

    QList<QStringList> tmp;
    if ( !file->open( QIODevice::ReadOnly | QIODevice::Text ) )
        return tmp;

    QDomDocument doc( "places" );
    if ( !doc.setContent( file ) )
        return tmp;

    QDomNodeList nodeList = doc.documentElement().childNodes();
    for( int i = 0;i < nodeList.count(); i++ )
    {
        QDomElement el = nodeList.at( i ).toElement();
        if ( el.nodeName() == "bookmark" )
        {
            QStringList values;
            QDomAttr a = el.attributeNode( "href" );
            values << a.value();
            QDomElement title = el.elementsByTagName( "title" ).at( 0 ).toElement();
            values << title.text();
            QDomElement info = el.elementsByTagName( "info" ).at( 0 ).toElement();
            QDomElement metadata = info.elementsByTagName( "metadata" ).at( 0 ).toElement();
            QDomElement eleIcon = metadata.elementsByTagName( "bookmark:icon" ).at( 0 ).toElement();
            values << eleIcon.attributeNode( "name" ).value();
            tmp << values;
        }
    }
    return tmp;
}
#endif
