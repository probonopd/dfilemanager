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


#include "operations.h"
#include <QFileInfo>
#include <QVector>
#include <QDebug>
#include <QProcess>

#ifdef Q_WS_X11
#include <magic.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/statvfs.h>
#endif
//#include <blkid/blkid.h> //dev block id info... maybe need later

using namespace DFM;

QString
Operations::getMimeType(const QString &file)
{
#ifdef Q_WS_X11
    const magic_t &mgcMime = magic_open( MAGIC_MIME_TYPE );
    magic_load( mgcMime, NULL );
    return QString( magic_file( mgcMime, file.toStdString().c_str() ) );
#else
    return QString();
#endif
}

QString
Operations::getFileType(const QString &file)
{
#ifdef Q_WS_X11
    magic_t mgcMime = magic_open( MAGIC_CONTINUE ); //print anything we can get
    magic_load( mgcMime, NULL );
    return QString( magic_file( mgcMime, file.toStdString().c_str() ) );
#else
    return QString();
#endif
}

quint64
Operations::getDriveInfo(const QString &file, const Usage &t)
{
#ifdef Q_WS_X11
    if(!QFileInfo(file).exists())
        return 0;
    struct statfs sfs;
    statfs(file.toLatin1(),&sfs);
    const quint64 &fragsize = sfs.f_frsize,
            &blocks = sfs.f_blocks,
            &available = sfs.f_bavail;
    const quint64 *id = (quint64 *)&sfs.f_fsid;
    const quint64 &total = blocks*fragsize,
            &free = available*fragsize,
            &used = (blocks-available)*fragsize;
    switch (t)
    {
    case Free: return free;
    case Used: return used;
    case Total: return total;
    case Id: return *id;
    }
    return 0;
#else
    return 0;
#endif
}

QColor
Operations::colorMid(const QColor c1, const QColor c2, int i1, int i2)
{
    int r,g,b,a;
    int i3 = i1+i2;
    r = qMin(255,(i1*c1.red() + i2*c2.red())/i3);
    g = qMin(255,(i1*c1.green() + i2*c2.green())/i3);
    b = qMin(255,(i1*c1.blue() + i2*c2.blue())/i3);
    a = qMin(255,(i1*c1.alpha() + i2*c2.alpha())/i3);
    return QColor(r,g,b,a);
}

void
Operations::openFile(const QString &file)
{
    if(!QFileInfo(file).exists())
        return;

    QProcess process;
    if(QFileInfo(file).isExecutable() && (QFileInfo(file).suffix().isEmpty() ||
                                                         QFileInfo(file).suffix() == "sh" ||
                                                         QFileInfo(file).suffix() == "exe")) // windows executable
    {
        process.startDetached(file);
    }
    else
    {
        QStringList list;
        list << file;

#ifdef Q_WS_X11 //linux
        process.startDetached("xdg-open",list);
#else //windows
        process.startDetached("cmd /c start " + list.at(0)); //todo: fix, this works but shows a cmd window real quick
#endif
    }
}
