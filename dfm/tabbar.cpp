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

#include <QTextLayout>
#include <qmath.h>
#include <QMessageBox>
#include <QDrag>
#include <QToolBar>
#include <QToolButton>
#include <QWidget>
#include <QMouseEvent>
#include <QDrag>
#include <QStackedWidget>
#include <QApplication>
#include <QMimeData>
#include <QUrl>

#include "mainwindow.h"
#include "viewcontainer.h"
#include "widgets.h"
#include "tabbar.h"
#include "config.h"
#include "operations.h"
#include "iconprovider.h"
#include "iojob.h"
#include "globals.h"
#include "fsmodel.h"

using namespace DFM;

static QColor bg;
static QColor fg;
static QColor hl;
static QColor high;
static QColor low;

void
WindowFrame::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.translate(0.5f, 0.5f);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(fg);
    p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 2, 2);
    p.end();
}

WinButton::WinButton(Type t, QWidget *parent)
    : QFrame(parent)
    , m_hasPress(false)
    , m_type(t)
    , m_mainWin(MainWindow::currentWindow())
{
    switch (t)
    {
    case Min:
        connect(this, SIGNAL(clicked()), m_mainWin, SLOT(showMinimized()));
//        setProperty("type", "min");
        setObjectName("minButton");
        break;
    case Max:
        connect(this, SIGNAL(clicked()), this, SLOT(toggleMax()));
//        setProperty("type", "max");
        setObjectName("maxButton");
        break;
    case Close:
        connect(this, SIGNAL(clicked()), m_mainWin, SLOT(close()));
//        setProperty("type", "close");
        setObjectName("closeButton");
        break;
    default:break;
    }

    setFixedSize(16, 16);
    setAttribute(Qt::WA_Hover);
}

void
WinButton::mousePressEvent(QMouseEvent *e)
{
    m_hasPress = true;
    update();
    e->accept();
}

void
WinButton::mouseReleaseEvent(QMouseEvent *e)
{
    if (m_hasPress && rect().contains(e->pos()))
        emit clicked();
    m_hasPress = false;
    e->accept();
}

void
WinButton::mouseMoveEvent(QMouseEvent *e)
{
    e->accept();
}

//static uint fcolors[3] = {0xFFBF0303, 0xFFF3C300, 0xFF00892B};
//static uint fcolors[3] = { 0xFFD86F6B, 0xFFD8CA6B, 0xFF76D86B };
//static uint fcolors[3] = { 0xFFBF2929, 0xFF29BF29, 0xFFBFBF29 };

static uint fcolors[3] = { 0xFFE55E4F, 0xFFF8C76D, 0xFFA1D886 };

void
WinButton::paintEvent(QPaintEvent *e)
{
    if (!Store::config.styleSheet.isEmpty())
    {
        QFrame::paintEvent(e);
        return;
    }
    QPainter p(this);
    p.translate(0.5f, 0.5f);
    p.setRenderHint(QPainter::Antialiasing);

    QRect r(rect().adjusted(3, 3, -4, -4));
    QColor f(isActiveWindow() ? QColor(fcolors[m_type]) : fg);
    QColor mid(Ops::colorMid(f, Qt::black, 12, 10));
    QRadialGradient rg(r.center()+QPointF(0, (underMouse()&&!m_hasPress)?-1:1), (float)r.width()*0.7f);
    rg.setColorAt(0.0f, f);
    rg.setColorAt(1.0f, mid);

    int y = bg.value()>fg.value()?1:-1;

    p.setPen(Qt::NoPen);
    p.setBrush(y==1?high:low);
    p.drawEllipse(r.translated(0.0f, y));

    p.setPen(Ops::colorMid(f, Qt::black));
    p.setBrush(rg);
    p.drawEllipse(r);
    p.end();
}

void
WinButton::toggleMax()
{
    if (m_mainWin->isMaximized())
        m_mainWin->showNormal();
    else
        m_mainWin->showMaximized();
}

FooBar::FooBar(QWidget *parent)
    : QWidget(parent)
    , m_tabBar(0L)
    , m_topMargin(8)
    , m_mainWin(MainWindow::currentWindow())
    , m_toolBar(MainWindow::currentWindow()->toolBar())
    , m_hasPress(false)
    , m_pressPos(QPoint())
    , m_flags(Qt::Widget)
{
    m_mainWin->installEventFilter(this);
//    if (Store::config.behaviour.frame)
        m_frame = new WindowFrame(m_mainWin);
    m_toolBar->installEventFilter(this);
    m_toolBar->setAttribute(Qt::WA_NoSystemBackground);
    m_toolBar->setAutoFillBackground(false);
    setAttribute(Qt::WA_NoSystemBackground);
    m_flags = m_mainWin->windowFlags();
//    m_mainWin->setWindowFlags(Qt::Popup);
//    m_mainWin->setWindowFlags(Qt::);
}

int
FooBar::headHeight(MainWindow *win)
{
    int h = 0;
    if (FooBar *foo = win->findChild<FooBar *>())
        h += foo->height();
    if (QToolBar *bar = win->toolBar())
        if (bar->isVisible())
            h += bar->height();
    return h;
}

