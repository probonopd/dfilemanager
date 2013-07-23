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
#include "devicemanager.h"
#include "iojob.h"
#include "icondialog.h"
#include "viewanimator.h"
#include "mainwindow.h"

#include <QInputDialog>
#include <QMenu>

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
    static PlacesView *placesView = APP->placesView();

    painter->save(); //must save... have to restore at end
    painter->setRenderHint( QPainter::Antialiasing );

    const bool &underMouse = option.state & QStyle::State_MouseOver,
            &selected = option.state & QStyle::State_Selected,
            &hasFocus = option.state & QStyle::State_HasFocus;

    QColor baseColor = placesView->palette().color( QPalette::Base );
    if ( underMouse )
        painter->fillRect( RECT, baseColor );

    int step = selected ? 8 : ViewAnimator::hoverLevel(placesView, index);

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
        QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &copy, painter, placesView);
        painter->setOpacity(1);
    }
    if ( index.parent().isValid() && MainWindow::config.behaviour.devUsage )
        if ( placesView->itemFromIndex(index.parent())->text(3) == "Devices" )
            if ( DeviceItem *d = static_cast<DeviceItem*>(placesView->itemFromIndex(index)) )
                if ( d->isMounted() )
                    drawDeviceUsage(d->used(), painter, option);

    QApplication::style()->drawItemText(painter, textRect, textFlags, pal, true, TEXT, selected ? QPalette::HighlightedText : QPalette::Text);
    if( isHeader( index ) )
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





////////////////////////////////////////////////////////////////////////////////////////////////////////////////






PlacesView::PlacesView( QWidget *parent ) : QTreeWidget( parent )
{
    APP->setPlacesView( this );
    m_mainWindow = APP->mainWindow();
    ViewAnimator::manage(this);
    DeviceManager::manage(this);

    setUniformRowHeights( false );
    setAllColumnsShowFocus( true );
    setItemDelegate( new PlacesViewDelegate( this ) );
    setHeaderHidden( true );
    setItemsExpandable( true );

    setColumnHidden( 1, true );
    setColumnHidden( 2, true );
    setColumnHidden( 3, true );
    setIndentation( 0 );
    setIconSize( QSize( 16, 16 ) );
    setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    setDragDropMode( QAbstractItemView::DragDrop );
    setDefaultDropAction( Qt::MoveAction );
    setEditTriggers( QAbstractItemView::NoEditTriggers );
    setMouseTracking( true );
    setExpandsOnDoubleClick( false );
    setAnimated( true );


    //base color... slight hihglight tint
    QPalette pal = palette();
    QColor midC = Operations::colorMid( pal.color( QPalette::Base ), pal.color( QPalette::Highlight ), 10, 1 );
    pal.setColor( QPalette::Base, Operations::colorMid( Qt::black, midC, 1, 10 ) );
    setPalette( pal );
}


void
PlacesView::dropEvent( QDropEvent *event )
{
    if ( event->source() != this )
        if ( event->mimeData()->hasUrls() )
            if ( itemAt( event->pos() ) )
                if (dropIndicatorPosition() == QAbstractItemView::OnItem)
                {
                    if ( !itemAt(event->pos())->parent() )
                        if ( QTreeWidgetItem *parent = itemAt(event->pos()) )
                        {
                            QFileInfo file = event->mimeData()->urls().first().toLocalFile();
                            if ( file.isDir() )
                            {
                                QSettings settings( file.filePath() + QDir::separator() + ".directory", QSettings::IniFormat );
                                QString icon("inode-directory");
                                const QString &containerName(parent->text( Name ));
                                if ( !settings.value( "Desktop Entry/Icon" ).toString().isNull() )
                                    icon = settings.value( "Desktop Entry/Icon" ).toString();
                                QTreeWidgetItem *item = new QTreeWidgetItem(parent);
                                item->setIcon( Name, QIcon::fromTheme( icon ) );
                                item->setText( Name, file.fileName() );
                                item->setText( Path, file.filePath() );
                                item->setText( Container, containerName );
                                item->setText( DevPath, icon );
                                event->setDropAction(Qt::IgnoreAction);
                            }
                        }
                    if ( QFileInfo(itemAt(event->pos())->text(Path)).isDir() )
                    {
                        QApplication::restoreOverrideCursor();
                        int ret = QMessageBox::question(m_mainWindow, tr("Are you sure?"), tr("you are about to move some stuff..."), QMessageBox::Yes, QMessageBox::No);
                        if ( ret == QMessageBox::Yes )
                        {
                            QStringList cp;
                            foreach( QUrl url, event->mimeData()->urls() )
                                cp << url.toLocalFile();
                            IO::Job::copy( cp, itemAt( event->pos() )->text( 1 ), true );
                        }
                        event->setDropAction(Qt::IgnoreAction);
                    }
                }
                else if ( dropIndicatorPosition() != QAbstractItemView::OnViewport )
                {
                    if ( !DeviceManager::itemIsDevice(itemAt( event->pos() )) )
                    {
                        QFileInfo file( event->mimeData()->urls().first().toLocalFile() );
                        if ( file.isDir() )
                        {
                            QSettings settings( file.filePath() + QDir::separator() + ".directory", QSettings::IniFormat );
                            QString icon("inode-directory");
                            const QString &containerName(itemAt( event->pos() )->parent() ? itemAt( event->pos() )->parent()->text( Name ) : itemAt( event->pos() )->text( Name ));
                            if ( !settings.value( "Desktop Entry/Icon" ).toString().isNull() )
                                icon = settings.value( "Desktop Entry/Icon" ).toString();
                            QTreeWidgetItem *item = new QTreeWidgetItem;
                            item->setIcon( Name, QIcon::fromTheme( icon ) );
                            item->setText( Name, file.fileName() );
                            item->setText( Path, file.filePath() );
                            item->setText( Container, containerName );
                            item->setText( DevPath, icon );
                            if ( indexAt( event->pos() ).parent().isValid() )
                                topLevelItem( indexAt( event->pos() ).parent().row() )->insertChild( indexAt( event->pos() ).row()+bool( dropIndicatorPosition() == QAbstractItemView::BelowItem ), item );
                            else
                                topLevelItem( indexAt( event->pos() ).row() )->insertChild( 0, item );
                        }
                    }
                    event->setDropAction(Qt::IgnoreAction);
                }

    if ( dropIndicatorPosition() == QAbstractItemView::OnItem ||
            dropIndicatorPosition() == QAbstractItemView::OnViewport ||
            !indexAt( event->pos() ).parent().isValid() ||
         (itemAt(event->pos()) && itemAt(event->pos())->parent()&& itemAt(event->pos())->parent() == DeviceManager::devicesParent()) ||
         (itemAt(event->pos()) && itemAt(event->pos()) == DeviceManager::devicesParent()))
        event->setDropAction(Qt::IgnoreAction);


    QTreeWidget::dropEvent( event );
}

