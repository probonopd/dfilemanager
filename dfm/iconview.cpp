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
#include <QRect>
#include <qmath.h>
#include <QTextEdit>
#include <QMessageBox>
#include <QTextCursor>
#include <QTextFormat>
#include <QDesktopServices>

#define TEXT index.data().toString()
#define RECT option.rect
#define FM option.fontMetrics
#define PAL option.palette
#define DECOSIZE option.decorationSize

//Q_DECLARE_METATYPE(QPainterPath)

using namespace DFM;

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

class IconDelegate : public FileItemDelegate
{
public:
    enum ShadowPart { TopLeft = 0, Top, TopRight, Left, Center, Right, BottomLeft, Bottom, BottomRight };
    enum Role { Text = 0, TextRect, Shape, Size };
    inline explicit IconDelegate( IconView *parent )
        : FileItemDelegate( parent )
        , m_size(4)
        , m_iv(parent)
    {
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
    void setEditorData(QWidget *editor, const QModelIndex &index) const
    {
        FileItemDelegate::setEditorData(editor, index);
        static_cast<QTextEdit *>(editor)->setAlignment(Qt::AlignCenter);
    }
    void paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const
    {
        if ( !m_model || !index.isValid() || !option.rect.isValid() )
            return;

        painter->save();
        painter->setRenderHint( QPainter::Antialiasing );

        const bool selected = option.state & QStyle::State_Selected;
        const int step = selected ? 8 : ViewAnimator::hoverLevel(m_iv, index);

        if ( step )
        {
            QColor h = selected?PAL.color(QPalette::Highlight):Ops::colorMid(PAL.color(QPalette::Highlight), PAL.color(QPalette::Base));
            h.setAlpha(255/8*step);
//            painter->fillRect(RECT.adjusted(1, 1, 0, 0), h);
            painter->setPen(Qt::NoPen);
            painter->setBrush(h);
            painter->drawRoundedRect(RECT.adjusted(1, 1, 0, 0), 3, 3);
        }
        const QColor &high(selected ? PAL.color(QPalette::HighlightedText) : PAL.color(QPalette::Text));
        QPen pen = painter->pen();
        painter->setPen(high);
        painter->setFont(index.data(Qt::FontRole).value<QFont>());
        painter->drawText(RECT.adjusted(0, DECOSIZE.height()+4, 0, 0), Qt::AlignTop|Qt::AlignHCenter, text(option, index));
        painter->setPen(pen);
//        QApplication::style()->drawItemText(painter, tr.adjusted(-2, 0, 2, 0), Qt::AlignCenter, PAL, option.state & QStyle::State_Enabled, et, high);

        QIcon icon = m_model->fileIcon(index);

        const bool isThumb = m_model->hasThumb(m_model->filePath(index));

        int newSize = icon.actualSize(DECOSIZE).height();
        if ( !isThumb && icon.actualSize(DECOSIZE).height() < DECOSIZE.height() )
        {
            QList<int> il;
            for ( int i = 0; i < icon.availableSizes().count(); ++i )
                il << icon.availableSizes().at(i).height();

            if ( il.count() > 1 )
                qSort(il);

            int i = -1;

            while ( newSize < DECOSIZE.height() && ++i<il.count() )
                newSize = il.at(i);
        }

        QPixmap pixmap = icon.pixmap(newSize);
        if ( pixmap.height() > DECOSIZE.height() && !isThumb )
            pixmap = pixmap.scaledToHeight(DECOSIZE.height(), Qt::SmoothTransformation);
        else if ( isThumb && pixmap.width() > pixmap.height() )
            pixmap = icon.pixmap( qMin<int>(RECT.width()-4, ((float)DECOSIZE.height()*((float)pixmap.width()/(float)pixmap.height()))-4 ));
        QRect theRect = pixmap.rect();
        theRect.moveCenter( QPoint( RECT.center().x(), RECT.y()+2+( DECOSIZE.height()/2 ) ) );
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
    inline void setModel( FileSystemModel *fsModel ) { m_model = fsModel; }
protected:
    QSize sizeHint( const QStyleOptionViewItem &option, const QModelIndex &index ) const
    {
        return m_iv->gridSize();
    }
    QString text( const QStyleOptionViewItem &option, const QModelIndex &index ) const
    {
        if ( !index.isValid() )
            return QString();
        QFontMetrics fm( option.fontMetrics );

        QString spaces(TEXT.replace(".", QString(" ")));
        spaces = spaces.replace("_", QString(" "));
        QString theText;

        QTextLayout textLayout(spaces, option.font);
        int lineCount = -1;
        QTextOption opt;
        opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        textLayout.setTextOption(opt);

        const int w = RECT.width();

        textLayout.beginLayout();
        while (++lineCount < Store::config.views.iconView.lineCount)
        {
            QTextLine line = textLayout.createLine();
            if (!line.isValid())
                break;

            line.setLineWidth(w);
            QString actualText;
            actualText = TEXT.mid(line.textStart(), qMin(line.textLength(), TEXT.count()));
            if ( line.lineNumber() == Store::config.views.iconView.lineCount-1 )
                actualText = fm.elidedText(TEXT.mid(line.textStart(), TEXT.count()), Qt::ElideRight, w);

            //this should only happen if there
            //are actual dots or underscores...
            //so width should always be > oldw
            //we do this only cause QTextLayout
            //doesnt think that dots or underscores
            //are actual wordseparators
            if ( fm.boundingRect(actualText).width() > w )
            {
                int width = 0;
                if ( actualText.contains(".") )
                    width += fm.boundingRect(".").width()*actualText.count(".");
                if ( actualText.contains("_") )
                    width += fm.boundingRect("_").width()*actualText.count("_");

                int oldw = fm.boundingRect(" ").width()*actualText.count(" ");
                int diff = width - oldw;

                line.setLineWidth(w-diff);
                actualText = TEXT.mid(line.textStart(), qMin(line.textLength(), TEXT.count()));
                if ( line.lineNumber() == Store::config.views.iconView.lineCount-1 )
                    actualText = fm.elidedText(TEXT.mid(line.textStart(), TEXT.count()), Qt::ElideRight, w);
            }

            theText.append(QString("%1\n").arg(actualText));
        }
        textLayout.endLayout();
        return theText;
    }
    static inline int textFlags() { return Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap; }
private:
    QPixmap m_shadowData[9];
    int m_size;
    IconView *m_iv;
    FileSystemModel *m_model;
};

IconView::IconView( QWidget *parent )
    : QAbstractItemView( parent )
    , m_scrollTimer( new QTimer(this) )
    , m_delta(0)
    , m_newSize(0)
    , m_sizeTimer(new QTimer(this))
    , m_gridHeight(0)
    , m_container(static_cast<ViewContainer *>(parent))
    , m_layTimer(new QTimer(this))
{
    setItemDelegate( new IconDelegate( this ) );
//    setUniformItemSizes(true);
//    setMovement( QAbstractItemView::Snap );
    const int iSize = Store::config.views.iconView.iconSize*16;
    setGridHeight(iSize);
    setSelectionMode( QAbstractItemView::ExtendedSelection );
//    setResizeMode( QAbstractItemView::Adjust );
    setIconSize( QSize( iSize, iSize ) );
    updateLayout();
//    setWrapping( true );
    setEditTriggers( QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed );
//    setSelectionRectVisible( true );
    setDragDropMode( QAbstractItemView::DragDrop );
    setDropIndicatorShown( true );
    setDefaultDropAction( Qt::MoveAction );
    viewport()->setAcceptDrops( true );
    setDragEnabled( true );
//    setViewMode( QAbstractItemView::IconMode );
    setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
    setSelectionBehavior(QAbstractItemView::SelectRows);

    ViewAnimator::manage(this);
    connect( verticalScrollBar(), SIGNAL(valueChanged(int)), viewport(), SLOT(update()) );
//    connect( ViewAnimator::manage(this), SIGNAL(animation()), verticalScrollBar(), SLOT(update()) );

    connect( m_scrollTimer, SIGNAL(timeout()), this, SLOT(scrollEvent()) );
    connect( this, SIGNAL(iconSizeChanged(int)), this, SLOT(setGridHeight(int)) );
    connect( MainWindow::currentWindow(), SIGNAL(settingsChanged()), this, SLOT(correctLayout()) );
    connect( m_sizeTimer, SIGNAL(timeout()), this, SLOT(updateIconSize()) );

    connect( m_layTimer, SIGNAL(timeout()), this, SLOT(calculateRects()) );

    m_slide = false;
    m_startSlide = false;

    for ( int i = 16; i <= 256; i+=16 )
        m_allowedSizes << i;

//    QDir storage(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
//    if ( storage.exists() && storage.isAbsolute() )
//        m_homePix.load(storage.absoluteFilePath("home.png"));
}

void
IconView::setNewSize(const int size)
{
    m_newSize = size;
    m_firstIndex = firstValidIndex();
    if ( !m_sizeTimer->isActive() )
        m_sizeTimer->start(25);
}

void
IconView::updateIconSize()
{
    if ( iconWidth() > m_newSize ) //zoom out
        setIconWidth( iconWidth()-qMax(1, ((iconWidth()-m_newSize)/2)) );
    else if ( iconWidth() < m_newSize ) //zoom in
        setIconWidth( iconWidth()+qMax(1, ((m_newSize-iconWidth())/2)) );
    else
    {
        emit iconSizeChanged( m_newSize );
        m_sizeTimer->stop();
    }
}

void
IconView::wheelEvent( QWheelEvent * event )
{
    if ( event->orientation() == Qt::Vertical )
    {
        const int numDegrees = event->delta();

        if ( event->modifiers() & Qt::CTRL )
        {
            QSlider *s = MainWindow::window(this)->iconSizeSlider();
            s->setValue(s->value()+(numDegrees>0?1:-1));
        }
        else
        {
            if ( Store::config.views.iconView.smoothScroll )
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
IconView::showEvent(QShowEvent *e)
{
    QAbstractItemView::showEvent(e);
    updateLayout();
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
IconView::keyPressEvent(QKeyEvent *event)
{
    if ( event->key() == Qt::Key_Escape )
        clearSelection();
    if ( event->key() == Qt::Key_Return
         && event->modifiers() == Qt::NoModifier
         && state() == NoState )
    {
        if ( selectionModel()->selectedIndexes().count() )
            foreach ( const QModelIndex &index, selectionModel()->selectedIndexes() )
                if ( !index.column() )
                    emit activated(index);
        event->accept();
        return;
    }
    QAbstractItemView::keyPressEvent(event);
}

void
IconView::resizeEvent( QResizeEvent *e )
{
    QAbstractItemView::resizeEvent( e );
    if ( e->size().width() != e->oldSize().width() )
        updateLayout();
    calculateRects();
//    verticalScrollBar()->resize(12, height());
//    verticalScrollBar()->move(width()-12, 0);
    verticalScrollBar()->setPageStep(viewport()->height());
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

    QAbstractItemView::mouseMoveEvent( event );
    if (m_pressPos != QPoint())
        viewport()->update();
}

void
IconView::mousePressEvent( QMouseEvent *event )
{
    m_pressPos = event->pos();
    QAbstractItemView::mousePressEvent( event );

    if (!selectionModel() || (state() == EditingState))
        return;

    if ( event->button() == Qt::MiddleButton )
    {
         m_startPos = event->pos();
        m_startSlide = true;
        setDragEnabled( false );   //we will likely not want to drag items around at this point...
        event->accept();
        return;
    }
    viewport()->update(); //required for itemdelegate toggling between bold / normal font.... just update( index ) doesnt work.
}

void
IconView::mouseReleaseEvent( QMouseEvent *e )
{
    m_pressPos = QPoint();
    const QModelIndex &index = indexAt(e->pos());

    if ( Store::config.views.singleClick
         && !e->modifiers()
         && e->button() == Qt::LeftButton
         && m_startPos == e->pos()
         && state() == NoState )
    {
        emit activated(index);
        e->accept();
        return;
    }
    if ( e->button() == Qt::MiddleButton )
    {
        if ( e->pos() == m_startPos
             && !e->modifiers() )
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
            e->accept();
            m_startPos = QPoint();
            return;
        }
    }

    QAbstractItemView::mouseReleaseEvent( e );
    viewport()->update(); //required for itemdelegate toggling between bold / normal font.... just update( index ) doesnt work.
}

void
IconView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (!Store::config.views.singleClick)
        QAbstractItemView::mouseDoubleClickEvent(event);
}

QStyleOptionViewItem
IconView::viewOptions() const
{
    QStyleOptionViewItemV4 option(QAbstractItemView::viewOptions());
    option.widget=this;
//    option.decorationAlignment=Qt::AlignHCenter|Qt::AlignTop;
    option.decorationPosition=QStyleOptionViewItem::Top;
//    option.displayAlignment=Qt::AlignHCenter|Qt::AlignTop;
    return option;
}

void
IconView::paintEvent( QPaintEvent *e )
{
    QPainter p(viewport());
    if (isCategorized())
    {
        const QStringList &categories(m_model->categories());
        QFont f(font());
        f.setBold(true);
        QFontMetrics fm(f);
        for (int cat = 0; cat < categories.count(); ++cat)
        {
            const QRect catRect = visualRect(categories.at(cat));
            p.fillRect(catRect, QColor(0,0,0,cat&1?32:16));
            p.setPen(QColor(255,255,255,64));
            p.drawLine(catRect.topLeft(), catRect.topRight());
            p.setPen(QColor(0,0,0,64));
            p.drawLine(catRect.bottomLeft(), catRect.bottomRight());
            p.setPen(Ops::colorMid(palette().color(QPalette::Base), palette().color(QPalette::Text)));
            p.save();
            p.setFont(f);
            p.drawText(QRect(catRect.topLeft(), fm.boundingRect(categories.at(cat)).size()), categories.at(cat));
            p.restore();
            const QModelIndexList &block(m_model->category(categories.at(cat)));
            for (int i = 0; i < block.count(); ++i)
            {
                const QModelIndex &index(block.at(i));
                const QRect vr(visualRect(index));
                if (!viewport()->rect().intersects(vr))
                    continue;
                QStyleOptionViewItemV4 option(viewOptions());
                option.rect=vr;
                if (selectionModel()->isSelected(index))
                    option.state|=QStyle::State_Selected;
                itemDelegate()->paint(&p, option, index);
            }
        }
    }
    else
    {
        for (int i = 0; i < m_model->rowCount(rootIndex()); ++i)
        {
            const QModelIndex &index(m_model->index(i, 0, rootIndex()));
            const QRect vr(visualRect(index));
            if (!viewport()->rect().intersects(vr))
                continue;
            QStyleOptionViewItemV4 option(viewOptions());
            option.rect=vr;
            option.widget=this;
            if (selectionModel()->isSelected(index))
                option.state|=QStyle::State_Selected;
            itemDelegate()->paint(&p, option, index);
        }
    }
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
    }
    if (m_pressPos != QPoint())
    {
        QStyleOptionRubberBand opt;
        opt.initFrom(this);
        opt.shape = QRubberBand::Rectangle;
        opt.opaque = false;
        opt.rect = QRect(m_pressPos, mapFromGlobal(QCursor::pos()));
        style()->drawControl(QStyle::CE_RubberBand, &opt, &p);
    }
    p.end();
}

void
IconView::setGridHeight(int gh)
{
    m_gridHeight = gh + 4 + fontMetrics().height()*Store::config.views.iconView.lineCount;
}

void
IconView::correctLayout()
{
    m_gridHeight = iconSize().height() + 4 + fontMetrics().height()*Store::config.views.iconView.lineCount;
    updateLayout();
}

void
IconView::updateLayout()
{
    if ( !isVisible() )
        return;

    QStyleOptionFrame opt;
    opt.initFrom(viewport());
    int horMargin = (width()-viewport()->width())+style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &opt, viewport());
    int contentsWidth = width()-horMargin;
    int horItemCount = m_horItems = contentsWidth/( iconSize().width() + Store::config.views.iconView.textWidth*2 );
    if ( model()->rowCount( rootIndex() ) < horItemCount && model()->rowCount( rootIndex() ) > 1 && !isCategorized() )
        horItemCount = model()->rowCount( rootIndex() );
    if ( contentsWidth && horItemCount )
        setGridSize( QSize( contentsWidth/horItemCount, m_gridHeight ) );
}

void
IconView::contextMenuEvent( QContextMenuEvent *event )
{
    QMenu popupMenu;
    if ( Store::customActions().count() )
        popupMenu.addMenu(Store::customActionsMenu());
    popupMenu.addActions( actions() );
    const QString &file = static_cast<FileSystemModel *>( model() )->filePath( indexAt( event->pos() ) );
    QMenu openWith( tr( "Open With" ), this );
    openWith.addActions( Store::openWithActions( file ) );
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
    QAbstractItemView::setModel( model );
    if ( m_model = qobject_cast<FileSystemModel *>( model ) )
    {
        connect( m_model, SIGNAL( directoryLoaded(QString)), this, SLOT( rootPathChanged( QString ) ) );
        connect( m_model, SIGNAL( rootPathChanged(QString)), this, SLOT( rootPathChanged( QString ) ) );
        connect( m_model, SIGNAL(sortingChanged(int,int)), this, SLOT(calculateRects()) );
        static_cast<IconDelegate*>( itemDelegate() )->setModel( m_model );
    }
}

void
IconView::rootPathChanged( const QString &path )
{
    m_layTimer->start(20);
    updateLayout();
#if 0
    QIcon icon = m_model->fileIcon(m_model->index(path));
    if ( icon.name() != "inode-directory" )
    {
        m_bgPix[0] = icon.pixmap(256);
        m_bgPix[1] = icon.pixmap(256);
        QTransform r;
        r.translate(m_bgPix[1].width()/2, m_bgPix[1].height()/3);
        r.rotate(75, Qt::YAxis);
        r.scale(2.0, 1.0);
        r.translate(-m_bgPix[1].width()/2, -m_bgPix[1].height()/3);
        m_bgPix[1] = m_bgPix[1].transformed(r, Qt::SmoothTransformation);


        QTransform l;
        l.translate(m_bgPix[0].width()/2, m_bgPix[0].height()/3);
        l.rotate(-75, Qt::YAxis);
        l.scale(2.0, 1.0);
        l.translate(-m_bgPix[0].width()/2, -m_bgPix[0].height()/3);
        m_bgPix[0] = m_bgPix[0].transformed(l, Qt::SmoothTransformation);
    }
    else
    {
        m_bgPix[0] = QPixmap();
        m_bgPix[1] = QPixmap();
    }
#endif
}

QModelIndex
IconView::firstValidIndex()
{
    int i = iconSize().width()/2;
    QModelIndex index;
    while ( !index.isValid() && ++i < qMin(width(), height()) )
        index = indexAt(QPoint(i, i));
    return index;
}

void
IconView::setIconWidth( const int width )
{
//    float val = 0;
//    if ( verticalScrollBar()->isVisible() )
//        val = (float)verticalScrollBar()->value()/(float)verticalScrollBar()->maximum();
    QAbstractItemView::setIconSize( QSize( width, width ) );
    setGridHeight(width);
    setGridSize( QSize( gridSize().width(), m_gridHeight ) );
    if ( m_firstIndex.isValid() /*&& width == m_newSize */)
        scrollTo(m_firstIndex, QAbstractItemView::PositionAtTop);
//    if ( val && verticalScrollBar()->isVisible() )
//        verticalScrollBar()->setValue(verticalScrollBar()->maximum()*val);

    updateLayout();
}

QRect
IconView::visualRect(const QModelIndex &index) const
{
    if (!index.isValid()||index.column()||!m_horItems||!m_rects.contains(index.internalPointer()))
        return QRect();
    return m_rects.value(index.internalPointer(), QRect()).translated(0, -verticalOffset());
}

QRect
IconView::visualRect(const QString &cat) const
{
    if ( m_catRects.contains(cat) )
        return m_catRects.value(cat, QRect()).translated(0, -verticalOffset());
    return QRect();
}

QModelIndex
IconView::indexAt(const QPoint &p) const
{
    if (m_scrollTimer->isActive()||m_sizeTimer->isActive()||m_layTimer->isActive())
        return QModelIndex();
    for (int i = 0; i < m_model->rowCount(rootIndex()); ++i)
        if (visualRect(m_model->index(i, 0, rootIndex())).contains(p))
            return m_model->index(i, 0, rootIndex());
    return QModelIndex();
}

void
IconView::scrollTo(const QModelIndex &index, ScrollHint hint)
{
    if (!index.isValid())
        return;
    const QRect r(visualRect(index));
    if (viewport()->rect().intersects(r)&&hint==QAbstractItemView::EnsureVisible)
        return;
    if (hint==QAbstractItemView::PositionAtTop)
        verticalScrollBar()->setValue(r.top());
    else if (hint==QAbstractItemView::PositionAtBottom)
        verticalScrollBar()->setValue(r.top()+(viewport()->height()-r.height()));
    else if (hint==QAbstractItemView::PositionAtCenter)
        verticalScrollBar()->setValue(r.top()+(viewport()->height()/2-r.height()));
}

bool
IconView::isCategorized() const
{
    return Store::config.views.iconView.categorized;
}

void
IconView::rowsInserted(const QModelIndex &parent, int start, int end)
{
//    QAbstractItemView::rowsInserted(parent, start, end);
//    if (!m_layTimer->isActive())
//        m_layTimer->start(20);
}

void
IconView::calculateRects()
{
    m_rects.clear();
    m_catRects.clear();
    const int hsz = gridSize().width();
    const int vsz = gridSize().height();
    int h = -vsz;

    if (isCategorized())
    {
        const QStringList &categories(m_model->categories());
        QFont f(font());
        f.setBold(true);
        QFontMetrics fm(f);
        for (int cat = 0; cat < categories.count(); ++cat)
        {
            h+=vsz;
            int starth = h;

            h+=fm.boundingRect(categories.at(cat)).height();
            int row = -1;
            const QModelIndexList &block(m_model->category(categories.at(cat)));
            for (int i = 0; i < block.count(); ++i)
            {
                if (row+1==m_horItems)
                {
                    h+=vsz;
                    row=0;
                }
                else
                    ++row;
                const QModelIndex &index(block.at(i));
                if (index.isValid()&&index.internalPointer())
                    m_rects.insert(index.internalPointer(), QRect(hsz*row, h, hsz, vsz));
            }
            m_catRects.insert(categories.at(cat), QRect(0, starth, viewport()->width(), (h+vsz)-starth));
        }
    }
    else
    {
        for (int i = 0; i < m_model->rowCount(rootIndex()); ++i)
        {
            const QModelIndex &index(m_model->index(i, 0, rootIndex()));
            const int col = i % m_horItems;
            h = vsz * (i / m_horItems);
            if (index.isValid()&&index.internalPointer())
                m_rects.insert(index.internalPointer(), QRect(hsz*col, h, hsz, vsz));
        }
    }
    h+=vsz;
    if (h>viewport()->height())
    {
//        verticalScrollBar()->show();
        verticalScrollBar()->setRange(0, h-viewport()->height());
    }
    else
    {
        verticalScrollBar()->setRange(0, -1);
//        verticalScrollBar()->hide();
    }
    viewport()->update();
    if (!m_model->isPopulating())
        m_layTimer->stop();
}

int
IconView::horizontalOffset() const
{
    return 0;
}

bool
IconView::isIndexHidden(const QModelIndex & index) const
{
    return false;
}

QModelIndex
IconView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    //TODO ..or not?
    return QModelIndex();
}

void
IconView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags flags)
{
    QItemSelection selection;
    for (int i = 0; i < m_model->rowCount(rootIndex()); ++i)
    {
        const QModelIndex &index(m_model->index(i, 0, rootIndex()));
        const QRect vr(visualRect(index));
        if (rect.intersects(vr))
            selection.select(index, index);
    }
    selectionModel()->select(selection, flags);
}

