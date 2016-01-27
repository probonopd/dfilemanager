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

#include <QSettings>
#include <QString>
#include <QColor>
#include <QFileInfo>
#include <QWidget>
#include <QTextEdit>
#include <QStyledItemDelegate>

#include "globals.h"

#if  defined(HASSYS)
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
namespace FS{class Model;}

class Ops : public QObject
{
    Q_OBJECT
public:
    static void expblur(QImage &img, int radius, Qt::Orientations o = Qt::Horizontal|Qt::Vertical );
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
    template<typename T> static inline bool writeDesktopValue(const QDir &dir, const QString &key, T v, const QString &custom = QString())
    {
        if (dir.isAbsolute() && QFileInfo(dir.path()).isWritable())
        {
            QSettings settings(dir.absoluteFilePath(".directory"), QSettings::IniFormat);
            settings.beginGroup("DFM");
            settings.setValue(key, v);
            settings.endGroup();
            return true;
        }
        QString newKey;
        if (dir.isAbsolute() && dir.exists())
            newKey = dir.path();
        else if (!custom.isEmpty())
            newKey = custom;
        else
            return false;
        newKey.replace("/", "_");
        QSettings settings("dfm", "desktopFile");
        settings.beginGroup(newKey);
        settings.setValue(key, v);
        settings.endGroup();
        return true;
    }
    template<typename T> static inline T getDesktopValue(const QDir &dir, const QString &key, bool *ok = 0, const QString &custom = QString())
    {
        if (ok)
            *ok = false;
        if (!dir.isAbsolute() && custom.isEmpty())
            return T();
        const QFileInfo fi(dir.absoluteFilePath(".directory"));
        QVariant var;
        if (dir.isAbsolute() && fi.isReadable() && fi.isAbsolute())
        {
            QSettings settings(fi.filePath(), QSettings::IniFormat);
            settings.beginGroup("DFM");
            var = settings.value(key);
            settings.endGroup();
        }
        else
        {          
            QString newKey;
            if (dir.isAbsolute() && dir.exists())
                newKey = dir.path();
            else if (!custom.isEmpty())
                newKey = custom;
            else
                return T();
            newKey.replace("/", "_");
            QSettings settings("dfm", "desktopFile");
            settings.beginGroup(newKey);
            var = settings.value(key);
            settings.endGroup();
        }
        if (var.isValid())
        {
            if (ok)
                *ok = true;
            return var.value<T>();
        }
        return T();
    }
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
#if defined(HASSYS)
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
    void getPathToClipBoard();

protected:
    explicit Ops(QObject *parent = 0):QObject(parent){}

private:
    static Ops *m_instance;
};

}

#endif // OPERATIONS_H
