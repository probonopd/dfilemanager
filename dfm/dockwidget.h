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


#ifndef DOCKWIDGET_H
#define DOCKWIDGET_H

#include <QDockWidget>
#include "globals.h"

namespace DFM
{
namespace Docks
{

class DockWidget : public QDockWidget
{
    Q_OBJECT
public:
    DockWidget(QWidget *parent = 0, const QString &title = QString(), const Qt::WindowFlags &flags = 0, const Pos &pos = Left);
    inline bool isLocked() const { return m_isLocked; }

protected:
    virtual bool eventFilter(QObject *, QEvent *);
//    inline virtual void showEvent(QShowEvent *event) { QDockWidget::showEvent(event); if(isFloating()) { m_mainWindow->setFocus(); setMask(shape()); } }
    virtual void resizeEvent(QResizeEvent *event);
    virtual void moveEvent(QMoveEvent *event);
    virtual void paintEvent(QPaintEvent *event);
    void adjustPosition();
    QRegion shape() const;

public slots:
    void floatationChanged(const bool floating);
    inline void setFloat() { /*m_mainWindow->raise(); m_timer->start(10);*/ if (isFloating()) m_dirIn = true; else setFloating(true); }
    inline void toggleVisibility() { setVisible(!isVisible()); }
    inline void toggleLock() { if (m_isLocked) setLocked(false); else setLocked(true); }
    void setLocked(const bool locked = false);

private slots:
    void animate();
    void postConstructorJobs();

private:
    Pos m_position;
    QTimer *m_timer;
    int m_animStep;
    bool m_dirIn, m_isLocked;
    QMargins m_margins;
};

}
}

#endif // DOCKWIDGET_H
