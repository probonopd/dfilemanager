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


/* A collection of classes that's based on QWidget thats used
 * all around in the code.
 */

#ifndef WIDGETS_H
#define WIDGETS_H

#include <QWidget>
#include <QIcon>
#include <QMenu>
#include <QTimer>

namespace DFM
{

class Button : public QWidget
{
    Q_OBJECT
public:
    explicit Button(QWidget *parent = 0);
    void setIcon(const QIcon &icon);
    inline void setMenu(QMenu *menu) { m_menu = menu; m_hasMenu = true; }

public slots:
    void animate();
    void stopAnimating();
    void startAnimating();

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void paintEvent(QPaintEvent *);
    void paletteChange(const QPalette &);

private:
    bool m_hasPress, m_hasMenu, m_isAnimating;
    int m_animStep, m_stepSize;
    QIcon m_icon, m_animIcon;
    QMenu *m_menu;
    QTimer *m_animTimer;
    QTransform m_transform;
};

}

#endif // WIDGETS_H
