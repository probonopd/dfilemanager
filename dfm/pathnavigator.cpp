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


#include "pathnavigator.h"
#include "mainwindow.h"
#include "iojob.h"
#include "iconprovider.h"
#include <QDebug>
#include <QCompleter>
#include <QDirModel>

using namespace DFM;

PathSeparator::PathSeparator(QWidget *parent, const QUrl &url, FS::Model *fsModel)
    : QWidget(parent)
    , m_url(url)
    , m_fsModel(fsModel)
    , m_nav(qobject_cast<PathNavigator *>(parent))
{
    setAttribute(Qt::WA_Hover);
    setFixedSize(9, 9);
}

void
PathSeparator::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    QColor fg = palette().color(underMouse()?QPalette::Highlight:foregroundRole());
    QColor bg = palette().color(backgroundRole());

    int y = bg.value()>fg.value()?1:-1;
    QColor hl = Ops::colorMid(bg, y==1?Qt::white:Qt::black);
    QRect r = rect();
    QPainterPath triangle;
    triangle.moveTo(r.topLeft());
    triangle.lineTo(r.adjusted(0, r.height()/2, 0, 0).topRight());
    triangle.lineTo(r.bottomLeft());
    if (!underMouse())
        p.setOpacity(0.5f);
    p.setBrush(hl);
    p.drawPath(triangle.translated(0, y));
    p.setBrush(fg);
    p.drawPath(triangle);
    p.end();
}

void
PathSeparator::mousePressEvent(QMouseEvent *event)
{
    Menu menu;
    const QModelIndex &parent = m_fsModel->index(m_url);
    for (int i = 0; i < m_fsModel->rowCount(parent); ++i)
    {
        const QModelIndex &child = parent.child(i, 0);
        QAction *action = menu.addAction(child.data().toString(), m_fsModel, SLOT(setUrlFromDynamicPropertyUrl()));
        action->setProperty("url", QVariant::fromValue(m_fsModel->url(child)));
    }
    menu.exec(event->globalPos());
}

//-----------------------------------------------------------------------------

void
Menu::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
        QMenu::mousePressEvent(e);
    else if (e->button() == Qt::MidButton)
    {
        e->accept();
        QAction *action = qobject_cast<QAction *>(actionAt(e->pos()));
        const QString &path = action->data().toString();
        MainWindow::currentWindow()->addTab(path);
    }
    else
        e->accept();
}

//-----------------------------------------------------------------------------

NavButton::NavButton(QWidget *parent, const QUrl &url, const QString &text)
    : QWidget(parent)
    , m_url(url)
    , m_nav(static_cast<PathNavigator *>(parent))
    , m_hasData(false)
    , m_margin(4)
    , m_text(text)
{
    setMaximumHeight(16);
    if (m_nav->url() != url)
        setMinimumWidth(23);
    setAcceptDrops(true);
}

QSize
NavButton::sizeHint() const
{
    return QSize(18+fontMetrics().boundingRect(m_text).width()+m_margin*2, 16);
}

QSize
NavButton::minimumSizeHint() const
{
    return QSize(QWidget::minimumSizeHint().width(), 16);
}

void
NavButton::dragEnterEvent(QDragEnterEvent *e)
{
    if (m_url != m_nav->url())
    if (e->mimeData()->hasUrls())
    {
        e->acceptProposedAction();
        m_hasData = true;
        update();
    }
}

void
NavButton::dropEvent(QDropEvent *e)
{
    if (m_url.isLocalFile())
        IO::Manager::copy(e->mimeData()->urls(), m_url.path(), true, true);
    m_hasData = false;
    update();
}

void
NavButton::leaveEvent(QEvent *e)
{
    QWidget::leaveEvent(e);
    m_hasData = false;
    update();
}

void
NavButton::dragLeaveEvent(QDragLeaveEvent *e)
{
    QWidget::dragLeaveEvent(e);
    m_hasData = false;
    update();
}

void
NavButton::paintEvent(QPaintEvent *e)
{
//    QToolButton::paintEvent(e);
    const QPixmap &pix = m_nav->model()->fileIcon(m_nav->model()->index(m_url)).pixmap(16);
    QPainter p(this);
    p.drawTiledPixmap(m_margin, 0, 16, 16, pix);

    const QRect &textRect = QRect(18+m_margin, 0, width()-(18+m_margin), height());
    const QColor &fg = palette().color(foregroundRole());
    const QColor &bg = palette().color(backgroundRole());
    int y = bg.value()>fg.value()?1:-1;
    const QColor &emb = Ops::colorMid(bg, y==1?Qt::white:Qt::black);

    QLinearGradient lgf(textRect.topLeft(), textRect.topRight());
    lgf.setColorAt(0.0f, fg);
    lgf.setColorAt(0.6f, fg);
    lgf.setColorAt(1.0f, Qt::transparent);
    QLinearGradient lgb(textRect.topLeft(), textRect.topRight());
    lgb.setColorAt(0.0f, emb);
    lgb.setColorAt(0.6f, emb);
    lgb.setColorAt(1.0f, Qt::transparent);

    if (rect().width() < sizeHint().width()-m_margin)
        p.setPen(QPen(lgb, 1.0f));
    else
        p.setPen(emb);

    p.drawText(textRect.translated(0, 1), Qt::AlignLeft|Qt::AlignVCenter, m_text);

    if (rect().width() < sizeHint().width()-m_margin)
        p.setPen(QPen(lgf, 1.0f));
    else
        p.setPen(fg);

    p.drawText(textRect, Qt::AlignLeft|Qt::AlignVCenter, m_text);
    if (m_hasData)
    {
        QRectF r(rect());
        r.adjust(0.5f, 0.5f, -0.5f, -0.5f);
        p.setPen(palette().color(foregroundRole()));
        p.drawRect(r);
    }
    p.end();
}

