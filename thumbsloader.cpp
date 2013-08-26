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


#include "thumbsloader.h"
#include <QImage>
#include "columnsview.h"
#include "mainwindow.h"
#include "operations.h"
#include "config.h"

QHash<QString, QImage> ThumbsLoader::m_loadedThumbs[4];
QStringList ThumbsLoader::m_queue;
QList<QPair<QImage, QModelIndex> > ThumbsLoader::m_imgQueue;
QFileSystemWatcher *ThumbsLoader::m_fsWatcher = 0;
DFM::FileSystemModel *ThumbsLoader::m_fsModel = 0;
QTimer *ThumbsLoader::m_timer = 0;

static QImageReader ir;
static QRect vr;

 /* blurring function below from:
  * http://stackoverflow.com/questions/3903223/qt4-how-to-blur-qpixmap-image
  * unclear to me who wrote it.
  */
static QImage blurred(const QImage& image, const QRect& rect, int radius, bool alphaOnly = false)
{
    int tab[] = { 14, 10, 8, 6, 5, 5, 4, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2 };
    int alpha = (radius < 1)  ? 16 : (radius > 17) ? 1 : tab[radius-1];

    QImage result = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    int r1 = rect.top();
    int r2 = rect.bottom();
    int c1 = rect.left();
    int c2 = rect.right();

    int bpl = result.bytesPerLine();
    int rgba[4];
    unsigned char* p;

    int i1 = 0;
    int i2 = 3;

    if (alphaOnly)
        i1 = i2 = (QSysInfo::ByteOrder == QSysInfo::BigEndian ? 0 : 3);

    for (int col = c1; col <= c2; col++) {
        p = result.scanLine(r1) + col * 4;
        for (int i = i1; i <= i2; i++)
            rgba[i] = p[i] << 4;

        p += bpl;
        for (int j = r1; j < r2; j++, p += bpl)
            for (int i = i1; i <= i2; i++)
                p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
    }

    for (int row = r1; row <= r2; row++) {
        p = result.scanLine(row) + c1 * 4;
        for (int i = i1; i <= i2; i++)
            rgba[i] = p[i] << 4;

        p += 4;
        for (int j = c1; j < c2; j++, p += 4)
            for (int i = i1; i <= i2; i++)
                p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
    }

    for (int col = c1; col <= c2; col++) {
        p = result.scanLine(r2) + col * 4;
        for (int i = i1; i <= i2; i++)
            rgba[i] = p[i] << 4;

        p -= bpl;
        for (int j = r1; j < r2; j++, p -= bpl)
            for (int i = i1; i <= i2; i++)
                p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
    }

    for (int row = r1; row <= r2; row++) {
        p = result.scanLine(row) + c2 * 4;
        for (int i = i1; i <= i2; i++)
            rgba[i] = p[i] << 4;

        p -= 4;
        for (int j = c1; j < c2; j++, p -= 4)
            for (int i = i1; i <= i2; i++)
                p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
    }

    return result;
}

static QImage flowImg( const QImage &image )
{
    QImage p(SIZE, SIZE, QImage::Format_ARGB32);
    p.fill(Qt::transparent);
    QPainter pt(&p);
    QRect r = image.rect();
    r.moveCenter(p.rect().center());
    r.moveBottom(p.rect().bottom()-1);
    pt.setBrushOrigin(r.topLeft());
    pt.fillRect(r, image);
    pt.end();
    return p;
}

