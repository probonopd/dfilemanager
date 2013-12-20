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
#include <QWaitCondition>
#include <QMutex>
#include "filesystemmodel.h"
#include "interfaces.h"

namespace DFM
{

#define PAUSING inline void setPause(bool p) {m_mutex.lock(); m_pause=p; m_mutex.unlock(); if (!p) m_pauseCond.wakeAll();}\
    inline void pause() { m_mutex.lock(); if (m_pause) m_pauseCond.wait(&m_mutex); m_mutex.unlock(); }\
    bool m_pause;\
    QMutex m_mutex;\
    QWaitCondition m_pauseCond;

class FileSystemModel;
class ThumbsLoader : public QThread
{
    Q_OBJECT
    PAUSING
public:
    explicit ThumbsLoader(QObject *parent = 0);
    enum Type { Thumb = 0, FlowPic, Reflection, FallBackRefl };
    static ThumbsLoader *instance();
    void queueFile(const QString &file);
    bool hasThumb(const QString &file) const;
    bool hasIcon(const QString &dir) const;
    QImage thumb(const QString &file) const;
    QString icon(const QString &dir) const;
    inline void discontinue() { m_mutex.lock(); m_queue.clear(); m_quit=true; m_mutex.unlock(); setPause(false); }

public slots:
    void fileRenamed(const QString &path, const QString &oldName, const QString &newName);
    void removeThumb(const QString &file);
    void clearQueue();
    
signals:
    void thumbFor(const QString &file, const QString &name);

protected:
    void run();
    void genThumb(const QString &path);

private:
    bool m_quit;
    mutable QMutex m_listMutex;
    QStringList m_queue, m_tried;
    int m_extent;
    static ThumbsLoader *m_instance;
};

class ImagesThread : public QThread
{
    Q_OBJECT
    PAUSING
public:
    explicit ImagesThread(QObject *parent = 0);
    void queueFile( const QString &file, const QImage &source, const bool force = false );
    void queueName( const QIcon &icon );
    QImage flowData( const QString &file, const bool refl = false );
    QImage flowNameData( const QString &name, const bool refl = false );
    bool hasData( const QString &file );
    bool hasNameData( const QString &name );
    void removeData( const QString &file );
    inline bool hasFileInQueue( const QString &file ) { return m_imgQueue.contains(file); }
    inline void discontinue() { m_mutex.lock(); m_imgQueue.clear(); m_quit=true; m_mutex.unlock(); setPause(false); }

public slots:
    inline void clearData() { for (int i=0; i<2; ++i) m_images[i].clear(); }

signals:
    void imagesReady( const QString &file );

protected:
    void genImagesFor( const QString &file );
    void genNameIconsFor( const QString &name );
    void run();

private:
    bool m_quit;
    QMap<QString, QImage> m_imgQueue;
    QMap<QString, QImage> m_names[2];
    QMap<QString, QImage> m_images[2];
    QMap<QString, QImage> m_nameQueue;
};

}

#endif // THUMBSLOADER_H