void
FooBar::setTabBar(TabBar *tabBar)
{
    m_tabBar = tabBar;
    QTimer::singleShot(200, this, SLOT(correctTabBarHeight()));
}

void
FooBar::correctTabBarHeight()
{
//    m_mainWin->setWindowFlags(m_flags);
    if (!m_mainWin->isVisible())
        m_mainWin->show();
    m_tabBar->setFixedHeight(m_tabBar->tabSizeHint(m_tabBar->currentIndex()).height());

    QPixmap confPix(16, 16);
    confPix.fill(Qt::transparent);
    QPainter p(&confPix);

    if (Store::config.behaviour.invertedColors)
    {
        fg = m_mainWin->palette().color(backgroundRole());
        bg = m_mainWin->palette().color(foregroundRole());
        fg.setHsv(fg.hue(), fg.saturation(), qBound(80, fg.value(), 220));
        bg.setHsv(bg.hue(), bg.saturation(), qBound(80, bg.value(), 220));
        QPalette tpal = m_toolBar->palette();
        tpal.setColor(m_toolBar->foregroundRole(), fg);
        tpal.setColor(m_toolBar->backgroundRole(), bg);
        m_toolBar->setPalette(tpal);
    }
    else
    {
        bg = m_mainWin->palette().color(backgroundRole());
        fg = m_mainWin->palette().color(foregroundRole());
    }

    hl = m_mainWin->palette().color(QPalette::Highlight);
    high = Ops::colorMid(bg, Qt::white);
    low = Ops::colorMid(bg, Qt::black);
    int y = bg.value() > fg.value() ? 1 : -1;
    QColor emboss(y==1?high:low);

    QPixmap emb(IconProvider::icon(IconProvider::Configure, 16, emboss, false).pixmap(16));
    QPixmap realPix(IconProvider::icon(IconProvider::Configure, 16, fg, false).pixmap(16));

    p.drawPixmap(confPix.rect().translated(0, y), emb);
    p.drawPixmap(confPix.rect(), realPix);
    p.end();

    Button *menu = new Button(this);
    menu->setFixedSize(16, 16);
    menu->setIcon(confPix);
    menu->setMenu(m_mainWin->mainMenu());

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QHBoxLayout *tl = new QHBoxLayout();
    tl->setContentsMargins(0, m_topMargin, 0, 0);
    tl->setSpacing(0);
    tl->addWidget(m_tabBar);
    if (Store::config.behaviour.windowsStyle)
    {
        layout->addWidget(menu);
        layout->addLayout(tl);
        layout->addWidget(new WinButton(WinButton::Min, this));
        layout->addSpacing(3);
        layout->addWidget(new WinButton(WinButton::Max, this));
        layout->addSpacing(3);
        layout->addWidget(new WinButton(WinButton::Close, this));
        layout->addSpacing(4);
    }
    else
    {
        layout->addSpacing(4);
        layout->addWidget(new WinButton(WinButton::Close, this));
        layout->addSpacing(3);
        layout->addWidget(new WinButton(WinButton::Min, this));
        layout->addSpacing(3);
        layout->addWidget(new WinButton(WinButton::Max, this));
        layout->addLayout(tl);
        layout->addWidget(menu);
        layout->addSpacing(4);
    }

    QFont font = m_tabBar->font();
    font.setPointSize(qMax(8, Store::config.behaviour.minFontSize));
    m_tabBar->setFont(font);
    m_tabBar->setAttribute(Qt::WA_Hover);
    m_tabBar->setMouseTracking(true);
    if (Store::config.behaviour.newTabButton)
        m_tabBar->setAddTabButton(new TabButton(m_tabBar));

    setLayout(layout);
    setContentsMargins(0, 0, 0, 0);
    setFixedHeight(m_tabBar->height()+m_topMargin);
    m_mainWin->setContentsMargins(0, height()+1/*(int)Store::config.behaviour.frame*/, 0, 0);
}

QPainterPath
FooBar::tab(const QRect &r, int round, TabShape shape)
{
    int x = r.x(), y = r.y(), w = x+(r.width()-1), h = y+(r.height()-1);
    QPainterPath path;
    if (shape == Standard)
    {
        path.moveTo(x, h);
        path.quadTo(x+(round), h, x+round, h-round);
        path.lineTo(x+round, y+round);
        path.quadTo(x+round, y, x+(round*2), y);
        path.lineTo(w-(round*2), y);
        path.quadTo(w-round, y, w-round, y+round);
        path.lineTo(w-round, h-round);
        path.quadTo(w-round, h, w, h);
    }
    else if (shape == Chrome)
    {
        int half = h/2, hf = round/2;
        path.moveTo(x, h);
        path.quadTo(x+hf, h, x+round, half);
        path.quadTo(x+round+hf, y, x+(round*2), y);
        path.lineTo(w-(round*2), y);
        path.quadTo(w-(round+hf), y, w-round, half);
        path.quadTo(w-hf, h, w, h);
    }
    return path;
}