void
NavButton::mousePressEvent(QMouseEvent *e)
{
    m_hasPress = true;
    QWidget::mousePressEvent(e);
}

void
NavButton::mouseReleaseEvent(QMouseEvent *e)
{
    if (!m_hasPress)
        return QWidget::mouseReleaseEvent(e);

    if (m_hasPress && !rect().contains(e->pos()))
    {
        m_hasPress = false;
        return QWidget::mouseReleaseEvent(e);
    }

    if (e->button() == Qt::LeftButton)
    {
        m_hasPress = false;
        emit urlRequest(m_url);
        e->accept();
        return;
    }
    else if (e->button() == Qt::MidButton)
    {
        m_hasPress = false;
        e->accept();
        MainWindow::currentWindow()->addTab(m_url);
    }
    else
        e->accept();
}

//-----------------------------------------------------------------------------

void
PathBox::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(palette().color(foregroundRole()));
    QStyleOptionComboBox opt;
    opt.initFrom(this);
    QRect r = QRect(0, 0, 9, 9);
    r.moveCenter(style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxArrow, this).center());
    QPolygon triangle(3);
    int w = r.right(), x = r.x(), y = r.y(), h = r.bottom();
    triangle.putPoints(0, 3,   x,y,   w,y,    r.center().x(),h);
    QPainterPath path;
    path.addPolygon(triangle);
    path.closeSubpath();
    p.drawPath(path);
    p.end();
}

//-----------------------------------------------------------------------------

PathNavigator::PathNavigator(QWidget *parent, FS::Model *model)
    : QWidget(parent)
    , m_fsModel(model)
    , m_hasPress(false)
    , m_layout(new QHBoxLayout(this))
    , m_bc(static_cast<BreadCrumbs *>(parent))
    , m_schemeButton(new Button(this))
{
//    connect(m_fsModel, SIGNAL(directoryLoaded(QString)), this, SLOT(genNavFromPath(QString)));
    connect(m_fsModel, SIGNAL(urlChanged(QUrl)), this, SLOT(genNavFromUrl(QUrl)));
    setContentsMargins(0, 0, 0, 0);
    m_layout->setContentsMargins(24, 0, 0, 0);
    m_layout->setSpacing(0);
    setLayout(m_layout);
    QTimer::singleShot(0, this, SLOT(postConstructorJobs()));
}

void
PathNavigator::postConstructorJobs()
{
    m_schemeButton->setIcon(IconProvider::icon(IconProvider::Configure, 16, palette().color(foregroundRole()), false));
    m_schemeButton->setMenu(m_fsModel->schemes());
}

void
PathNavigator::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    m_schemeButton->move(0, 0);
    m_schemeButton->resize(24, height());
}

QSize
PathNavigator::sizeHint() const
{
    return QSize(QWidget::sizeHint().width(), 16);
}

void
PathNavigator::mousePressEvent(QMouseEvent *e)
{
    QWidget::mousePressEvent(e);
    if (!childAt(e->pos()))
        m_hasPress = true;
}

void
PathNavigator::mouseReleaseEvent(QMouseEvent *e)
{
    QWidget::mouseReleaseEvent(e);
    if (m_hasPress && rect().contains(e->pos()))
        emit edit();
    m_hasPress = false;
}

void
PathNavigator::clear()
{
    while (QLayoutItem *item = m_layout->takeAt(0))
    {
        if (item->widget())
            delete item->widget();
        delete item;
    }
}

void
PathNavigator::genNavFromUrl(const QUrl &url)
{
    m_bc->setUrl(url);
    clear();

    if (!url.isLocalFile()) //for now....
        return;

    const QString &path = url.path();

    if (!QFileInfo(path).exists())
        return;

    QDir dir(path);
    do {
        const QString &newPath = dir.path();
        const QFileInfo fi(newPath);
        const QString buttonText(fi.fileName().isEmpty() ? fi.filePath() : fi.fileName());
        const QUrl &pathUrl = QUrl::fromLocalFile(newPath);
        NavButton *nb = new NavButton(this, pathUrl, buttonText);

        QFont f(font());
        f.setPointSize(qMax<int>(Store::config.behaviour.minFontSize, f.pointSize()*0.8));

        if (newPath == path)
            f.setBold(true);
        else
            nb->setCursor(Qt::PointingHandCursor);
        nb->setFont(f);

        m_layout->insertWidget(m_layout->indexOf(m_schemeButton)+1, nb);
        connect(nb, SIGNAL(urlRequest(QUrl)), m_fsModel, SLOT(setUrl(QUrl)));

        if (newPath != path)
        {
            PathSeparator *separator = new PathSeparator(this, pathUrl, m_fsModel);
            m_layout->insertWidget(m_layout->indexOf(nb)+1, separator);
        }
    } while (dir.cdUp());
    m_layout->addStretch();
    m_bc->pathBox()->setEditText(path);
}

