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


#ifndef INFOWIDGET_H
#define INFOWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPainter>
#include <QModelIndex>

namespace DFM
{

class TextLabel : public QWidget
{
    Q_OBJECT
public:
    explicit TextLabel(QWidget *parent = 0) : QWidget(parent)
    {
        m_fg = palette().color(foregroundRole());
        setMinimumSize(64, 64);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
protected:
    virtual void paintEvent(QPaintEvent *)
    {
        QPainter p(this);
        p.setPen(m_fg);
        p.drawText(rect(), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWrapAnywhere, m_text );
        p.end();
    }
public slots:
    void setText( const QString &text ) { m_text = text; update(); }
private:
    QString m_text;
    QColor m_fg;
};


class ThumbWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ThumbWidget(QWidget *parent = 0) : QWidget(parent) { setFixedSize(64, 64); }
protected:
    void paintEvent(QPaintEvent *)
    {
        QPainter p(this);
        QRect pixRect(pix.rect()), r(rect());
        QSize picSize(pixRect.size());
        if (qMin(picSize.width(), picSize.height()) > 64)
            picSize.scale(r.size(), Qt::KeepAspectRatio);
        QRect newRect(QPoint(), picSize);
        newRect.moveCenter(r.center());
        p.drawTiledPixmap(newRect, pix);
        p.end();
    }
public slots:
    void setPixmap(QPixmap pixmap) { pix = pixmap; update(); }
private:
    QPixmap pix;
};


class InfoWidget : public QFrame
{
    Q_OBJECT
public:
    explicit InfoWidget(QWidget *parent = 0);
    
signals:
protected:
    
public slots:
    void hovered(const QModelIndex &index);

private:
    ThumbWidget *tw;
    QLabel *ownerLbl, *owner, *typeLbl, *typ, *mimeLbl, *mime, *sizeLbl, *m_size;
    TextLabel *fileName;
};

}

#endif // INFOWIDGET_H
