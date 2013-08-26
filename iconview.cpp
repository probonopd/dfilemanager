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


#include "iconview.h"
#include "viewcontainer.h"
#include "mainwindow.h"
#include "operations.h"
#include "viewanimator.h"
#include <QAction>
#include <QMenu>
#include <QProcess>
#include <QStyledItemDelegate>
#include <QTextLayout>

#define TEXT index.data().toString()
#define RECT option.rect
#define FM option.fontMetrics
#define PAL option.palette
#define DECOSIZE option.decorationSize

using namespace DFM;

static int lines = 4;

inline static QPixmap shadowPix( int size, int intens, QColor c )
{
    QPixmap pix( size, size );
    pix.fill( Qt::transparent );
    QPainter p( &pix );
    QRect r( pix.rect() );
    const float d = r.height()/2;
    QRadialGradient rg( r.center(), d );
    const QColor &c2(QColor(c.red(), c.green(), c.blue(), c.alpha()/4));
    rg.setColorAt( 0, c );
    rg.setColorAt( 0.5, c2 );
    rg.setColorAt( 1, Qt::transparent );
    p.setBrush( rg );
    p.setPen( Qt::NoPen );
    p.setRenderHint( QPainter::Antialiasing );
    p.drawEllipse( r );
    p.end();
    return pix;
}

class IconDelegate : public QStyledItemDelegate
{
public:
    enum ShadowPart { TopLeft = 0, Top, TopRight, Left, Center, Right, BottomLeft, Bottom, BottomRight };
    inline explicit IconDelegate( QWidget *parent ) : QStyledItemDelegate( parent )
    {
        m_size = 4;
        m_iv = static_cast<IconView*>( parent );
        genShadowData();
    }
    inline void genShadowData()
    {
        QPixmap shadow = shadowPix( ( m_size*2 )+1, 0, Qt::black );

        m_shadowData[TopLeft] = shadow.copy( 0, 0, m_size, m_size );
        m_shadowData[Top] = shadow.copy( m_size, 0, 1, m_size );
        m_shadowData[TopRight] = shadow.copy( m_size, 0, m_size, m_size );
        m_shadowData[Left] = shadow.copy( 0, m_size, m_size, 1 );
        m_shadowData[Center] = QPixmap(); //no center...
        m_shadowData[Right] = shadow.copy( shadow.width()-( m_size+1 ), m_size, m_size, 1 );
        m_shadowData[BottomLeft] = shadow.copy( 0, m_size, m_size, m_size );
        m_shadowData[Bottom] = shadow.copy( m_size, shadow.height()-( m_size+1 ), 1, m_size );
        m_shadowData[BottomRight] = shadow.copy( m_size, m_size, m_size, m_size );
    }
    inline void renderShadow( QRect rect, QPainter *painter ) const
    {
        const int x = rect.x(), y = rect.y(), r = rect.right(), b = rect.bottom(), w = rect.width(), h = rect.height();
        painter->drawTiledPixmap( QRect( rect.topLeft(), m_shadowData[TopLeft].size() ), m_shadowData[TopLeft] );
        painter->drawTiledPixmap( QRect( QPoint( x+m_size, y ), QSize( w-( m_size*2+1 ), m_shadowData[Top].height() ) ), m_shadowData[Top] );
        painter->drawTiledPixmap( QRect( QPoint( r-m_size, y ), m_shadowData[TopRight].size() ), m_shadowData[TopRight] );
        painter->drawTiledPixmap( QRect( QPoint( x, y+m_size ), QSize( m_shadowData[Left].width(), h-( m_size*2+1 ) ) ), m_shadowData[Left] );
        painter->drawTiledPixmap( QRect( QPoint( r-m_size, y+m_size ), QSize( m_shadowData[Right].width(), h-( m_size*2+1 ) ) ), m_shadowData[Right] );
        painter->drawTiledPixmap( QRect( QPoint( x, b-m_size ), m_shadowData[BottomLeft].size() ), m_shadowData[BottomLeft] );
        painter->drawTiledPixmap( QRect( QPoint( x+m_size, b-m_size ), QSize( w-( m_size*2+1 ), m_shadowData[Bottom].height() ) ), m_shadowData[Bottom] );
        painter->drawTiledPixmap( QRect( QPoint( r-m_size, b-m_size ), m_shadowData[BottomRight].size() ), m_shadowData[BottomRight] );
    }
    inline virtual void paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const
    {
        if ( !m_fsModel )
            return;

        painter->save();
        painter->setRenderHint( QPainter::Antialiasing );

        painter->setFont(qvariant_cast<QFont>(m_fsModel->data(index, Qt::FontRole)));

        const bool &selected = option.state & QStyle::State_Selected;
        const int &step = selected ? 8 : ViewAnimator::hoverLevel(m_iv, index);

        const QStyleOptionViewItem &copy = option;
        QRect saved = RECT, tr;
        QString et = elidedText(option, index, 0, &tr);
        tr.moveTop(RECT.y()+DECOSIZE.height());
        const_cast<QStyleOptionViewItem *>(&copy)->rect = tr;

        if ( step )
        {
            const_cast<QStyleOptionViewItem *>(&copy)->state |= QStyle::State_MouseOver; //grrrrr, must trick styles....
            painter->setOpacity((1.0f/8.0f)*step);
            QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &copy, painter, m_iv);
            painter->setOpacity(1);
        }
        const_cast<QStyleOptionViewItem *>(&copy)->rect = saved;

