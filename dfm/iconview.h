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

#include <QAbstractItemView>
#include <QApplication>
#include <QWheelEvent>
#include <QScrollBar>
#include <QSettings>
#include <QPropertyAnimation>
#include <QTransform>

#include "filesystemmodel.h"
#include "viewcontainer.h"

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
class IconView;
class GayBar : public QScrollBar
{
    Q_OBJECT
public:
    GayBar(const Qt::Orientation ori = Qt::Vertical, QWidget *parent = 0, QWidget *vp = 0);
protected:
    void paintEvent(QPaintEvent *);
private:
    QWidget *m_viewport;
    IconView *m_iv;
};

class FileSystemModel;
class ViewContainer;
class IconView : public QAbstractItemView
{
    Q_OBJECT
public:
    explicit IconView(QWidget *parent = 0);
    void setNewSize( const int size );
    void setModel(QAbstractItemModel *model);
    inline QSize gridSize() const { return m_gridSize; }
    void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible);
    inline QPixmap bgPix() const { return m_pix; }

protected:
    void wheelEvent(QWheelEvent *);
    void resizeEvent(QResizeEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void contextMenuEvent(QContextMenuEvent *event);
    void mouseReleaseEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void showEvent(QShowEvent *e);
    QRect visualRect(const QModelIndex &index) const;
    QRect visualRect(const QString &cat) const;
    QModelIndex indexAt(const QPoint &p) const;
    void rowsInserted(const QModelIndex &parent, int start, int end);
    void setIconWidth(const int width);
    QModelIndex firstValidIndex();
    inline int iconWidth() const { return iconSize().width(); }
    bool isCategorized() const;
    QStyleOptionViewItem viewOptions() const;

    int horizontalOffset() const;
    bool isIndexHidden(const QModelIndex & index) const;
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers);
    void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags flags);
    int verticalOffset() const;
    QRegion visualRegionForSelection(const QItemSelection &selection) const;

public slots:
    void updateLayout();
    void correctLayout();
    inline void setGridSize(const QSize &gridSize) { m_gridSize = gridSize; calculateRects(); }

signals:
    void iconSizeChanged(const int size);
    void newTabRequest(const QModelIndex &path);
    
private slots:
    void rootPathChanged(const QString &path);
    void scrollEvent();
    void setGridHeight(int gh);
    void updateIconSize();
    void calculateRects();

private:
    QPixmap m_pix;
    QSize m_gridSize;
    ViewContainer *m_container;
//    GayBar *m_gayBar;
    QPoint m_startPos, m_pressPos;
    QList<int> m_allowedSizes;
    QHash<void *, QRect> m_rects;
    QHash<QString, QRect> m_catRects;
    bool m_slide, m_startSlide;
    FileSystemModel *m_model;
    QTimer *m_scrollTimer, *m_sizeTimer, *m_layTimer;
    QModelIndex m_firstIndex;
    QPixmap m_homePix, m_bgPix[2];
    int m_delta, m_newSize, m_gridHeight, m_horItems;
};

}

#endif // ICONVIEW_H