void
PlacesView::mouseReleaseEvent( QMouseEvent *event )
{
    m_lastClicked = itemAt( event->pos() );
    if ( event->button() == Qt::MiddleButton )
    {
        event->ignore();
        return;
    }
    if ( !indexAt( event->pos() ).parent().isValid() && indexAt( event->pos() ).isValid() )
    {
        QTreeWidgetItem *header = itemAt( event->pos() );
        header->setExpanded( !header->isExpanded() );
    }
    QTreeWidget::mouseReleaseEvent( event );
}

void
PlacesView::mousePressEvent( QMouseEvent *event )
{
    if ( event->button() == Qt::MiddleButton )
    {
        event->ignore();
        if ( indexAt( event->pos() ).isValid() && indexAt( event->pos() ).parent().isValid() )
            emit newTabRequest( itemAt( event->pos() )->text( Path ) );
        return;
    }
    else
        QTreeWidget::mousePressEvent(event);
}

void
PlacesView::keyPressEvent( QKeyEvent *event )
{
    if ( event->key() == Qt::Key_Delete )
        removePlace();
    if ( event->key() == Qt::Key_F2 )
        renPlace();
    QTreeWidget::keyPressEvent( event );
}

void
PlacesView::renPlace()
{
    QTreeWidgetItem *item = currentItem();
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    editItem(item);
}

void
PlacesView::addPlace( QString name, QString path, QIcon icon )
{
    QTreeWidgetItem *current = currentItem();
    if ( current->parent() )
        return;

    QTreeWidgetItem *item = new QTreeWidgetItem( current );
    item->setIcon( 0, icon );
    item->setText( 0, name );
    item->setText( 1, path );
    item->setText( 2, item->parent()->text( 0 ) );
}

void
PlacesView::addPlaceCont()
{
    QTreeWidgetItem *item = new QTreeWidgetItem();
    insertTopLevelItem(0, item);
    item->setText(0, "New_Container");
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    editItem(item, 0);
}

void
PlacesView::setPlaceIcon()
{
    const QString &i = IconDialog::icon();
    if ( i.isEmpty() )
        return;
    m_lastClicked->setText( 3, i );
    m_lastClicked->setIcon( 0, QIcon::fromTheme( i ) );
}

void
PlacesView::activateAppropriatePlace( const QString &index )
{
    for ( int i = 0; i < topLevelItemCount(); i++ )
        for( int a = 0; a < topLevelItem( i )->childCount(); a++ )
        {
            if ( topLevelItem( i )->child( a )->text( 1 ) == index )
            {
                setCurrentItem( topLevelItem( i )->child( a ) );
                return;
            }
            else if ( currentItem() && index.contains( topLevelItem( i )->child( a )->text( 1 ) ) )
                if ( currentItem()->text( 1 ).length() < topLevelItem( i )->child( a )->text( 1 ).length() )
                    setCurrentItem( topLevelItem( i )->child( a ) );
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
