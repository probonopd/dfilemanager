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


#ifndef PATHNAVIGATOR_H
#define PATHNAVIGATOR_H

#include <QToolBar>
#include <QToolButton>
#include <QPropertyAnimation>
#include <QAction>
#include <QMenu>
#include <QMouseEvent>
#include <QStackedWidget>
#include <QComboBox>
#include <QStyleOptionComboBox>
#include <QEvent>

#include "filesystemmodel.h"
#include "Proxystyle/proxystyle.h"
#include "operations.h"

namespace DFM
{

class NavButton : public QToolButton
{
    Q_OBJECT
public:
    inline explicit NavButton(QWidget *parent = 0, const QString &path = QString()) : QToolButton(parent), m_path(path)
    {
        setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        connect(this, SIGNAL(released()), this, SLOT(emitPath()));
        setMinimumHeight(16);
    }

signals:
    void navPath(const QString &path);

public slots:

protected:
    void mouseReleaseEvent(QMouseEvent *);
    void paintEvent(QPaintEvent *);
    inline QSize sizeHint() { return QSize(QToolButton::sizeHint().width(), iconSize().height()); }
private slots:
    void emitPath() { emit navPath(m_path); }

private:
    QString m_path;

};

class Menu : public QMenu
{
    Q_OBJECT
public:
    inline explicit Menu(QWidget *parent = 0) : QMenu(parent) {}
protected:
    virtual void mousePressEvent(QMouseEvent *e);
signals:
    void newTabRequest( const QString &path );
};

class PathSeparator : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int animLevel READ animLevel WRITE setAnimLevel)
public:
    inline explicit PathSeparator(QWidget *parent, const QString &path = QString(), FileSystemModel *fsModel = 0)
        : QWidget(parent)
        , m_animLevel(0)
        , m_animation(new QPropertyAnimation(this, "animLevel"))
        , m_path(path)
        , m_fsModel(fsModel)
    {
        setAttribute(Qt::WA_Hover);
        m_animation->setDuration(300);
        setFixedSize(16,16);
    }
    inline int animLevel() const { return m_animLevel; }

public slots:
    inline void setAnimLevel (const int &animLevel)
    {
        if (animLevel != m_animLevel)
        {
            m_animLevel = animLevel;
            update();
        }
    }

protected:
    inline virtual void mousePressEvent(QMouseEvent *event)
    {
        Menu menu;
        foreach( QFileInfo info, QDir( m_path ).entryInfoList( QDir::AllDirs | QDir::NoDotAndDotDot ) )
        {
            QAction *action = new QAction( info.fileName(), this );
            action->setText( info.fileName() );
            action->setData( info.filePath() );
            menu.addAction( action );
            connect( action, SIGNAL( triggered() ), this, SLOT( setPath() ) );
        }
        menu.exec(event->globalPos());
    }

    inline virtual void paintEvent(QPaintEvent *)
    {
        QPainter p(this);
#if 0   //carved circle look
        p.setPen(Qt::NoPen);
        p.setRenderHint(QPainter::Antialiasing);
        QColor high = palette().color(QPalette::Highlight);
        QColor fg = palette().color(QPalette::WindowText);
        QColor bg = palette().color(QPalette::Window);
        QRect r = rect().adjusted(2,2,-2,-2);
        QLinearGradient linGrad(r.topLeft(),r.bottomLeft());
        linGrad.setColorAt(0,Operations::colorMid(bg,Qt::black,1,1));
        linGrad.setColorAt(1,Operations::colorMid(bg,Qt::white,1,1));
        p.setBrush(linGrad);
        p.drawEllipse(r);
        QColor brush = Operations::colorMid(high,fg,uM,2);
        p.setBrush(brush);
        p.drawEllipse(rect().adjusted(5,5,-5,-5));
#endif
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        QColor fg = palette().color(QPalette::WindowText);
        QColor bg = palette().color(QPalette::Window);
        QRect r = rect().adjusted(4, 4, -4, -4);
        QLinearGradient linGrad(r.topLeft(), r.bottomLeft());
        linGrad.setColorAt(0, Qt::transparent);
        linGrad.setColorAt(0.5, fg);
        linGrad.setColorAt(1, Qt::transparent);
        p.translate(m_animLevel, 0);
        p.rotate(m_animLevel*5.5);
        QPainterPath triangle;
        triangle.moveTo(r.topLeft());
        triangle.lineTo(r.adjusted(0, r.height()/2, 0, 0).topRight());
        triangle.lineTo(r.bottomLeft());
        if (!m_animLevel)
        {
            p.setBrush(Operations::colorMid(bg, Qt::white, 1, 1));
            p.drawPath(triangle.translated(0,1));
        }
        p.setBrush(fg);
        p.drawPath(triangle);
        p.end();
    }
    inline virtual void enterEvent(QEvent *)
    {
        m_animation->setStartValue(0);
        m_animation->setEndValue(8);
        m_animation->start();
    }
    inline virtual void leaveEvent(QEvent *)
    {
        m_animation->setStartValue(m_animLevel);
        m_animation->setEndValue(0);
        m_animation->start();
    }

