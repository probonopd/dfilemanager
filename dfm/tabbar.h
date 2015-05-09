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


#ifndef TABBAR_H
#define TABBAR_H

#include <QTabBar>
#include <QFrame>
#include <QStackedWidget>

class QHBoxLayout;
class QToolBar;
class QToolButton;
namespace DFM
{
class MainWindow;
class ViewContainer;

class WindowFrame : public QWidget
{
public:
    inline explicit WindowFrame(QWidget *parent = 0) : QWidget(parent) {setAttribute(Qt::WA_TransparentForMouseEvents);}
    ~WindowFrame(){}

protected:
    void paintEvent(QPaintEvent *);

};

class WinButton : public QFrame
{
    Q_OBJECT
public:
    enum Type { Close = 0, Min, Max, Other };
    WinButton(Type t = Other, QWidget *parent = 0);
    ~WinButton(){}

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);

private slots:
    void toggleMax();

private:
    Type m_type;
    MainWindow *m_mainWin;
    bool m_hasPress;
};

class TabBar;

class FooBar : public QWidget
{
    Q_OBJECT
public:
    enum TabShape { Standard = 0, Chrome };
    explicit FooBar(QWidget *parent = 0);
    ~FooBar(){}

    void setTabBar(TabBar *tabBar);
    static int headHeight(MainWindow *win);
    static QLinearGradient headGrad(MainWindow *win);
    static QPainterPath tab(const QRect &r, int round = 4, TabShape shape = Standard);

protected:
    bool eventFilter(QObject *o, QEvent *e);
    void paintEvent(QPaintEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    QRegion shape();

private slots:
    void correctTabBarHeight();

private:
    TabBar *m_tabBar;
    QToolBar *m_toolBar;
    int m_topMargin;
    bool m_hasPress;
    QPoint m_pressPos;
    MainWindow *m_mainWin;
    WindowFrame *m_frame;
    Qt::WindowFlags m_flags;
    friend class TabBar;
};

class TabCloser : public WinButton
{
public:
    inline explicit TabCloser(QWidget *parent = 0) : WinButton(WinButton::Other, parent) {}
    ~TabCloser(){}

protected:
    void paintEvent(QPaintEvent *);
};

class TabButton : public WinButton
{
    Q_OBJECT
public:
    explicit TabButton(QWidget *parent = 0);
    ~TabButton(){}

protected:
    void paintEvent(QPaintEvent *e);
    void resizeEvent(QResizeEvent *);

private slots:
    void regenPixmaps();

private:
    QPixmap m_pix[2];
};

class DropIndicator : public QWidget
{
public:
    inline explicit DropIndicator(QWidget *parent = 0):QWidget(parent){}
    ~DropIndicator(){}
protected:
    void paintEvent(QPaintEvent *);
};

class TabBar : public QTabBar
{
    Q_OBJECT
public:
    explicit TabBar(QWidget *parent = 0);
    ~TabBar(){}
    void setAddTabButton(TabButton *addButton);
    void setTabCloseButton(QToolButton *closeButton);
    
signals:
    void newTabRequest();

protected:
    QSize tabSizeHint(int index) const;
    void mousePressEvent(QMouseEvent *e);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *event);
    void tabInserted(int index);
    void tabRemoved(int index);
    void mouseMoveEvent(QMouseEvent *e);
    void leaveEvent(QEvent *e);
    void resizeEvent(QResizeEvent *e);
    void showEvent(QShowEvent *);
    void dragEnterEvent(QDragEnterEvent *e);
    void dragLeaveEvent(QDragLeaveEvent *e);
    void dragMoveEvent(QDragMoveEvent *e);
    void dropEvent(QDropEvent *e);
    bool eventFilter(QObject *, QEvent *);
    void drawTab(QPainter *p, int index);

private slots:
    void tabCloseRequest();
    void newWindowTab(int tab);
    void closeCurrent();
    void postConstructorOps();
    void correctAddButtonPos();

private:
    friend class FooBar;
    int m_hoveredTab;
    bool m_dragCancelled, m_filteringEvents;
    QHBoxLayout *m_layout;
    TabButton *m_addButton;
    QToolButton *m_closeButton;
    DropIndicator *m_dropIndicator;
    QString m_lastDraggedFile;
    QPoint m_pressPos;
};

class TabManager : public QStackedWidget
{
    Q_OBJECT
public:
    explicit TabManager(MainWindow *mainWin = 0, QWidget *parent = 0);
    ~TabManager(){}

    inline TabBar *tabBar() { return m_tabBar; }
    void setTabBar(TabBar *tabBar);
    int addTab(ViewContainer *c, const QIcon& icon = QIcon(), const QString &text = QString());
    int insertTab(int index, ViewContainer *c, const QIcon& icon = QIcon(), const QString &text = QString());
    ViewContainer *forTab(const int tab);
    ViewContainer *takeTab(const int tab);
    void deleteTab(const int tab);

signals:
    void currentTabChanged(int tab);
    void newTabRequest();
    void tabCloseRequested(int tab);

private slots:
    void tabMoved(int from, int to);

private:
    TabBar *m_tabBar;
    MainWindow *m_mainWin;
};


}

#endif // TABBAR_H
