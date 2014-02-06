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
#include <QBoxLayout>

#include "filesystemmodel.h"
#include "operations.h"
#include "widgets.h"

namespace DFM
{
class PathNavigator;
class BreadCrumbs;
class PathBox;
class NavButton : public QWidget
{
    Q_OBJECT
public:
    NavButton(QWidget *parent = 0, const QUrl &url = QUrl(), const QString &text = QString());
    QSize sizeHint() const;
    QSize minimumSizeHint() const;

signals:
    void urlRequest(const QUrl &url);
    void released();

protected:
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void leaveEvent(QEvent *);
    void dragEnterEvent(QDragEnterEvent *);
    void dragLeaveEvent(QDragLeaveEvent *);
    void dropEvent(QDropEvent *);
    void paintEvent(QPaintEvent *);

private:
    QString m_text;
    QUrl m_url;
    PathNavigator *m_nav;
    bool m_hasData, m_hasPress;
    int m_margin;
};

//-----------------------------------------------------------------------------

class Menu : public QMenu
{
    Q_OBJECT
public:
    inline explicit Menu(QWidget *parent = 0) : QMenu(parent) {}
protected:
    virtual void mousePressEvent(QMouseEvent *e);
signals:
    void newTabRequest(const QString &path);
};

//-----------------------------------------------------------------------------

class PathSeparator : public QWidget
{
    Q_OBJECT
public:
    PathSeparator(QWidget *parent, const QUrl &url = QUrl(), FS::Model *fsModel = 0);

protected:
    void mousePressEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *);
    void leaveEvent(QEvent *e) {QWidget::leaveEvent(e); update(); }

private:
    QUrl m_url;
    PathNavigator *m_nav;
    FS::Model *m_fsModel;
};

//-----------------------------------------------------------------------------

class PathBox : public QComboBox
{
    Q_OBJECT
public:
    inline explicit PathBox(QWidget *parent = 0) : QComboBox(parent) {}

signals:
    void cancelEdit();

protected:
    void keyPressEvent(QKeyEvent *e) { if (e->key() == Qt::Key_Escape) emit cancelEdit(); else QComboBox::keyPressEvent(e); }
    void paintEvent(QPaintEvent *);
};

//-----------------------------------------------------------------------------

class PathNavigator : public QWidget
{
    Q_OBJECT
public:
    explicit PathNavigator(QWidget *parent = 0, FS::Model *model = 0);
    inline QUrl url() const { return m_url; }
    inline FS::Model *model() { return m_fsModel; }
    inline QList<NavButton *> navButtons() { return QList<NavButton *>(findChildren<NavButton *>()); }
    inline int count() { return navButtons().count(); }
    QSize sizeHint() const;
    
signals:
    void edit();

protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void resizeEvent(QResizeEvent *);
    void clear();

private slots:
     void genNavFromUrl(const QUrl &url);
     void postConstructorJobs();

private:
    QUrl m_url;
    QStringList m_pathList;
    FS::Model *m_fsModel;
    QHBoxLayout *m_layout;
    QList<QWidget *> m_widgets;
    BreadCrumbs *m_bc;
    Button *m_schemeButton;
    bool m_hasPress;
};

//-----------------------------------------------------------------------------

class BreadCrumbs : public QStackedWidget
{
    Q_OBJECT
public:
    explicit BreadCrumbs(QWidget *parent = 0, FS::Model *fsModel = 0);
    inline void setEditable(const bool editable) { setCurrentWidget(editable ? static_cast<QWidget *>(m_pathBox) : static_cast<QWidget *>(m_pathNav)); currentWidget()->setFocus(); }
    inline QUrl url() { return m_pathNav->url(); }
    inline bool isEditable() { return currentWidget() == static_cast<QWidget *>(m_pathBox); }
    inline PathNavigator *pathNav() { return m_pathNav; }
    inline PathBox *pathBox() { return m_pathBox; }
    QSize sizeHint() const;

public slots:
    void urlFromEdit(const QString &urlString);
    inline void toggleEditable() { currentWidget() == m_pathNav ? setEditable(true) : setEditable(false); }
    void complete(const QString &path);
    void setUrl(const QUrl &url);
    void paletteOps();

signals:
    void newPath(const QString &path);

private:
    PathBox *m_pathBox;
    PathNavigator *m_pathNav;
    FS::Model *m_fsModel;
};

}


#endif // PATHNAVIGATOR_H
