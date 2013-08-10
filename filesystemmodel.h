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


#ifndef FILESYSTEMMODEL_H
#define FILESYSTEMMODEL_H

#include <QFileSystemModel>
#include <QPainter>

#include <QDir>
#include <QFileIconProvider>
#include <QTimer>
#include <QMimeData>
#include <QSettings>
#include <QDateTime>
#include <QUrl>
#include <QLabel>
#include <QStyle>
#include <QDebug>

namespace DFM
{

class FileSystemModel : public QFileSystemModel
{
    Q_OBJECT
public:
    enum Roles {
        FileIconRole = Qt::DecorationRole,
        FilePathRole = Qt::UserRole + 1,
        FileNameRole = Qt::UserRole + 2,
        FilePermissions = Qt::UserRole + 3,
        Thumbnail = Qt::UserRole +4,
        Reflection = Qt::UserRole + 5,
        FlowPic = Qt::UserRole + 6,
        DecoImage = Qt::UserRole + 7
    };
    explicit FileSystemModel(QObject *parent = 0);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    static QPixmap iconPix(const QFileInfo &info,const int &extent);
    static QStringList supportedThumbs( const bool &filter = false );
    inline void refresh() { const QString &path = rootPath(); setRootPath(""); setRootPath(path); }

public slots:
    void setPath(const QString &path) { setRootPath(path); }
private slots:
    void emitRootIndex( const QString &path ) { emit rootPathAsIndex(index(path)); }

signals:
    void rootPathAsIndex( const QModelIndex &index );

protected:
    virtual int columnCount(const QModelIndex &parent) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
    QStringList m_nameThumbs;
    QObject *m_parent; 
};

}

#endif // FILESYSTEMMODEL_H
