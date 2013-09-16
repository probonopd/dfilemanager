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


#ifndef THUMBSLOADER_H
#define THUMBSLOADER_H

#include <QObject>
#include <QHash>
#include <QThread>
#include <QModelIndex>
#include <QDebug>
#include <QImageReader>
#include <QAbstractItemView>
#include <QFile>
#include <QDir>
#include <QEvent>
#include <QFileSystemWatcher>
#include "filesystemmodel.h"

namespace DFM
{

class FileSystemModel;
class ThumbsLoader : public QThread
{
    Q_OBJECT
public:
    explicit ThumbsLoader(QObject *parent = 0);
    enum Type { Thumb = 0, FlowPic, Reflection, FallBackRefl };
    void queueFile(const QString &file);
    bool hasThumb(const QString &file);
    QImage thumb(const QString &file);
    void clearQueue();
    
signals:
    void thumbFor(const QString &file);

protected:
    virtual void run();
    void loadThumb( const QString &path );

private:
    QStringList m_queue;
    FileSystemModel *m_fsModel;
    int m_extent;
};

class ImagesThread : public QThread
{
    Q_OBJECT
public:
    explicit ImagesThread(QObject *parent = 0);
    void queueFile( const QString &file );
    QImage flowData( const QString &file, const bool refl = false );
    bool hasData( const QString &file );
    void removeData( const QString &file );
    inline bool hasFileInQueue( const QString &file ) { return m_pixQueue.contains(file); }
    void clearQueue();

signals:
    void imagesReady( const QString &file );

protected:
    void genImagesFor( const QString &file );
    void run();

private:
    QStringList m_pixQueue;
    QMap<QString, QImage> m_sourceImgs;
    QMap<QString, QImage > m_result[2];
    FileSystemModel *m_fsModel;
};

}

#endif // THUMBSLOADER_H
