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

#include <QDebug>
#include <QCompleter>
#include <QMimeData>
#include <QApplication>

#include "pathnavigator.h"
#include "mainwindow.h"
#include "iojob.h"
#include "iconprovider.h"
#include "config.h"

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
//    p.setRenderHint(QPainter::Antialiasing);
//    p.setPen(Qt::NoPen);
//    QColor fg = palette().color(underMouse()?QPalette::Highlight:foregroundRole());
//    QColor bg = palette().color(backgroundRole());

//    int y = bg.value()>fg.value()?1:-1;
//    QColor hl = Ops::colorMid(bg, y==1?Qt::white:Qt::black);
//    QRect r = rect();
//    QPainterPath triangle;
//    triangle.moveTo(r.topLeft());
//    triangle.lineTo(r.adjusted(0, r.height()/2, 0, 0).topRight());
//    triangle.lineTo(r.bottomLeft());
//    if (!underMouse())
//        p.setOpacity(0.5f);
//    p.setBrush(hl);
//    p.drawPath(triangle.translated(0, y));
//    p.setBrush(fg);
//    p.drawPath(triangle);
    QStyleOption opt;
    opt.initFrom(this);
    style()->drawPrimitive(QStyle::PE_IndicatorArrowRight, &opt, &p, this);
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
        const QUrl &url = child.data(FS::UrlRole).toUrl();
        const QFileInfo fi(url.toLocalFile());
        if (!fi.isDir())
            continue;
        QAction *action = menu.addAction(child.data().toString(), m_fsModel, SLOT(setUrlFromDynamicPropertyUrl()));
        if (m_fsModel->rootUrl().toString().startsWith(url.toString()))
        {
            QFont f(action->font());
            f.setBold(true);
            action->setFont(f);
        }
        action->setProperty("url", QVariant::fromValue(url));
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
        const QUrl &path = action->property("url").toUrl();
        if (MainWindow *mw = MainWindow::currentWindow())
            mw->addTab(path);
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
//    setMaximumHeight(16);
    if (m_nav->url() != url)
        setMinimumWidth(23);
    if (text.isEmpty())
        setFixedWidth(24);
    setAcceptDrops(true);
    setAttribute(Qt::WA_Hover);
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
    const QFileInfo &fi = m_url.toLocalFile();
    if (fi.exists() && e->mimeData()->hasUrls())
    {
        QMenu m;
        QAction *move = m.addAction(QString("move to %1").arg(fi.filePath()));
        m.addAction(QString("copy to %1").arg(fi.filePath()));
        if (QAction *a = m.exec(QCursor::pos()))
        {
            const bool cut(a == move);
            IO::Manager::copy(e->mimeData()->urls(), fi.filePath(), cut, cut);
        }
    }
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
    const QModelIndex &index = m_nav->model()->index(m_url);
    QIcon icon = index.data(FS::FileIconRole).value<QIcon>();
    if (icon.isNull())
        icon = FS::FileIconProvider::fileIcon(QFileInfo(m_url.toLocalFile()));
    const QPixmap &pix = icon.pixmap(16);
    QPainter p(this);
    p.drawTiledPixmap(QRect(m_margin, height()/2-8, 16, 16), pix);

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

    QFont f = p.font();
    f.setUnderline(underMouse()&&!f.bold());
    p.setFont(f);
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
    {
        QWidget::mouseReleaseEvent(e);
        return;
    }

    if (m_hasPress && !rect().contains(e->pos()))
    {
        m_hasPress = false;
        QWidget::mouseReleaseEvent(e);
        return;
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
        if (MainWindow *mw = MainWindow::currentWindow())
            mw->addTab(m_url);
    }
    else
        e->accept();
}

//-----------------------------------------------------------------------------

PathNavigator::PathNavigator(QWidget *parent, FS::Model *model)
    : QWidget(parent)
    , m_fsModel(model)
    , m_hasPress(false)
    , m_layout(new QHBoxLayout(this))
    , m_bc(static_cast<NavBar *>(parent))
{
    connect(m_fsModel, SIGNAL(urlChanged(QUrl)), this, SLOT(genNavFromUrl(QUrl)));
    setContentsMargins(0, 0, 0, 0);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    setLayout(m_layout);
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
    m_url = url;
    m_bc->setUrl(url);
    clear();

    if (!url.isLocalFile()) //for now....
        return;

    const QString &path = url.toLocalFile();

    if (!QFileInfo(path).exists())
        return;

    QDir dir(path);
    do {
        const QString &newPath = dir.path();
        const QFileInfo fi(newPath);
        const QString buttonText(newPath=="/" ? QString() : fi.fileName().isEmpty() ? fi.filePath() : fi.fileName());
        const QUrl &pathUrl = QUrl::fromLocalFile(newPath);
        NavButton *nb = new NavButton(this, pathUrl, buttonText);

        QFont f(font());
        f.setPointSize(qMax<int>(Store::config.behaviour.minFontSize, f.pointSize()*0.8));

        if (newPath == path)
            f.setBold(true);
        else
            nb->setCursor(Qt::PointingHandCursor);
        nb->setFont(f);

        m_layout->insertWidget(0, nb);
        connect(nb, SIGNAL(urlRequest(QUrl)), m_fsModel, SLOT(setUrl(QUrl)));

        if (newPath != path)
        {
            PathSeparator *separator = new PathSeparator(this, pathUrl, m_fsModel);
            m_layout->insertWidget(m_layout->indexOf(nb)+1, separator);
        }
    } while (dir.cdUp());
    m_layout->addStretch();
}