QRegion
FooBar::shape()
{
    int w = m_mainWin->width(), h = m_mainWin->height();
    QRegion mask = QRegion(0, 2, w, h-4);
    mask += QRegion(2, 0, w-4, h);
    mask += QRegion(1, 1, w-2, h-2);
    return mask;
}

QLinearGradient
FooBar::headGrad(MainWindow *win)
{
    QWidget *w = MainWindow::currentWindow();
    QColor topMid(Ops::colorMid(Qt::white, bg, 1, 4));
    QColor bottomMid(Ops::colorMid(Qt::black, bg, 1, 4));
    QLinearGradient lg(0, 0, 0, headHeight(win));
    lg.setColorAt(0.0f, topMid);
    lg.setColorAt(1.0f, bottomMid);
    return lg;
}

void
FooBar::paintEvent(QPaintEvent *e)
{
    QPainter p(this);
    p.setBrushOrigin(rect().topLeft());
    p.setRenderHint(QPainter::Antialiasing);
    p.translate(0.5f, 0.5f);

    QLinearGradient lg(0, 0, 0, height());
    lg.setColorAt(0.0f, bg/*Operations::colorMid(bg, fg, 8, 1)*/);
    lg.setColorAt(1.0f, Ops::colorMid(bg, Qt::black, 6, 1));

    p.fillRect(rect().adjusted(-1, 0, 0, 0), lg);

    p.setPen(low);
    p.drawLine(0, height()-2, width(), height()-2);
    p.setPen(high);
    p.drawLine(0, height()-1, width(), height()-1);

    QPainterPath path;
    int r = 5, w = width()-1;
    path.moveTo(0, r);
    path.quadTo(0, 0, r, 0);
    path.lineTo(w-r, 0);
    path.quadTo(w, 0, w, r);
    path.moveTo(0, r);
    path.closeSubpath();

    QLinearGradient pg(0, 0, 0, r);
    pg.setColorAt(0.0f, high);
    pg.setColorAt(1.0f, Qt::transparent);

    p.setPen(QPen(pg, 1.0f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPath(path);

    p.end();
}

bool
FooBar::eventFilter(QObject *o, QEvent *e)
{
    if (o == m_mainWin
         && e->type() == QEvent::Resize)
    {
        resize(m_mainWin->width(), height());
//        if (Store::config.behaviour.frame)
//        {
            m_frame->resize(m_mainWin->size());
            QRegion mask = m_frame->rect();
            mask -= m_frame->rect().adjusted(1, 4, -1, -4);
            m_frame->setMask(mask);
            move(0, 1);
//        }
        m_mainWin->setMask(shape());
        return false;
    }
    if (o == m_toolBar && e->type() == QEvent::Paint)
    {
        QPainter p(m_toolBar);
        p.setBrushOrigin(0, -m_tabBar->height());
        p.fillRect(m_toolBar->rect(), headGrad(MainWindow::window(m_toolBar)));
        p.setPen(low);
        p.drawLine(m_toolBar->rect().bottomLeft(), m_toolBar->rect().bottomRight());
        p.end();
        return true;
    }
    return QWidget::eventFilter(o, e);
}

void
FooBar::mousePressEvent(QMouseEvent *e)
{
    m_hasPress = true;
    m_pressPos = e->globalPos();
    e->accept();
}

void
FooBar::mouseReleaseEvent(QMouseEvent *e)
{
    m_hasPress = false;
    m_pressPos = QPoint();
    e->accept();
}

void
FooBar::mouseMoveEvent(QMouseEvent *e)
{
    if (m_hasPress)
    {
        m_mainWin->move(m_mainWin->pos()+(e->globalPos()-m_pressPos));
        m_pressPos = e->globalPos();
    }
}

static QImage closer[2];

static QImage closeImg(QColor color)
{
    QImage img(QSize(16, 16), QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.translate(0.5f, 0.5f);
    p.setPen(QPen(color, 2.0f));
    int a = 4;
    p.setClipRect(img.rect().adjusted(0, a, 0, -(a+2)));
    p.drawLine(img.rect().topLeft(), img.rect().bottomRight());
    p.drawLine(img.rect().topRight(), img.rect().bottomLeft());
    p.end();
    return img;
}

void
TabCloser::paintEvent(QPaintEvent *)
{
    if (closer[0].isNull())
        for (int i = 0; i < 2; ++i)
    {
        int y = bg.value()>fg.value()?1:-1;
        closer[i] = QImage(QSize(16, 16), QImage::Format_ARGB32);
        closer[i].fill(Qt::transparent);
        QPainter p(&(closer[i]));
        p.drawImage(closer[i].rect().translated(0, y), closeImg(y==1?high:low));
        p.drawImage(closer[i].rect(), closeImg(i?hl:fg));
        p.end();
    }

    QPainter pt(this);
    pt.drawImage(rect(), closer[underMouse()]);
    pt.end();
}

TabButton::TabButton(QWidget *parent)
    : WinButton(WinButton::Other, parent)
{
    if (Store::config.behaviour.gayWindow)
        setFixedSize(28, 18);

    connect(MainWindow::window(this), SIGNAL(settingsChanged()), this, SLOT(regenPixmaps()));
}

void
TabButton::paintEvent(QPaintEvent *e)
{
    QPainter p(this);
    if (Store::config.behaviour.gayWindow)
    {
        p.translate(0.5f, 0.5f);
        p.setRenderHint(QPainter::Antialiasing);
        int y = bg.value()>fg.value()?1:-1;
        QColor emb(Ops::colorMid(bg, /*y==1 ? */Qt::white /*: Qt::black*/, 2, 1));

        if (Store::config.behaviour.tabShape == FooBar::Chrome)
        {
            QRect pr(rect().adjusted(1, 1, -1, -1));
            int px = pr.x(), py = pr.y(), pw = pr.width()-1, ph = pr.height()-1;
            QPainterPath path(QPoint(px+pw*0.05f, py));
            path.lineTo(px+pw*0.75f, py);
            path.quadTo(px+pw*0.85f, py, px+pw, py+ph*0.95f);
            path.quadTo(px+pw, py+ph, px+pw*0.95f, py+ph);
            path.lineTo(px+pw*0.25f, py+ph);
            path.quadTo(px+pw*0.15f, py+ph, px, py+ph*0.05f);
            path.quadTo(px, py, px+pw*0.05f, py);
            path.closeSubpath();

            p.setPen(emb);
            p.drawPath(path.translated(0,1));

            QLinearGradient it(pr.topLeft(), pr.bottomLeft());
            it.setColorAt(0.0f, QColor(255,255,255,underMouse()?127:63));
            it.setColorAt(1.0f, Qt::transparent);

            p.setBrush(it);
            p.setPen(low);
            p.drawPath(path);
        }

        QRect vert(QPoint(0,0), QSize(2, 8));
        vert.moveCenter(rect().center());
        QRect hor(QPoint(0,0), QSize(8, 2));
        hor.moveCenter(rect().center());
        if (y == 1)
        {
            vert.translate(0, -1);
            hor.translate(0, -1);
        }

        emb = y==1?high:low;
        p.setBrush(emb);
        p.setPen(emb);
        p.drawRect(vert.translated(0, y));
        p.drawRect(hor.translated(0, y));

        p.setPen(underMouse()?hl:fg);
        p.setBrush(underMouse()?hl:fg);
        p.drawRect(vert);
        p.drawRect(hor);
    }
    else
    {
        p.drawTiledPixmap(rect().translated(2, 0), m_pix[underMouse()]);
    }
    p.end();
}

void
TabButton::resizeEvent(QResizeEvent *e)
{
    WinButton::resizeEvent(e);
    if (!Store::config.behaviour.gayWindow && e->size().height() != e->oldSize().height())
        regenPixmaps();
}

void
TabButton::regenPixmaps()
{
    QStyleOptionTabV3 tab;
    tab.state = QStyle::State_Enabled;
    tab.rect = rect();
    const bool sp(style()->objectName().compare("styleproject", Qt::CaseInsensitive) == 0);
    if (!sp)
        tab.position = QStyleOptionTab::OnlyOneTab;
    else
        tab.position = QStyleOptionTab::End;

    for (int i = 0; i < 2; ++i)
    {
        m_pix[i] = QPixmap(rect().size());
        m_pix[i].fill(Qt::transparent);
    }
    QPainter p(&m_pix[0]);
//    QRect iconRect(0, 0, 16, 16);
//    iconRect.moveCenter(rect().center());
    const QIcon &icon = IconProvider::icon(IconProvider::NewTab, 16, Ops::colorMid(palette().color(foregroundRole()), palette().color(backgroundRole()), 4, 1), Store::config.behaviour.systemIcons);
    icon.paint(&p, rect(), Qt::AlignCenter, QIcon::Normal);
    p.end();
    p.begin(&m_pix[1]);
    tab.state |= QStyle::State_MouseOver;
//    style()->drawControl(QStyle::CE_TabBarTab, &tab, &p, parentWidget());
    const QIcon &activeIcon = IconProvider::icon(IconProvider::NewTab, 16, palette().color(QPalette::Highlight), Store::config.behaviour.systemIcons);
    activeIcon.paint(&p, rect(), Qt::AlignCenter, QIcon::Selected, QIcon::On);
    p.end();
}

void
DropIndicator::paintEvent(QPaintEvent *)
{
    QLinearGradient lg(rect().topLeft(), rect().topRight());
    lg.setColorAt(0.0f, Qt::transparent);
    lg.setColorAt(0.5f, Store::config.behaviour.gayWindow ? hl : palette().color(QPalette::Highlight));
    lg.setColorAt(1.0f, Qt::transparent);

    QPainter p(this);
    p.fillRect(rect(), lg);
    p.end();
}

TabBar::TabBar(QWidget *parent)
    : QTabBar(parent)
    , m_addButton(0)
    , m_dropIndicator(new DropIndicator(this))
    , m_filteringEvents(false)
    , m_closeButton(0)
    , m_layout(new QHBoxLayout())
{
    setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
    setDocumentMode(true);
    if (!Store::config.behaviour.gayWindow)
        setTabsClosable(true);
    else
        setAttribute(Qt::WA_NoSystemBackground);
    setMovable(false);
    setDrawBase(true);
    setExpanding(!Store::config.behaviour.gayWindow);
    setElideMode(Qt::ElideRight);
    setAcceptDrops(true);
    m_dropIndicator->setFixedWidth(8);
    m_dropIndicator->setVisible(false);

    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    m_layout->addStretch();

    if (!Store::config.behaviour.gayWindow)
    {
        setTabCloseButton(new QToolButton());
        setAddTabButton(new TabButton(this));
        QTimer::singleShot(0, this, SLOT(postConstructorOps()));
    }
    connect(MainWindow::window(this), SIGNAL(settingsChanged()), this, SLOT(postConstructorOps()));
    QAction *closeCurrent = new QAction(tr("Close current tab"), this);
    connect(closeCurrent, SIGNAL(triggered()), this, SLOT(closeCurrent()));
    addAction(closeCurrent);
    closeCurrent->setShortcut(QKeySequence(QString("Ctrl+W")));

    setLayout(m_layout);
    QTimer::singleShot(1000, this, SLOT(correctAddButtonPos()));
}

void
TabBar::postConstructorOps()
{
//    m_closeButton->setAutoRaise(true);
//    m_closeButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    const QIcon &icon = IconProvider::icon(IconProvider::CloseTab, 16, palette().color(foregroundRole()), Store::config.behaviour.systemIcons);
    if (m_closeButton)
    {
        m_closeButton->setIcon(icon);
        m_closeButton->setVisible(Store::config.behaviour.showCloseTabButton);
    }
}

void
TabBar::setTabCloseButton(QToolButton *closeButton)
{
    QWidget *w(new QWidget());
    w->setContentsMargins(0, 0, 0, 0);
    QVBoxLayout *l(new QVBoxLayout());
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);
    m_closeButton = closeButton;
    l->addWidget(m_closeButton);
    connect(closeButton, SIGNAL(clicked()), this, SLOT(closeCurrent()));
    w->setLayout(l);
    m_layout->addWidget(w);
}

void
TabBar::setAddTabButton(TabButton *addButton)
{
    m_addButton = addButton;
    connect(addButton, SIGNAL(clicked()), this, SIGNAL(newTabRequest()));
    addButton->show();
}
void
TabBar::correctAddButtonPos()
{
    if (!m_addButton || !count())
        return;

    int x = tabRect(count()-1).right();
//    if (!style()->objectName().contains("styleproject", Qt::CaseInsensitive))
//        x+=Store::config.behaviour.tabOverlap/2;
//    else
    if (Store::config.behaviour.gayWindow)
        x+=Store::config.behaviour.tabRoundness;
//    else
//        x-=4;
    int y = qFloor((float)rect().height()/2.0f-(float)m_addButton->height()/2.0f);
    m_addButton->move(x, y);
}

void
TabBar::closeCurrent()
{
    if (count() < 2)
        return;

    emit tabCloseRequested(currentIndex());
}

void
TabBar::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->source() == this && e->mimeData()->property("tab").isValid())
        m_dropIndicator->show();
    m_pressPos = QPoint();
    if (e->mimeData()->hasUrls())
        e->acceptProposedAction();
}

