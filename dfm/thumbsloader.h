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
#include "objects.h"
#include "helpers.h"

namespace DFM
{

struct Data
{
    QImage thumb, flowImage, flowReflection;
    QString count; //entries for dirs...
    QString mimeType, iconName;
};

class DataLoader : public Thread
{
    Q_OBJECT
public:
    explicit DataLoader(QObject *parent = 0);
    enum Type { Thumb = 0, FlowPic, Reflection, FallBackRefl };
    static DataLoader *instance();
    static inline void clearQueue() { instance()->clearQ(); }
    static inline void removeData(const QString &file) { instance()->_removeData(file); }
    static inline struct Data *data(const QString &file) { return instance()->_data(file); }

public slots:
    void discontinue() { m_queueMutex.lock(); m_queue.clear(); m_queueMutex.unlock(); Thread::discontinue(); }
    void fileRenamed(const QString &path, const QString &oldName, const QString &newName);
    void _removeData(const QString &file);
    
signals:
    void newData(const QString &file, const QString &name);

protected:
    void run();
    void getData(const QString &path);
    void clearQ();
    Data *_data(const QString &file);

private:
    mutable QMutex m_queueMutex, m_dataMutex;
    QHash<QString, QImage> m_thumbs;
    QHash<QString, QString> m_dateCheck;
    QHash<QString, QString> m_icons;
    QStringList m_queue, m_tried;
    int m_extent;
    MimeProvider mimeProvider;
    static DataLoader *m_instance;
};

class ImagesThread : public Thread
{
    Q_OBJECT
public:
    explicit ImagesThread(QObject *parent = 0);
    void queueFile(const QString &file, const QImage &source, const bool force = false);
    void queueName(const QIcon &icon);
    QImage flowData(const QString &file, const bool refl = false);
    QImage flowNameData(const QString &name, const bool refl = false);
    bool hasData(const QString &file);
    bool hasNameData(const QString &name);
    void removeData(const QString &file);

public slots:
    void discontinue() { m_imgQueue.clear(); Thread::discontinue(); }
    inline void clearData() { for (int i=0; i<2; ++i) m_images[i].clear(); }

signals:
    void imagesReady(const QString &file);

protected:
    void genImagesFor(const QPair<QString, QImage> &strImg);
    void genNameIconsFor(const QPair<QString, QImage> &strImg);
    void run();

private:
    QMap<QString, QPair<QString, QImage> > m_imgQueue;
    QMap<QString, QImage> m_names[2];
    QMap<QString, QImage> m_images[2];
    QMap<QString, QPair<QString, QImage> > m_nameQueue;
    mutable QMutex m_queueMutex, m_thumbsMutex;
};

}

#endif // THUMBSLOADER_H