//-----------------------------------------------------------------------------

PathBox::PathBox(QWidget *parent) : QComboBox(parent)
{
    setContentsMargins(0, 0, 0, 0);
}

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

NavBar::NavBar(QWidget *parent, FS::Model *fsModel)
    : FakeView(parent)
    , m_pathNav(new PathNavigator(this, fsModel))
    , m_pathBox(new PathBox(this))
    , m_fsModel(fsModel)
    , m_schemeButton(new Button(this))
    , m_stack(new QStackedLayout())
    , m_dock(0)
{
    setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    m_pathBox->setInsertPolicy(QComboBox::InsertAtBottom);
    m_pathBox->setEditable(true);
    m_pathBox->setDuplicatesEnabled(false);
    setFixedHeight(25);

    PathCompleter *completer = new PathCompleter(m_fsModel, m_pathBox);
    m_pathBox->setCompleter(completer);

    connect(completer, SIGNAL(highlighted(QString)), m_pathBox, SLOT(setEditText(QString)));
    connect(m_pathBox, SIGNAL(editTextChanged(QString)), completer, SLOT(textChanged(QString)));
    connect(m_pathBox, SIGNAL(activated(QString)), this, SLOT(urlFromEdit(QString)));
    connect(m_pathBox, SIGNAL(cancelEdit()), this, SLOT(toggleEditable()));
    connect(m_pathNav, SIGNAL(edit()), this, SLOT(toggleEditable()));
    connect(m_schemeButton, SIGNAL(clicked()), this, SLOT(toggleEditable()));

    m_stack->addWidget(m_pathNav);
    m_stack->addWidget(m_pathBox);
    m_stack->setContentsMargins(0, 0, 0, 0);
    m_stack->setSpacing(0);
    m_stack->setCurrentWidget(m_pathNav);

    QHBoxLayout *layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_schemeButton);
    layout->addLayout(m_stack);
    viewport()->setLayout(layout);

    QTimer::singleShot(0, this, SLOT(paletteOps()));
    QTimer::singleShot(0, this, SLOT(postConstructorJobs()));
}

void
NavBar::postConstructorJobs()
{
    m_schemeButton->setIcon(IconProvider::icon(IconProvider::Circle, 16, viewport()->palette().color(foregroundRole()), false));
//    m_schemeButton->setMenu(m_fsModel->schemes());
    m_schemeButton->setFixedSize(16, 16);
}

void
NavBar::paletteOps()
{
    const int barStyle = Store::config.behaviour.pathBarStyle;
    if (barStyle == -1 && style()->objectName() == "oxygen")
    {
//        setContentsMargins(0, 0, 0, 0);
        setFrameStyle(0);
        m_dock = new QDockWidget(this);
        m_dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
        m_dock->setVisible(true);
        m_dock->setFixedSize(size());
        m_dock->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_dock->setMask(QRegion(rect())-rect().adjusted(4, 4, -4, -4));
        return;
    }
    QPalette::ColorRole bg = viewport()->backgroundRole(), fg = viewport()->foregroundRole();
    if (barStyle == -1)
    {
        viewport()->setAutoFillBackground(false);
        setFrameStyle(0);
    }
    if (!viewport()->autoFillBackground())
    {
        bg = backgroundRole();
        fg = foregroundRole();
    }
    setBackgroundRole(bg);
    setForegroundRole(fg);
    viewport()->setBackgroundRole(bg);
    viewport()->setForegroundRole(fg);
    QPalette pal = palette();
    const QColor fgc = pal.color(fg), bgc = pal.color(bg);
    if (barStyle == 1)
    {
        viewport()->setAutoFillBackground(true);
        //base color... slight hihglight tint
        QColor midC = Ops::colorMid(pal.color(QPalette::Base), qApp->palette().color(QPalette::Highlight), 10, 1);
        pal.setColor(bg, Ops::colorMid(Qt::black, midC, 255-pal.color(QPalette::Window).value(), 255));
        pal.setColor(fg, qApp->palette().color(fg));
    }
    else if (barStyle == 2)
    {
        pal.setColor(bg, Ops::colorMid(fgc, qApp->palette().color(QPalette::Highlight), 10, 1));
        pal.setColor(fg, bgc);
    }
    else if (barStyle == 3)
    {
        const QColor &wtext = pal.color(QPalette::WindowText), w = pal.color(QPalette::Window);
        pal.setColor(bg, Ops::colorMid(wtext, qApp->palette().color(QPalette::Highlight), 10, 1));
        pal.setColor(fg, w);
    }
    setPalette(pal);
}