void
TabBar::dragMoveEvent(QDragMoveEvent *e)
{
    if (e->source() == this && !m_filteringEvents)
    {
        m_filteringEvents = true;
        qApp->installEventFilter(this);
    }
    if (e->mimeData()->property("tab").isValid() && tabAt(e->pos()) != -1)
    {
        QRect r = tabRect(tabAt(e->pos()));
        m_dropIndicator->setVisible(true);
        m_dropIndicator->move(e->pos().x() > r.center().x() ? r.topRight()-QPoint(3, 0) : r.topLeft()-QPoint(4, 0));
    }
}

void
TabBar::dragLeaveEvent(QDragLeaveEvent *e)
{
    m_dropIndicator->hide();
    if (!m_filteringEvents)
    {
        m_filteringEvents = true;
        qApp->installEventFilter(this);
    }
}

void
TabBar::dropEvent(QDropEvent *e)
{
    m_dropIndicator->setVisible(false);
    m_pressPos = QPoint();
    QApplication::restoreOverrideCursor();
    MainWindow *w = MainWindow::window(this);
    int tab = tabAt(e->pos());
    QRect r = tabRect(tab);
    if (e->mimeData()->property("tab").isValid()) //dragging a tab
    {
        if (e->source() == this && r.isValid()) //dragging a tab inside tabbar
        {
            int fromTab = e->mimeData()->property("tab").toInt();
            int toTab = tab;
            if (tab > fromTab && e->pos().x() < r.center().x())
                --toTab;
            else if (tab < fromTab && e->pos().x() > r.center().x())
                ++toTab;
            if (fromTab != toTab)
                moveTab(fromTab, toTab);
        }
        else if (e->source() != this) //from a tabbar in another window
        {
            int toTab = r.isValid()?tab:count();
            if (r.isValid() && e->pos().x() > r.center().x())
                ++toTab;
            MainWindow *sourceWin = MainWindow::window(static_cast<QWidget *>(e->source()));
            ViewContainer *container = sourceWin->takeContainer(e->mimeData()->property("tab").toInt());
            w->addTab(container, toTab);
        }
    }
    else
    {
        if (r.isValid())
        {
            const QString &dest = w->containerForTab(tab)->model()->rootUrl().toLocalFile();
            if (QFileInfo(dest).exists())
            {
                QMenu m;
                QAction *move = m.addAction(QString("move to %1").arg(dest));
                m.addAction(QString("copy to %1").arg(dest));
                if (QAction *a = m.exec(QCursor::pos()))
                {
                    const bool cut(a == move);
                    IO::Manager::copy(e->mimeData()->urls(), dest, cut, cut);
                }
            }
        }
        else
        {
            if (e->mimeData()->urls().count() == 1 || QMessageBox::question(w, tr("Are you sure?"), QString(tr("You are about to open %1 tabs").arg(e->mimeData()->urls().count())), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
                foreach (const QUrl &file, e->mimeData()->urls())
                    if (QFileInfo(file.toLocalFile()).isDir())
                        w->addTab(file);
        }
    }
    e->accept();
}

void
TabBar::showEvent(QShowEvent *e)
{
    QTabBar::showEvent(e);
    QTimer::singleShot(250, this, SLOT(correctAddButtonPos()));
}

void
TabBar::resizeEvent(QResizeEvent *e)
{
    QTabBar::resizeEvent(e);
    if (!Store::config.behaviour.gayWindow && m_addButton)
        m_addButton->setFixedSize(16, height());

    QTimer::singleShot(0, this, SLOT(correctAddButtonPos()));
}

void
TabBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    m_pressPos = QPoint();
    QTabBar::mouseDoubleClickEvent(event);
    if (rect().contains(event->pos()) && event->button() == Qt::LeftButton)
    {
        emit newTabRequest();
        event->accept();
        return;
    }
}

void
TabBar::mousePressEvent(QMouseEvent *e)
{
    e->accept();
    if (e->button() == Qt::LeftButton)
    {
        m_pressPos = e->pos();
        if (FooBar *bar = MainWindow::window(this)->findChild<FooBar *>())
            QCoreApplication::sendEvent(bar, e);
    }
}

void
TabBar::newWindowTab(int tab)
{
    ViewContainer *container(MainWindow::window(this)->takeContainer(tab));
    if (!container)
        return;
    MainWindow *win = new MainWindow(QStringList(), false);
    win->addTab(container);
    win->show();
}

bool
TabBar::eventFilter(QObject *o, QEvent *e)
{
    if (e->type()==QEvent::KeyPress)
        if (static_cast<QKeyEvent *>(e)->key()==Qt::Key_Escape)
            m_dragCancelled=true;
    return QTabBar::eventFilter(o, e);
}

void
TabBar::mouseMoveEvent(QMouseEvent *e)
{
    QTabBar::mouseMoveEvent(e);
    if (Store::config.behaviour.gayWindow)
    {
        int t = tabAt(e->pos());
        if (tabRect(t).isValid())
            m_hoveredTab = t;
        else
            m_hoveredTab = -1;
        update();
    }
    if (!m_pressPos.isNull()
            && QPoint(m_pressPos-e->pos()).manhattanLength() > QApplication::startDragDistance()
            && tabAt(e->pos()) != -1)
    {
        m_dragCancelled = false;
        QDrag *drag = new QDrag(this);
        connect(drag, SIGNAL(destroyed()), m_dropIndicator, SLOT(hide()));
//        connect(drag, SIGNAL(targetChanged(QWidget*)), this, SLOT(drag()));
        QMimeData *data = new QMimeData();
        m_lastDraggedFile = MainWindow::window(this)->containerForTab(tabAt(e->pos()))->model()->rootUrl().toLocalFile();
        data->setUrls(QList<QUrl>() << QUrl::fromLocalFile(m_lastDraggedFile));
        data->setProperty("tab", tabAt(e->pos()));
        drag->setMimeData(data);
        QWidget *w = MainWindow::currentWindow()->containerForTab(tabAt(e->pos()));
        if (tabAt(e->pos()) != currentIndex())
            w->resize(MainWindow::currentWindow()->containerForTab(currentIndex())->size());
        QPixmap pix(w->size());
        w->render(&pix);
        pix = pix.scaledToHeight(128);
        drag->setPixmap(pix);
        if (!drag->exec(Qt::CopyAction|Qt::MoveAction))
            if (count() > 1 && !m_dragCancelled) //no idea how to actually get the dragcancel 'event'
                newWindowTab(data->property("tab").toInt());
        if (m_filteringEvents)
            qApp->removeEventFilter(this);
        m_filteringEvents = false;
    }
    if (FooBar *bar = MainWindow::window(this)->findChild<FooBar *>())
        QCoreApplication::sendEvent(bar, e);
    if (m_dropIndicator->isVisible())
        m_dropIndicator->setVisible(false);
}

void
TabBar::leaveEvent(QEvent *e)
{
    QTabBar::leaveEvent(e);
    if (Store::config.behaviour.gayWindow)
    {
        m_hoveredTab = -1;
        update();
    }
    if (!m_pressPos.isNull())
        m_pressPos = QPoint();
}

void
TabBar::mouseReleaseEvent(QMouseEvent *e)
{
    e->accept();
    if (!m_pressPos.isNull() && QPoint(m_pressPos-e->pos()).manhattanLength() < QApplication::startDragDistance() && tabAt(e->pos()) != -1)
        setCurrentIndex(tabAt(e->pos()));
    m_pressPos = QPoint();
    if (e->button() == Qt::MiddleButton && tabAt(e->pos()) > -1 && count() > 1)
        emit tabCloseRequested(tabAt(e->pos()));
}

void
TabBar::tabCloseRequest()
{
    TabCloser *tc = static_cast<TabCloser *>(sender());
    int index = tabAt(tc->geometry().center());
    if (count() > 1)
        emit tabCloseRequested(index);
}

void
TabBar::drawTab(QPainter *p, int index)
{
    QRect r(tabRect(index));
    QString s(tabText(index));
    if (!r.isValid())
        return;

    FooBar::TabShape tabShape = (FooBar::TabShape)Store::config.behaviour.tabShape;
    int rndNess = Store::config.behaviour.tabRoundness;
    int overlap = Store::config.behaviour.tabOverlap;
    QRect shape(r.adjusted(-overlap, 1, overlap, 0));

    if (shape.left() < rect().left())
        shape.setLeft(rect().left());
    if (shape.right() > rect().right())
        shape.setRight(rect().right());

    p->setPen(QPen(low, 3.0f));
    p->drawPath(FooBar::tab(shape, rndNess, tabShape)); //dark frame on all tabs

    if (index == currentIndex())
    {
        p->setPen(high);
        p->drawLine(rect().bottomLeft(), rect().bottomRight());
        p->setPen(QPen(FooBar::headGrad(MainWindow::window(this)), 1.0f));
        p->drawLine(shape.bottomLeft(), shape.bottomRight());
        p->setBrush(FooBar::headGrad(MainWindow::window(this)));

        QLinearGradient hg(shape.topLeft(), shape.bottomLeft());
        hg.setColorAt(0.0f, Ops::colorMid(high, Qt::white));
        hg.setColorAt(1.0f, high);

        p->setPen(QPen(hg, 1.5f));
        p->drawPath(FooBar::tab(shape, rndNess, tabShape));
    }
    else
    {
        QLinearGradient it(r.topLeft(), r.bottomLeft());
        it.setColorAt(0.0f, index==m_hoveredTab ? Ops::colorMid(bg, Qt::white, 5, 1) : bg);
        it.setColorAt(1.0f, Ops::colorMid(bg, Qt::black, index==m_hoveredTab ? 15 : 10, 1));

        QColor h = high;
        h.setAlpha(bg.value());
        QLinearGradient hg(shape.topLeft(), shape.bottomLeft());
        hg.setColorAt(0.0f, Ops::colorMid(h, Qt::white));
        hg.setColorAt(1.0f, h);

        p->setPen(QPen(hg, 1.5f));
        p->setBrush(it);
        p->drawPath(FooBar::tab(shape.adjusted(0, 0, 0, 0), rndNess, tabShape));
    }

    //icon

    int leftMargin = 2;
    QPixmap pix = tabIcon(index).pixmap(16);
    style()->drawItemPixmap(p, QRect(r.x() == rect().x() ? overlap+leftMargin : r.x()+leftMargin, r.y(), 16, r.bottom()-1), Qt::AlignCenter, pix);
//    tabIcon(index).paint(p, QRect(r.x() == rect().x() ? overlap+leftMargin : r.x()+leftMargin, r.y(), 16, r.bottom()-1));

    //text

    QFont f(font());
    f.setBold(index == currentIndex());
    p->setFont(f);

    r.setRight(tabButton(index, RightSide)->geometry().x());
    r.setLeft(r.x() == rect().x() ? r.left()+20+overlap+leftMargin : r.left()+20+leftMargin);

    int y = bg.value() > fg.value() ? 1 : -1;
    QColor emboss(Ops::colorMid(bg, y==1 ? Qt::white : Qt::black, 2, 1));

    QLinearGradient embGrad(r.topLeft(), r.topRight());
    embGrad.setColorAt(0, emboss);
    embGrad.setColorAt(0.75, emboss);
    embGrad.setColorAt(1, Qt::transparent);

    p->setPen(QPen(embGrad, 1.0f));
    p->drawText(r.translated(0, y), /*l > r.width() ?*/ Qt::AlignLeft|Qt::AlignVCenter /*: Qt::AlignCenter*/, s);

    QLinearGradient fgGrad(r.topLeft(), r.topRight());
    fgGrad.setColorAt(0, fg);
    fgGrad.setColorAt(0.75, fg);
    fgGrad.setColorAt(1, Qt::transparent);

    p->setPen(QPen(fgGrad, 1.0f));
    p->drawText(r, /*l > r.width() ?*/ Qt::AlignLeft|Qt::AlignVCenter /*: Qt::AlignCenter*/, s);
}

void
TabBar::paintEvent(QPaintEvent *event)
{
    if (!Store::config.behaviour.gayWindow)
    {
        QTabBar::paintEvent(event);
        return;
    }

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.translate(0.5f, 0.5f);

    for (int i = 0; i < currentIndex(); ++i)
        drawTab(&p, i);
    for (int i = count(); i > currentIndex(); --i)
        drawTab(&p, i);
    p.setPen(low);
    p.drawLine(0, height()-2, width(), height()-2);
    drawTab(&p, currentIndex());
    p.end();
}

QSize
TabBar::tabSizeHint(int index) const
{
    if (Store::config.behaviour.gayWindow)
        return QSize(qMin(Store::config.behaviour.tabWidth,(width()/count())-(m_addButton?qCeil(((float)m_addButton->width()/(float)count())+2):0)), Store::config.behaviour.tabHeight/*QTabBar::tabSizeHint(index).height()+3*/);
//    if (expanding())
        return QTabBar::tabSizeHint(index);

//    int w = width();
//    if (m_addButton && m_addButton->isVisible())
//        w -= m_addButton->width();
//    if (m_closeButton && m_closeButton->isVisible())
//        w -= m_closeButton->width();
//    return QSize(qMin(150, w/count()), QTabBar::tabSizeHint(index).height());
}

void
TabBar::tabInserted(int index)
{
    QTabBar::tabInserted(index);
    if (Store::config.behaviour.gayWindow)
    {
        TabCloser *tc = new TabCloser();
        connect(tc, SIGNAL(clicked()), this, SLOT(tabCloseRequest()));
        setTabButton(index, RightSide, tc);
    }
//    QMetaObject::invokeMethod(this, "correctAddButtonPos", Qt::QueuedConnection);
//    correctAddButtonPos();
    QTimer::singleShot(50, this, SLOT(correctAddButtonPos()));
}

void
TabBar::tabRemoved(int index)
{
    QTabBar::tabRemoved(index);
//    if (Store::config.behaviour.gayWindow)
        correctAddButtonPos();
}

//-------------------------------------------------------------------------------------------

TabManager::TabManager(MainWindow *mainWin, QWidget *parent)
    : QStackedWidget(parent)
    , m_mainWin(mainWin)
    , m_tabBar(0)
{

}

void
TabManager::deleteTab(const int tab)
{
    if (count() <= 1)
        return;
    if (ViewContainer *c = takeTab(tab))
        c->deleteLater();
    setCurrentIndex(m_tabBar->currentIndex());
    window()->setWindowTitle(m_tabBar->tabText(m_tabBar->currentIndex()));
}

ViewContainer
*TabManager::takeTab(const int tab)
{
    if (ViewContainer *c = static_cast<ViewContainer *>(widget(tab)))
    {
        blockSignals(true);
        removeWidget(c);
        blockSignals(false);
        m_tabBar->blockSignals(true);
        m_tabBar->removeTab(tab);
        m_tabBar->blockSignals(false);
        m_tabBar->setVisible(m_tabBar->count() > 1 || !Store::config.behaviour.hideTabBarWhenOnlyOneTab);
        return c;
    }
    return 0;
}

ViewContainer
*TabManager::forTab(const int tab)
{
    if (ViewContainer *c = static_cast<ViewContainer *>(widget(tab)))
        return c;
    return 0;
}

void
TabManager::setTabBar(TabBar *tabBar)
{
    m_tabBar = tabBar;
    connect(m_tabBar, SIGNAL(tabMoved(int,int)), this, SLOT(tabMoved(int, int)));
    connect(m_tabBar, SIGNAL(currentChanged(int)), this, SLOT(setCurrentIndex(int)));
    connect(m_tabBar, SIGNAL(currentChanged(int)), this, SIGNAL(currentTabChanged(int)));
    connect(m_tabBar, SIGNAL(newTabRequest()), this, SIGNAL(newTabRequest()));
    connect(m_tabBar, SIGNAL(tabCloseRequested(int)), this, SIGNAL(tabCloseRequested(int)));
}

void
TabManager::tabMoved(int from, int to)
{
    blockSignals(true);
    QWidget *w = widget(from);
    removeWidget(w);
    insertWidget(to, w);
    setCurrentIndex(m_tabBar->currentIndex());
    blockSignals(false);
}

int
TabManager::insertTab(int index, ViewContainer *c, const QIcon &icon, const QString &text)
{
    if (!c)
        return -1;
    index = insertWidget(index, c);
    index = m_tabBar->insertTab(index, icon, text);
    m_tabBar->setVisible(m_tabBar->count() > 1 || !Store::config.behaviour.hideTabBarWhenOnlyOneTab);
    return index;
}

int
TabManager::addTab(ViewContainer *c, const QIcon &icon, const QString &text)
{
    return insertTab(-1, c, icon, text);
}