static QImage reflection( const QImage &img = QImage() )
{
    QImage refl(SIZE, SIZE, QImage::Format_ARGB32);
    refl.fill( Qt::transparent);
    QRect r = img.rect();
    r.moveCenter(refl.rect().center());
    r.moveTop(refl.rect().top());
    QPainter p(&refl);
    p.setBrushOrigin(r.topLeft());
    if ( !img.isNull() )
        p.fillRect(r, img.mirrored());
    p.end();
    int size = refl.width() * refl.height();
    QRgb *pixel = reinterpret_cast<QRgb *>(refl.bits());
    QColor bg = DFM::Operations::colorMid(qApp->palette().color(QPalette::Highlight), Qt::black);
    bg.setHsv(bg.hue(), qMin(64, bg.saturation()), bg.value(), bg.alpha());
    for ( int i = 0; i < size; ++i )
        if ( qAlpha(pixel[i]) )
        {
            QColor c = QColor(pixel[i]);
            c = DFM::Operations::colorMid(c, bg, 1, 4);
            pixel[i] = qRgba(c.red(), c.green(), c.blue(), qAlpha(pixel[i]));
        }
    return blurred( refl, refl.rect(), 5 );
}

ThumbsLoader *inst = 0;

ThumbsLoader::ThumbsLoader(QObject *parent) :
    QThread(parent),
    m_extent(256),
    m_currentView(0){}


ThumbsLoader
*ThumbsLoader::instance()
{
    if ( !inst )
    {
        inst = new ThumbsLoader(qApp);
        m_fsWatcher = new QFileSystemWatcher(inst);
        m_timer = new QTimer(inst);
        m_timer->setInterval(200);
        connect ( m_fsWatcher, SIGNAL(fileChanged(QString)), inst, SLOT(fileChanged(QString)) );
        connect ( m_timer, SIGNAL(timeout()), inst, SLOT(loadThumbs()) );
    }
    return inst;
}

void
ThumbsLoader::loadThumb( const QString &path )
{
    if ( !m_currentView || !m_fsModel )
        return;
    if ( !QFileInfo(path).exists() )
    {
        for ( int i = 0; i<3; ++i )
            m_loadedThumbs[i].remove(path);
        m_fsWatcher->removePath(path);
        return;
    }
    ir.setFileName(path);
    if ( !ir.canRead() || path.endsWith( "xcf", Qt::CaseInsensitive ) )
        return;

    const QModelIndex &index = m_fsModel->index(path);
    if ( vr.intersects(m_currentView->visualRect(index)) )
    {
        QSize thumbsize = ir.size();
        if ( qMax( thumbsize.width(), thumbsize.height() ) > m_extent )
            thumbsize.scale( m_extent, m_extent, Qt::KeepAspectRatio );
        ir.setScaledSize(thumbsize);

        const QImage image(ir.read());

        if ( !image.isNull() )
        {
            if ( !m_fsWatcher->files().contains(path) )
                m_fsWatcher->addPath(path);
            m_loadedThumbs[Thumb].insert(path, image);
            m_loadedThumbs[Reflection].insert(path, reflection(image));
            m_loadedThumbs[FlowPic].insert(path, flowImg(image));
            emit dataChanged(index, index);
        }
    }
}

void
ThumbsLoader::genReflection(const QPair<QImage, QModelIndex> &imgStr)
{
    if ( !m_currentView || !m_fsModel )
        return;
    const QModelIndex &index = imgStr.second;
    const QString &name = qvariant_cast<QIcon>(m_fsModel->data(index, Qt::DecorationRole)).name();
    if ( !m_loadedThumbs[FallBackRefl].contains(name) )
        m_loadedThumbs[FallBackRefl].insert(name, reflection(imgStr.first));
    emit dataChanged(index, index);
}

QImage
ThumbsLoader::pic(const QString &filePath, const Type &t)
{
    if ( DFM::Configuration::config.views.showThumbs )
        if ( m_loadedThumbs[t].contains(filePath) )
            return m_loadedThumbs[t].value(filePath);

    if ( t >= Reflection && m_fsModel )
    {
        const Type &ft = DFM::Configuration::config.views.showThumbs ? Reflection : FallBackRefl;
        const QString &icon = qvariant_cast<QIcon>(m_fsModel->data(m_fsModel->index(filePath), Qt::DecorationRole)).name();
        if ( m_loadedThumbs[ft].contains(icon) )
            return m_loadedThumbs[ft].value(icon);
    }
    return QImage();
}

