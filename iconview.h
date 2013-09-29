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


#ifndef ICONVIEW_H
#define ICONVIEW_H

#include <QListView>
#include <QApplication>
#include <QWheelEvent>
#include <QScrollBar>
#include <QSettings>
#include <QPropertyAnimation>
#include <QTransform>

#include "filesystemmodel.h"

namespace DFM
{
#if 0
class Overlay : public QWidget
{
    Q_OBJECT
public:
    Overlay(QWidget *parent) : QWidget(parent){}

protected:
    inline virtual void paintEvent(QPaintEvent *)
    {
        QPainter p(this);
        setAttribute(Qt::WA_TransparentForMouseEvents);
        QPixmap turnone(":/turnfinder.png");
        QRect rect(turnone.rect());
        p.drawTiledPixmap(rect,turnone);
        p.end();
    }
};
#endif
class FileSystemModel;
class IconView : public QListView
{
    Q_OBJECT
public:
    explicit IconView(QWidget *parent = 0);
    void setFilter(QString filter);
    void setNewSize( const int size );
    void setModel(QAbstractItemModel *model);

protected:
    void wheelEvent(QWheelEvent *);
    void resizeEvent(QResizeEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void contextMenuEvent(QContextMenuEvent *event);
    void mouseReleaseEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void setIconWidth(const int width);
    QModelIndex firstValidIndex();
    inline int iconWidth() const { return iconSize().width(); }

public slots:
    void updateLayout();
    void correctLayout();

signals:
    void iconSizeChanged(int size);
    void newTabRequest(QModelIndex path);
    
private slots:
    void rootPathChanged(QString path);
    void scrollEvent();
    void setGridHeight(int gh);
    void updateIconSize();

private:
    QPoint m_startPos;
    QList<int> m_allowedSizes;
    bool m_slide, m_startSlide;
    FileSystemModel *m_fsModel;
    QTimer *m_scrollTimer, *m_sizeTimer;
    QModelIndex m_firstIndex;
    int m_delta, m_newSize, m_gridHeight;
};

}

#endif // ICONVIEW_H
