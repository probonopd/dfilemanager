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
#include "viewcontainer.h"

using namespace DFM;

static QHash<QString, QImage> s_thumbs;
static QImageReader ir;
static QRect vr;

ThumbsLoader::ThumbsLoader(QObject *parent) :
    QThread(parent),
    m_extent(256),
    m_fsModel(static_cast<DFM::FileSystemModel *>(parent))
{}

bool ThumbsLoader::hasThumb(const QString &file) { return s_thumbs.contains(file); }

void
ThumbsLoader::queueFile(const QString &file)
{
    if ( m_queue.contains(file) || s_thumbs.contains(file) )
        return;
    m_queue << file;
    start();
}

QImage
ThumbsLoader::thumb(const QString &file)
{
    if ( hasThumb(file) )
        return s_thumbs.value(file);
    return QImage();
}

void
ThumbsLoader::loadThumb( const QString &path )
{
    if ( !QFileInfo(path).exists() )
    {
        s_thumbs.remove(path);
        return;
    }
    ir.setFileName(path);
    if ( !ir.canRead() || path.endsWith( "xcf", Qt::CaseInsensitive ) )
        return;

    QSize thumbsize = ir.size();
    if ( qMax( thumbsize.width(), thumbsize.height() ) > m_extent )
        thumbsize.scale( m_extent, m_extent, Qt::KeepAspectRatio );
    ir.setScaledSize(thumbsize);

    const QImage image(ir.read());

    if ( !image.isNull() )
    {
        s_thumbs.insert(path, image);
        emit thumbFor(path);
    }
}

void
ThumbsLoader::run()
{
    while ( !m_queue.isEmpty() )
        loadThumb(m_queue.takeFirst());
}

QHash<QString, QImage> s_images[2];

ImagesThread::ImagesThread(QObject *parent)
    :QThread(parent)
    ,m_fsModel(static_cast<FileSystemModel *>(parent))
{}

void
ImagesThread::removeData(const QString &file)
{
    for ( int i = 0; i < 2; ++i )
        s_images[i].remove(file);
    m_pixQueue.removeOne(file);
}

void
ImagesThread::genImagesFor(const QString &file)
{
    const QImage &source = m_sourceImgs.take(file);
    s_images[0].insert(file, Ops::flowImg(source));
    s_images[1].insert(file, Ops::reflection(source));
    emit imagesReady(file);
}

void ImagesThread::run()
{
    while ( !m_pixQueue.isEmpty() )
        genImagesFor(m_pixQueue.takeFirst());
}

QImage
ImagesThread::flowData(const QString &file, const bool refl)
{
    if ( hasData(file) )
        return s_images[refl].value(file);
    return QImage();
}

bool
ImagesThread::hasData(const QString &file)
{
    return s_images[0].contains(file);
}

void
ImagesThread::queueFile(const QString &file )
{
    if ( m_pixQueue.contains(file) || s_images[0].contains(file) )
        return;
    m_pixQueue << file;
    const QIcon &icon = qvariant_cast<QIcon>(m_fsModel->data(m_fsModel->index(file), Qt::DecorationRole));
    const QImage &source = icon.pixmap(SIZE).toImage();
    m_sourceImgs.insert(file, source);
    start();
}