//-----------------------------------------------------------------------------

BreadCrumbs::BreadCrumbs(QWidget *parent, FS::Model *fsModel)
    : QStackedWidget(parent)
    , m_pathNav(new PathNavigator(this, fsModel))
    , m_pathBox(new PathBox(this))
    , m_fsModel(fsModel)
{
    addWidget(m_pathNav);
    addWidget(m_pathBox);

    m_pathBox->setInsertPolicy(QComboBox::InsertAtBottom);
    m_pathBox->setEditable(true);
    m_pathBox->setDuplicatesEnabled(false);

    QCompleter *completer = new QCompleter(m_pathBox);
    QDirModel *dirModel = new QDirModel(completer); //this is no work w/ filesystemmodel?
    dirModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
    completer->setModel(dirModel);
    m_pathBox->setCompleter(completer);

    connect (m_pathBox, SIGNAL(activated(QString)), this, SLOT(urlFromEdit(QString)));
    connect (m_pathBox, SIGNAL(cancelEdit()), this, SLOT(toggleEditable()));
    connect (m_pathNav, SIGNAL(edit()), this, SLOT(toggleEditable()));
    setCurrentWidget(m_pathNav);
    QTimer::singleShot(0, this, SLOT(paletteOps()));
}

void
BreadCrumbs::paletteOps()
{
    const int style = Store::config.behaviour.pathBarStyle;
    if (style > 0)
        setAutoFillBackground(true);
    if (style!=0)
        setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    else
        setContentsMargins(0, 0, 0, 0);
    QPalette::ColorRole bg = backgroundRole(), fg = foregroundRole();
    QPalette pal = palette();
    const QColor fgc = pal.color(fg), bgc = pal.color(bg);
    if (style == 1)
    {
        //base color... slight hihglight tint
        QColor midC = Ops::colorMid(pal.color(QPalette::Base), qApp->palette().color(QPalette::Highlight), 10, 1);
        pal.setColor(bg, Ops::colorMid(Qt::black, midC, 1, 10));
        pal.setColor(fg, qApp->palette().color(QPalette::Text));
    }
    else if (style == 2)
    {
        pal.setColor(bg, Ops::colorMid(fgc, qApp->palette().color(QPalette::Highlight), 10, 1));
        pal.setColor(fg, bgc);
    }
    else if (style == 3)
    {
        const QColor &wtext = pal.color(QPalette::WindowText), w = pal.color(QPalette::Window);
        pal.setColor(bg, Ops::colorMid(wtext, qApp->palette().color(QPalette::Highlight), 10, 1));
        pal.setColor(fg, w);
    }
    pal.setBrush(bg, pal.color(bg));
    setPalette(pal);

    QPalette ppal(m_pathBox->palette());
    ppal.setColor(QPalette::Text, palette().color(foregroundRole()));
    ppal.setColor(QPalette::Base, palette().color(backgroundRole()));
    m_pathBox->setPalette(ppal);
}

QSize
BreadCrumbs::sizeHint() const
{
    return QSize(QFrame::sizeHint().width(), 16);
}

void
BreadCrumbs::urlFromEdit(const QString &urlString)
{
    const QString &checked = Ops::sanityChecked(urlString);
    QUrl url(checked);
    if (url.scheme().isEmpty())
        url = QUrl::fromLocalFile(checked);

    m_fsModel->setUrl(url);
    setEditable(false);
}

void
BreadCrumbs::setUrl(const QUrl &url)
{
    if (!m_fsModel)
        return;
    if (url != m_fsModel->rootUrl())
        return;
    bool exists = false;
    for (int i = 0; i < m_pathBox->count(); i++)
        if (m_pathBox->itemText(i) == url.path())
            exists = true;
    if (!exists && m_fsModel->index(url).isValid())
        m_pathBox->addItem(url.path());

    m_pathBox->setEditText(url.path());
}

void
BreadCrumbs::complete(const QString &path)
{
//    QString p = path;
//    if (!QDir(p).exists())
//        p = path.mid(0, path.lastIndexOf(QDir::separator()));
//    if (!p.endsWith(QDir::separator()))
//        p.append(QDir::separator());

//    const QString &current = path.mid(path.lastIndexOf(QDir::separator())+1);
//    QStringList paths;
//    foreach (const QString &subPath, QDir(p).entryList(QStringList() << "*"+current+"*", QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot, QDir::Name))
//        paths << QString("%1%2%3").arg(p, subPath, QDir::separator());

//    QCompleter comp(paths, m_pathBox);
//    comp.setCompletionMode(QCompleter::PopupCompletion);
//    comp.setCaseSensitivity(Qt::CaseInsensitive);
//    m_pathBox->setCompleter(&comp);
}
