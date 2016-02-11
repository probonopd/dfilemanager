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

/* A collection of classes that's based on QObject thats used
 * all around in the code.
 */

#ifndef OBJECTS_H
#define OBJECTS_H

#include <QThread>
#include <QStyledItemDelegate>
#include <QWaitCondition>
#include <QMutex>

class QAbstractScrollArea;

namespace DFM
{

class DThread : public QThread
{
    Q_OBJECT
public:
    explicit DThread(QObject *parent = 0);
    bool isPaused() const;
    void pause();

signals:
    void pauseToggled(const bool isPaused);

public slots:
    void setPause(bool p);
    virtual void discontinue();

protected:
    QWaitCondition m_pausingCondition;
    mutable QMutex m_pausingMutex;
    bool m_pause;
    bool m_quit;
};

class FileItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    enum ShadowPart { TopLeft = 0, Top, TopRight, Left, Center, Right, BottomLeft, Bottom, BottomRight };
    explicit FileItemDelegate(QObject *parent = 0);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    bool eventFilter(QObject *object, QEvent *event);
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    static inline int shadowSize() { return 5; }

protected:
    static QPixmap shadowPix();
    static void genShadowData();
    static void drawShadow(QRect rect, QPainter *painter);
    static QPixmap *s_shadowData;
};

class ScrollAnimator : public QObject
{
    Q_OBJECT
public:
    static void manage(QAbstractScrollArea *area);
    static ScrollAnimator *instance();

protected:
    ScrollAnimator(QObject *parent = 0);
    bool eventFilter(QObject *o, QEvent *e);
    bool processWheelEvent(QWheelEvent *e);

protected slots:
    void updateScrollValue();

private:
    QTimer *m_timer;
    bool m_up;
    int m_delta, m_step;
};

}

#endif // OBJECTS_H
