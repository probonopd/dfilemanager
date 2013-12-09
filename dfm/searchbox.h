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


#ifndef SEARCHBOX_H
#define SEARCHBOX_H

#include <QLineEdit>
#include <QPixmap>
#include <QDebug>

namespace DFM
{

class IconWidget : public QWidget
{
    Q_OBJECT
public:
    explicit IconWidget(QWidget *parent = 0);
    void setPix(bool clear) { m_type = clear; }
    void setOpacity(float opacity) { m_opacity = opacity; }

signals:
    void clicked();
protected:
    void mouseReleaseEvent(QMouseEvent *) { emit clicked(); }
    void paintEvent(QPaintEvent *);
    void enterEvent(QEvent *) { setOpacity( 1.0 ); }
    void leaveEvent(QEvent *) { setOpacity( 0.5 ); }

private:
    QPixmap m_pix[2];
    bool m_type;
    float m_opacity;
};

class SearchBox : public QLineEdit
{
    Q_OBJECT
public:
    explicit SearchBox(QWidget *parent = 0);

protected:
    void resizeEvent(QResizeEvent *);

private slots:
    void setClearButtonEnabled(QString filter) { m_iconWidget->setPix(!filter.isEmpty()); }

private:
    IconWidget *m_iconWidget;
    int m_margin;
};

}

#endif // SEARCHBOX_H
