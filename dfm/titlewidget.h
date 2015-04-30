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


#ifndef TITLEWIDGET_H
#define TITLEWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QPen>
#include <QLabel>
#include <QHBoxLayout>
#include <QDebug>
#include <QStatusBar>
#include <QMouseEvent>
#include "globals.h"

namespace DFM
{

namespace Docks
{

inline static QPixmap titlePix(bool floating, int size, QColor color)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    QPen pen(color, size/8);
    QPolygon pol;
    const int m = size/4;
    p.setClipRect(0, m, size, size-m*2);
    if(floating)
        pol.setPoints(3,  m,m,  m*2,size/2,  m,size-m);
    else
        pol.setPoints(3, size-m,m,  size-m*2,size/2,  size-m,size-m);
    QPainterPath path;
    path.addPolygon(pol);
    p.strokePath(path, pen);
    p.strokePath(path.translated(floating ? m : -m, 0), pen);
    p.end();
    return pixmap;
}

inline static QPixmap statusPix(bool visible, int size, QColor color)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    QPen pen(color, size/16);
    const int m = size/8;
    QRect r(pixmap.rect().adjusted(m, m, -m, -m));
    if (pen.width() & 0x1)
    {
        p.translate(0.5, 0.5);
        r.adjust(0, 0, -1, -1);
    }
    p.setPen(pen);
    p.drawRect(r);
    r.setRight(r.width()/3+1);
    p.drawLine(r.topRight(), r.bottomRight());
    if(visible)
    {
        r.setRight(r.right()-1);
        p.setBrush(color);
        p.drawRect(r);
    }
    p.end();
    return pixmap;
}

class FloatButton : public QWidget
{
    Q_OBJECT
public:
    inline explicit FloatButton(QWidget *parent = 0, Pos pos = Left) : QWidget(parent), m_floating(false), m_hover(false), m_hasClick(false), m_myPos(pos), m_parent(parent)
    {
        setCursor(Qt::PointingHandCursor);
        genIcons();
    }
public slots:
    inline void setFloating(bool f) { m_floating = f; m_hover = false; }
signals:
    void clicked();
    void rightClicked();
protected:
    inline void paintEvent(QPaintEvent *) {
        QPainter p(this);
        p.drawTiledPixmap(rect(), m_pixmap[m_floating][m_hover]);
        p.end();
    }
    inline void mousePressEvent(QMouseEvent *event) { m_hasClick = true; event->accept(); }
    inline void mouseReleaseEvent(QMouseEvent *event)
    {
        if (event->button() == Qt::RightButton)
        {
            emit rightClicked();
            return;
        }
        if (m_hasClick)
            emit clicked();
        m_hasClick = false;
        event->accept();
    }
    inline void enterEvent(QEvent *event) { m_hasClick = false; m_hover = true; update(); event->accept(); }
    inline void leaveEvent(QEvent *event) { m_hasClick = false; m_hover = false; update(); event->accept(); }
    inline bool event(QEvent *e) { if (e->type() == QEvent::PaletteChange) genIcons(); return QWidget::event(e); }
private slots:
    inline void genIcons()
    {
        QColor c[2];
        c[0] = m_parent ? m_parent->palette().color(m_parent->foregroundRole()) : palette().color(foregroundRole());
        c[1] = palette().color(QPalette::Highlight);
        for (int i = 0; i<2; i++)
            for (int h = 0; h<2; h++)
            {
                if (qobject_cast<QStatusBar *>(m_parent))
                    m_pixmap[i][h] = statusPix(i, 16, c[h]);
                else
                    m_pixmap[i][h] = titlePix(i, 16, c[h]);
                if (m_myPos == Bottom)
                {
                    QTransform t;
                    t.rotate(180);
                    m_pixmap[i][h] = m_pixmap[i][h].transformed(t);
                }
                if (m_myPos == Right)
                {
                    QTransform t;
                    t.rotate(270);
                    m_pixmap[i][h] = m_pixmap[i][h].transformed(t);
                }
            }
    }
private:
    QPixmap m_pixmap[2][2];
    bool m_floating, m_hover, m_hasClick;
    Pos m_myPos;
    QWidget *m_parent;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////

class TitleWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TitleWidget(QWidget *parent, const QString &title, Pos pos = Left) : QWidget(parent)
      , titleText(title), fb(0), floating(false), myPos(pos)
    {
        fb = new FloatButton(this, pos);
        fb->setFixedSize(16, 16);
        QFont f(font());
        f.setBold(true);
        QLabel *titleLbl = new QLabel(this);
        titleLbl->setAlignment(Qt::AlignCenter);
        titleLbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        titleLbl->setText(titleText);
        titleLbl->setFont(f);
        QHBoxLayout *layout = new QHBoxLayout();
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(fb);
        layout->addWidget(titleLbl);
        if(pos == Right)
            layout->setDirection(QBoxLayout::RightToLeft);
        setLayout(layout);
    }
    FloatButton *floatButton() { return fb; }
public slots:
    void setIsFloating(bool f) { floating = f; update(); fb->setFloating(f); }
protected:
    void mouseDoubleClickEvent(QMouseEvent *event) { event->accept(); }
    void mousePressEvent(QMouseEvent *event) { event->accept(); }
    void mouseReleaseEvent(QMouseEvent *event) { event->accept(); }
#if 0
    void paintEvent(QPaintEvent *)
    {
        if (!floating)
            return;
        QPainter p(this);
        QRect r(rect());
        QLinearGradient lg(r.topLeft(), r.bottomLeft());
        lg.setColorAt(0, QColor(255, 255, 255, 32));
        lg.setColorAt(1, QColor(0, 0, 0, 32));
        p.fillRect(r, lg);
        p.end();
    }
#endif

private:
    QString titleText;
    FloatButton *fb;
    bool floating;
    Pos myPos;
};

}

}
#endif // TITLEWIDGET_H
