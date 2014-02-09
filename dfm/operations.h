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


#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <QString>
#include <QColor>
#include <QFileInfo>
#include <QWidget>
#include <QTextEdit>
#include <QStyledItemDelegate>

#include "globals.h"
#include "filesystemmodel.h"

#ifdef Q_WS_X11
#include <magic.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/statvfs.h>
#endif

static int defaultSort = 0;
static Qt::SortOrder defaultOrder = Qt::AscendingOrder;

namespace DFM
{

static IOJobData defaultIoJobData;

class Ops : public QObject
{
    Q_OBJECT
public:
    static QString getMimeType(const QString &file);
    static QString getFileType(const QString &file);
    static QColor colorMid(const QColor c1, const QColor c2, int i1 = 1, int i2 = 1);
    static bool openFile(const QString &file);
    static Ops *instance();
    static QString prettySize(quint64 bytes);
    static QImage reflection(const QImage &img = QImage());
    static QImage flowImg(const QImage &img = QImage());
    static QImage blurred(const QImage& image, const QRect& rect, int radius, bool alphaOnly = false);
    static void getSorting(const QString &file, int &sortCol = defaultSort, Qt::SortOrder &order = defaultOrder);
    static QString sanityChecked(const QString &file);
    static IOTask taskFromString(const QString &task);
    static QString taskToString(const IOTask &task);
    static bool extractIoData(const QStringList &args, IOJobData &ioJobData = defaultIoJobData);
    static QStringList fromIoJobData(const IOJobData &data);
    static bool sameDisk(const QString &file1, const QString &file2);
    static QString getStatusBarMessage(const QUrl &url, FS::Model *model);

    template<typename T> static inline T absWinFor(QWidget *w)
    {
        QWidget *widget = w;
        while (widget->parentWidget())
            widget = widget->parentWidget();
        return qobject_cast<T>(widget);
    }

    enum Usage {Free = 0, Used, Total, Id};
    template<Usage type> static inline quint64 getDriveInfo(const QString &file = QString())
    {
#ifdef Q_WS_X11
        if (!QFileInfo(file).exists())
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
        switch (type)
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

public slots:
    void openWith();
    void customActionTriggered();
    void setRootPath();

protected:
    explicit Ops(QObject *parent = 0):QObject(parent){}
};

}

#endif // OPERATIONS_H