private slots:
    inline void setPath()
    {
        QAction *action = static_cast<QAction *>(sender());
        const QString &path = action->data().toString();
        m_fsModel->setRootPath(path);
    }

private:
    int m_animLevel;
    QString m_path;
    QPropertyAnimation *m_animation;
    FileSystemModel *m_fsModel;
};

class PathBox : public QComboBox
{
    Q_OBJECT
public:
    inline explicit PathBox(QWidget *parent = 0) : QComboBox(parent) {}
signals:
    void cancelEdit();
protected:
    void keyPressEvent(QKeyEvent *e) { if ( e->key() == Qt::Key_Escape ) emit cancelEdit(); else QComboBox::keyPressEvent(e); }
};

class PathNavigator : public QToolBar
{
    Q_OBJECT
public:
    explicit PathNavigator(QWidget *parent = 0, FileSystemModel *model = 0);
    inline QString path() const { return m_path; }
    
signals:
    void pathChanged(const QString &path);
    void edit();
    
public slots:
    void setPath(const QString &path);

protected:
    inline virtual QSize sizeHint() { return QSize(QToolBar::sizeHint().width(), iconSize().height()); }
    inline virtual void mousePressEvent(QMouseEvent *e) { QToolBar::mousePressEvent(e); m_hasPress = true; }
    inline virtual void mouseReleaseEvent(QMouseEvent *e) { QToolBar::mouseReleaseEvent(e); if ( m_hasPress ) emit edit(); }

private slots:
     void genNavFromPath(const QString &path);

private:
    QString m_path;
    QStringList m_pathList;
    FileSystemModel *m_fsModel;
    bool m_hasPress;
};

class BreadCrumbs : public QStackedWidget
{
    Q_OBJECT
public:
    explicit BreadCrumbs( QWidget *parent = 0, FileSystemModel *fsModel = 0 );
    inline void setEditable( const bool &editable ) { setCurrentWidget( editable ? static_cast<QWidget *>(m_pathBox) : static_cast<QWidget *>(m_pathNav) ); currentWidget()->setFocus(); }
    inline QString currentPath() { return m_pathNav->path(); }
    inline bool isEditable() { return currentWidget() == static_cast<QWidget *>(m_pathBox); }
public slots:
    inline void setPath( const QString &path ) { m_pathNav->setPath(path); }
    inline void setRootPath( const QString &rootPath ) { m_fsModel->setRootPath( rootPath ); setEditable(false); }
    inline void toggleEditable() { currentWidget() == m_pathNav ? setEditable(true) : setEditable(false); }
    void complete( const QString &path);
private slots:
    void pathChanged( const QString &path );
signals:
    void newPath( const QString &path );
protected:
    bool eventFilter(QObject *o, QEvent *e)
    {
        if ( o == m_pathBox )
        {
            if ( e->type() == QEvent::Paint )
            {
                QPainter p(m_pathBox);
                p.setRenderHint(QPainter::Antialiasing);
                p.setPen(Qt::NoPen);
                p.setBrush(m_pathBox->palette().color(m_pathBox->foregroundRole()));
                QStyleOptionComboBox opt;
                opt.initFrom(m_pathBox);
                QRect r = style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxArrow);
                QPolygon triangle(3);
                int w = r.right(), x = r.x(), y = r.y(), h = r.bottom();
                triangle.putPoints(0, 3,   x,y,   w,y,    r.center().x(),h);
                QPainterPath path;
                path.addPolygon(triangle);
                path.closeSubpath();
                p.drawPath(path);
                p.end();
                return true;
            }
            // WHY THE FUCK CANT THE FOLLOWING COMPILE?!?!? 0_o
//            else if ( e->type() == QEvent::KeyRelease )
//                if ( static_cast<QKeyEvent *>(e)->key() == Qt::Key_Escape )
//                    return true;
        }
        return QStackedWidget::eventFilter(o, e);
    }
private:
    PathBox *m_pathBox;
    PathNavigator *m_pathNav;
    FileSystemModel *m_fsModel;
};

}


#endif // PATHNAVIGATOR_H