void
ThumbsLoader::disconnectView()
{
    if ( !m_currentView )
        return;
    disconnect( m_currentView->verticalScrollBar(), SIGNAL(valueChanged(int)), m_timer, SLOT(start()) );
    disconnect( m_currentView->horizontalScrollBar(), SIGNAL(valueChanged(int)), m_timer, SLOT(start()) );
    disconnect( m_fsModel, SIGNAL(layoutChanged()),instance(), SLOT(loadReflections()));
    disconnect( m_fsModel, SIGNAL(directoryLoaded(QString)), m_timer, SLOT(start()) );
    disconnect( instance(), SIGNAL(dataChanged(QModelIndex,QModelIndex)), m_fsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)) );
    m_currentView->removeEventFilter( instance() );
    m_fsModel = 0;
    m_currentView = 0;
}

void
ThumbsLoader::connectView()
{
    m_fsModel = static_cast<DFM::FileSystemModel *>(m_currentView->model());
    connect( m_currentView->verticalScrollBar(), SIGNAL(valueChanged(int)), m_timer, SLOT(start()) );
    connect( m_currentView->horizontalScrollBar(), SIGNAL(valueChanged(int)), m_timer, SLOT(start()) );
    connect( m_fsModel, SIGNAL(layoutChanged()), instance(), SLOT(loadReflections()) );
    connect( m_fsModel, SIGNAL(directoryLoaded(QString)), m_timer, SLOT(start()) );
    connect( instance(), SIGNAL(dataChanged(QModelIndex,QModelIndex)), m_fsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)) );
    m_currentView->installEventFilter( instance() );
}


void
ThumbsLoader::loadThumbs()
{
    m_timer->stop();

    if ( !DFM::Configuration::config.views.showThumbs )
        return;

    for ( int i = 0; i < m_fsModel->rowCount(m_currentView->rootIndex()); ++i )
    {
        const QModelIndex &index = m_fsModel->index( i, 0, m_currentView->rootIndex() );
        const QString file = m_fsModel->filePath(index);
        if ( QImageReader(file).canRead() )
            if ( !m_loadedThumbs[Thumb].contains(file) && !m_queue.contains(file) )
                m_queue << file;
    }
    start();
}

void
ThumbsLoader::loadReflections()
{
    for ( int i = 0; i < m_fsModel->rowCount(m_currentView->rootIndex()); ++i )
    {
        const QModelIndex &index = m_fsModel->index( i, 0, m_currentView->rootIndex() );
        const QIcon &icon = qvariant_cast<QIcon>(m_fsModel->data(index, Qt::DecorationRole));
        if ( m_loadedThumbs[FallBackRefl].contains(icon.name()) )
            continue;
        QImage img(SIZE, SIZE, QImage::Format_ARGB32);
        img.fill(Qt::transparent);
        QPainter p(&img);
        icon.paint(&p, img.rect());
        p.end();
        m_imgQueue << QPair<QImage, QModelIndex>(img, index);
    }
    start();
    m_timer->start();
}

void
ThumbsLoader::setCurrentView(QAbstractItemView *view)
{
    m_queue.clear();
    m_imgQueue.clear();
    disconnectView();
    m_currentView = view;
    connectView();
    m_timer->start();
}

void
ThumbsLoader::fileChanged(const QString &file)
{
    m_queue << file;
    start();
}

void
ThumbsLoader::run()
{
    while ( !m_imgQueue.isEmpty() )
        genReflection( m_imgQueue.takeFirst() );
    if ( DFM::Configuration::config.views.showThumbs )
        while ( !m_queue.isEmpty() )
            loadThumb( m_queue.takeFirst() );
}

bool
ThumbsLoader::eventFilter(QObject *o, QEvent *e)
{
    if ( m_currentView )
    if ( o == m_currentView && e->type() == QEvent::Resize )
    {
        vr = m_currentView->viewport()->rect();
        m_timer->start();
    }
    return false;
}