void
NavBar::resizeEvent(QResizeEvent *e)
{
    QFrame::resizeEvent(e);
    if (m_dock)
    {
        m_dock->setFixedSize(e->size());
        m_dock->setMask(QRegion(rect())-rect().adjusted(4, 4, -4, -4));
    }
}

void
NavBar::urlFromEdit(const QString &urlString)
{
    const QString &checked = Ops::sanityChecked(urlString);
    QUrl url = QUrl::fromUserInput(checked);
    if (!url.toString().endsWith("///"))
        url = url.toEncoded(QUrl::StripTrailingSlash);
//    qDebug() << url << "NavBar::urlFromEdit";
    if (url.isLocalFile() /*&& !url.path().isEmpty()*/)
    {
        QString path = url.toLocalFile();
        if (path.endsWith(":"))
            path.append("/");
        if (QFileInfo(path).exists())
            url = QUrl::fromLocalFile(path);
    }
    if (m_fsModel->setUrl(url))
        m_pathBox->setEditText(url.toString());
    else
        m_pathBox->setEditText(m_fsModel->rootUrl().toString());
    setEditable(!QFileInfo(m_fsModel->rootUrl().toLocalFile()).exists());
}

void
NavBar::setUrl(const QUrl &url)
{
    bool exists = false;
    for (int i = 0; i < m_pathBox->count(); i++)
        if (m_pathBox->itemText(i) == url.toString())
            exists = true;
    if (!exists && m_fsModel->index(url).isValid())
        m_pathBox->addItem(url.toString());

    m_pathBox->setEditText(url.toString());
    setEditable(!url.isLocalFile());
}

void
NavBar::setEditable(const bool editable)
{
    if (!editable && url().isLocalFile())
    {
        m_stack->setCurrentWidget(m_pathNav);
        m_schemeButton->setIcon(IconProvider::icon(IconProvider::Circle, 16, viewport()->palette().color(foregroundRole()), false));
    }
    else
    {
        m_stack->setCurrentWidget(m_pathBox);
        m_schemeButton->setIcon(IconProvider::icon(IconProvider::OK, 16, viewport()->palette().color(foregroundRole()), false));
    }
    currentWidget()->setFocus();     
}

void NavBar::toggleEditable() { setEditable(currentWidget() == m_pathNav); }
void NavBar::stopAnimating() { m_schemeButton->stopAnimating(); }
void NavBar::startAnimating() { m_schemeButton->startAnimating(); }

//-----------------------------------------------------------------------------

PathCompleter::PathCompleter(QAbstractItemModel *model, QWidget *parent)
    : QCompleter(model, parent)
{
    setMaxVisibleItems(20);
    setCaseSensitivity(Qt::CaseInsensitive);
    setCompletionMode(QCompleter::UnfilteredPopupCompletion);
}

QString
PathCompleter::pathFromIndex(const QModelIndex &index) const
{
//    QString path = index.data(FS::Url).toString();
//    if (index.data(FS::FileType).toString() == "directory")
    const FS::Model *fsModel = static_cast<const FS::Model *>(index.model());
    QString path = fsModel->url(index).toString();
    if (fsModel->fileInfo(index).isDir() && !path.endsWith("/"))
        path.append("/");
    return path;
}

QStringList
PathCompleter::splitPath(const QString &path) const
{
    const QUrl url(path);
    if (!url.isLocalFile())
        return QStringList();
    QStringList list;
//    list << url.scheme();
    QStringList plist = QStringList() << url.toLocalFile().split("/", QString::SkipEmptyParts);
#if defined(ISUNIX)
    list << "/";
    list << plist;
#else
    if (plist.isEmpty())
        return list;
    QString partition = plist.takeFirst();
    partition.append("/");
    list << partition;
    list << plist;
#endif
    if (path.endsWith("/"))
        list << "*";
    return list;
}

void
PathCompleter::textChanged(const QString &text)
{
    if (text.endsWith("/"))
        setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    else
        setCompletionMode(QCompleter::PopupCompletion);
}