        QApplication::style()->drawItemText(painter, tr, textFlags(), PAL, option.state & QStyle::State_Enabled, et, selected ? QPalette::HighlightedText : QPalette::Text);

        const QImage &thumb( qvariant_cast<QImage>( index.data( FileSystemModel::Thumbnail ) ) );
        QIcon icon;
        if ( m_fsModel->fileInfo(index).isDir() )
            icon = m_fsModel->iconPix( m_fsModel->fileInfo( index ), DECOSIZE.width() );
        if ( icon.isNull() )
            icon = qvariant_cast<QIcon>( index.data( Qt::DecorationRole ) );


        const bool &isThumb = !thumb.isNull();
        QPixmap pixmap = icon.pixmap( isThumb && thumb.width() > thumb.height()+14 ? DECOSIZE.height()+14 : DECOSIZE.height() );
        QRect theRect = pixmap.rect();
        theRect.moveCenter( QPoint( RECT.center().rx(), RECT.y()+( DECOSIZE.height()/2 ) ) );
        if ( isThumb )
        {
            const int d = ( m_shadowData[TopLeft].width()/2 )+1;
            const QRect r( theRect.adjusted( -d, -d, d+1, ( d*2 )-1 ) );
            renderShadow( r, painter );
            painter->fillRect( theRect/*.adjusted( -1, -1, 1, 1 )*/, PAL.color( QPalette::Base ) );
        }
        QApplication::style()->drawItemPixmap(painter, theRect, Qt::AlignCenter, pixmap);

        painter->restore();
    }
    inline void setModel( FileSystemModel *fsModel ) { m_fsModel = fsModel; }
protected:
    inline QSize sizeHint( const QStyleOptionViewItem &option, const QModelIndex &index ) const
    {
        int ds = DECOSIZE.height();
        int h;
        elidedText(option, index, &h);
        return QSize(ds+(Configuration::config.views.iconView.textWidth*2), ds+h);
    }
    inline QString elidedText( const QStyleOptionViewItem &option, const QModelIndex &index, int *h = 0, QRect *r = 0 ) const
    {
        QFont font( qvariant_cast<QFont>(m_fsModel->data(index, Qt::FontRole)) );
        QFontMetrics fm( font );

        QTextLayout textLayout(TEXT, font);
        int widthUsed = 0, lineCount = 0, leading = fm.leading();
        textLayout.beginLayout();
        QTextOption opt;
        opt.setWrapMode(QTextOption::WrapAnywhere);
        textLayout.setTextOption(opt);

        float height = 0;
        while (++lineCount <= lines) {
            QTextLine line = textLayout.createLine();
            if (!line.isValid())
                break;

            line.setLineWidth(DECOSIZE.height()+Configuration::config.views.iconView.textWidth*2);
            height += leading;
            height += line.height();
            widthUsed += line.naturalTextWidth();
        }
        textLayout.endLayout();
        if ( h )
            *h = qRound(height);

        QString elidedText = lineCount > 3 ? fm.elidedText(TEXT, Qt::ElideRight, widthUsed) : TEXT;

        if ( r )
            *r = QApplication::style()->itemTextRect(fm, RECT, textFlags(), true, elidedText);
        return elidedText;
    }
    static inline int textFlags() { return Qt::AlignHCenter | Qt::AlignTop | Qt::TextWrapAnywhere | Qt::TextWordWrap; }
private:
    QPixmap m_shadowData[9];
    int m_size;
    IconView *m_iv;
    FileSystemModel *m_fsModel;
};