int
IconView::verticalOffset() const
{
    return verticalScrollBar()->value();
}

QRegion
IconView::visualRegionForSelection(const QItemSelection &selection) const
{
    QRegion region;
    foreach (const QModelIndex &index, selection.indexes())
        region += visualRect(index);
    return region;
}




//------------------------------------------------------------------------------------------------------------

GayBar::GayBar(const Qt::Orientation ori, QWidget *parent, QWidget *vp)
    :QScrollBar(ori, parent)
    ,m_viewport(vp)
    ,m_iv(static_cast<IconView *>(parent)){}

void
GayBar::paintEvent(QPaintEvent *)
{
    return;
    setAttribute(Qt::WA_TranslucentBackground);
//    QPixmap pix(rect().size());
//    pix.fill(Qt::transparent);

//    m_viewport->render(this);
    QPainter p(this);
    p.drawTiledPixmap(rect(), m_iv->bgPix(), QPoint(-(m_viewport->width()-12), 0));
//    m_viewport->render(&pix, QPoint(), QRegion(m_viewport->width()-12, 0, 12, m_viewport->height()), QWidget::DrawWindowBackground);
//    p.drawTiledPixmap(rect(), pix);
    QStyleOptionSlider opt;
    initStyleOption(&opt);

//    const QRect &groove = style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarGroove, this);
    const QRect &slider = style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarSlider, this).adjusted(1, 1, -1, -1);

    const int r = qCeil(slider.width()/2.0f);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(palette().color(QPalette::Text));
    p.setOpacity(0.5f);
    p.drawRoundedRect(slider,r,r);
    p.end();
}
