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

class ThumbsLoader : public QThread
{
    Q_OBJECT
public:
    enum Type { Thumb = 0, Reflection = 1, FlowPic = 2, FallBackRefl = 3 };
    static ThumbsLoader *instance();
    static QImage thumb( const QString &filePath, const Type &t = Thumb ) { return instance()->pic(filePath, t); }
    
signals:
    void dataChanged( const QModelIndex &topLeft, const QModelIndex &bottomRight );
    
public slots:
    void loadLater();
    void setCurrentView( QAbstractItemView *view );
    void loadThumbs();
    void fileChanged( const QString &file );
    void loadReflections();

protected:
    explicit ThumbsLoader(QObject *parent = 0);
    virtual void run();
    bool eventFilter(QObject *o, QEvent *e);
    void loadThumb( const QString &path );
    void genReflection( const QPair<QImage, QModelIndex> &imgStr );
    void connectView();
    void disconnectView();
    QImage pic( const QString &filePath, const Type &t = Thumb );

private:
    static QStringList m_queue;
    static QList<QPair<QImage, QModelIndex> > m_imgQueue;
    static QHash<QString, QImage> m_loadedThumbs[4];
    static QFileSystemWatcher *m_fsWatcher;
    static DFM::FileSystemModel *m_fsModel;
    static QTimer *m_timer;
    int m_extent;
    QAbstractItemView *m_currentView;
};

#endif // THUMBSLOADER_H