IconView::IconView( QWidget *parent ) : QListView( parent ), m_scrollTimer( new QTimer(this) ), m_delta(0), m_newSize(0), m_sizeTimer(0)
{
    setItemDelegate( new IconDelegate( this ) );
    ViewAnimator::manage(this);
    setMovement( QListView::Snap );
    const int &iSize = Configuration::config.views.iconView.iconSize*16;
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setResizeMode( QListView::Adjust );
    setIconSize( QSize( iSize, iSize ) );
    setGridSize( QSize( -1, iSize+60 ) );
    updateLayout();
    setWrapping( true );
    setEditTriggers( QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed );
    setSelectionRectVisible( true );
    setLayoutMode( QListView::SinglePass );
    setDragDropMode( QAbstractItemView::DragDrop );
    setDropIndicatorShown( true );
    setDefaultDropAction( Qt::MoveAction );
    viewport()->setAcceptDrops( true );
    setDragEnabled( true );
    setViewMode( QListView::IconMode );
    setAttribute( Qt::WA_Hover );
    setMouseTracking( true );
    setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    connect( m_scrollTimer, SIGNAL(timeout()), this, SLOT(scrollEvent()) );

    m_slide = false;
    m_startSlide = false;

    for ( int i = 16; i <= 256; i+=16 )
        m_allowedSizes << i;
}

void
IconView::wheelEvent( QWheelEvent * event )
{
    if ( event->orientation() == Qt::Vertical )
    {
        const int &numDegrees = event->delta();

        if ( event->modifiers() & Qt::CTRL )
        {
            m_newSize = numDegrees > 0 ? qMin( iconWidth() + 16, 256 ) : qMax( iconWidth() - 16, 16 );
            killTimer( m_sizeTimer );
            m_sizeTimer = startTimer( 50 );
        }
        else
        {
            if ( Configuration::config.views.iconView.smoothScroll )
            {
                m_delta += numDegrees;
                if ( !m_scrollTimer->isActive() )
                    m_scrollTimer->start( 25 );
            }
            else
                verticalScrollBar()->setValue( verticalScrollBar()->value()-( numDegrees ) );
        }
    }
    event->accept();
}

void
IconView::scrollEvent()
{
    if ( m_delta > 360 )
        m_delta = 360;
    else if ( m_delta < -360 )
        m_delta = -360;
    if ( m_delta != 0 )
        verticalScrollBar()->setValue( verticalScrollBar()->value()-( m_delta/8 ) );
    if ( m_delta > 0 ) //upwards
        m_delta -= 30;
    else if ( m_delta < 0 ) //downwards
        m_delta += 30;
    else
        m_scrollTimer->stop();
}

void
IconView::timerEvent(QTimerEvent *e)
{
    if ( e->timerId() == m_sizeTimer )
    {
        if ( iconWidth() > m_newSize ) //zoom out
            setIconWidth( iconWidth()-qMax(1, ((iconWidth()-m_newSize)/2)) );
        else if ( iconWidth() < m_newSize ) //zoom in
            setIconWidth( iconWidth()+qMax(1, ((m_newSize-iconWidth())/2)) );
        else
        {
            killTimer( m_sizeTimer );
            emit iconSizeChanged( m_newSize );
        }
    }
    else
        QListView::timerEvent(e);
}

void
IconView::keyPressEvent(QKeyEvent *event)
{
    if ( event->key() == Qt::Key_Return && event->modifiers() == Qt::NoModifier && state() != QAbstractItemView::EditingState )
    {
        if ( selectionModel()->selectedRows().count() )
            foreach ( const QModelIndex &index, selectionModel()->selectedRows() )
                emit activated(index);
        else if ( selectionModel()->selectedIndexes().count() )
            foreach ( const QModelIndex &index, selectionModel()->selectedIndexes() )
                emit activated(index);
    }
    QListView::keyPressEvent(event);
}

void
IconView::resizeEvent( QResizeEvent *e )
{
    QListView::resizeEvent( e );
    updateLayout();
}

void
IconView::mouseMoveEvent( QMouseEvent *event )
{
    int newVal = event->pos().x() > m_startPos.x() ? 16 : -16;
    bool shouldSlide = event->pos().x() > m_startPos.x() + 64 || event->pos().x() < m_startPos.x() - 64;
    newVal += iconSize().width();

    if ( shouldSlide && m_startSlide && m_allowedSizes.contains( newVal ) )
    {
        m_slide = true;
        emit iconSizeChanged( newVal );
        m_startPos = event->pos();
    }

    QListView::mouseMoveEvent( event );
}

void
IconView::mousePressEvent( QMouseEvent *event )
{
    if ( event->button() == Qt::MiddleButton )
    {
        m_startSlide = true;
        m_startPos = event->pos();
        setDragEnabled( false );   //we will likely not want to drag items around at this point...
        event->accept();
        return;
    }
    viewport()->update(); //required for itemdelegate toggling between bold / normal font.... just update( index ) doesnt work.
    QListView::mousePressEvent( event );
}

void
IconView::mouseReleaseEvent( QMouseEvent *e )
{
    if ( e->button() == Qt::MiddleButton )
    {
        if ( indexAt( e->pos() ).isValid() && e->pos() == m_startPos )
        {
            emit newTabRequest( indexAt( e->pos() ) );
            e->accept();
        }
        if ( m_startSlide )
        {
            m_slide = false;
            m_startSlide = false;
            setDragEnabled( true );
            viewport()->update();
            this->update();
            e->accept();
            return;
        }
    }
    QListView::mouseReleaseEvent( e );
    viewport()->update(); //required for itemdelegate toggling between bold / normal font.... just update( index ) doesnt work.
}

void
IconView::paintEvent( QPaintEvent *e )
{
    QListView::paintEvent( e );
    if ( m_slide )
    {
        QString sizeString = QString::number( iconSize().width() ) + " px";
        QPainter p( viewport() );
        QFont font = p.font();
        font.setBold( true );
        font.setPointSize( 32 );
        p.setFont( font );
        QRect rect;
        rect.setSize( QFontMetrics( font ).boundingRect( sizeString ).size() );
        rect.moveBottomRight( viewport()->rect().bottomRight() );
        p.drawText( rect, sizeString );
        p.end();
    }
    return;
}

void
IconView::updateLayout()
{
    int horMargin = style()->pixelMetric( QStyle::PM_ScrollBarExtent, 0, horizontalScrollBar() ) + qMax( 1, style()->pixelMetric( QStyle::PM_DefaultFrameWidth )*3 );
    int contentsWidth = viewport()->width() - horMargin;
    int horItemCount = contentsWidth/( iconSize().width() + 60 );
    if ( horItemCount == 0 || contentsWidth == 0 )
    {
        QTimer::singleShot( 200, this, SLOT( updateLayout() ) );  //try again later
        return;
    }
    if ( rootIndex().isValid() && model()->rowCount( rootIndex() ) < horItemCount && model()->rowCount( rootIndex() ) > 1 )
        horItemCount = model()->rowCount( rootIndex() );

    setGridSize( QSize( contentsWidth/horItemCount, gridSize().height() ) );
}

void
IconView::setFilter( QString filter )
{
    for( int i = 0; i < model()->rowCount( rootIndex() ); i++ )
    {
        if ( model()->index( i, 0, rootIndex() ).data().toString().toLower().contains( filter.toLower() ) )
            setRowHidden( i, false );
        else
            setRowHidden( i, true );
    }
}

void
IconView::contextMenuEvent( QContextMenuEvent *event )
{
    QMenu popupMenu;
    if ( Configuration::customActions().count() )
        popupMenu.addMenu(Configuration::customActionsMenu());
    popupMenu.addActions( actions() );
    const QString &file = static_cast<FileSystemModel *>( model() )->filePath( indexAt( event->pos() ) );
    QMenu openWith( tr( "Open With" ), this );
    openWith.addActions( Configuration::openWithActions( file ) );
    foreach( QAction *action, actions() )
    {
        popupMenu.addAction( action );
        if ( action->objectName() == "actionDelete" )
            popupMenu.insertSeparator( action );
        if ( action->objectName() == "actionCustomCmd" )
            popupMenu.insertMenu( action, &openWith );
    }
    popupMenu.exec( event->globalPos() );
}

void
IconView::setModel( QAbstractItemModel *model )
{
    QListView::setModel( model );
    if ( m_fsModel = qobject_cast<FileSystemModel*>( model ) )
    {
        connect( m_fsModel, SIGNAL( rootPathChanged( QString ) ), this,
                SLOT( rootPathChanged( QString ) ) );
        connect( m_fsModel, SIGNAL( directoryLoaded( QString ) ), this,
                SLOT( rootPathChanged( QString ) ) );
        static_cast<IconDelegate*>( itemDelegate() )->setModel( m_fsModel );
    }
}

void
IconView::rootPathChanged( QString path )
{
    updateLayout();
}

void
IconView::setIconWidth( const int width )
{
    QListView::setIconSize( QSize( width, width ) );
    setGridSize( QSize( gridSize().width(), iconSize().height()+60 ) );
    updateLayout();
}
