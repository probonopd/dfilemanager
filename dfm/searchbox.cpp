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


#include <QPixmap>
#include <QDebug>
#include <QToolButton>
#include <QTimer>
#include "searchbox.h"
#include "iconprovider.h"
#include "config.h"
#include "mainwindow.h"
#include "fsmodel.h"
#include "viewcontainer.h"

using namespace DFM;

SearchIndicator::SearchIndicator(QWidget *parent)
    : QWidget(parent)
    , m_timer(new QTimer(this))
    , m_in(true)
    , m_step(100)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoMousePropagation);
    m_timer->setInterval(20);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(animate()));
    setFixedSize(256, 256);
    const QColor &fg(palette().color(QPalette::Text));
    m_pixmap = QPixmap(256, 256);
    m_pixmap.fill(Qt::transparent);
    QPainter p(&m_pixmap);
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(fg, 48.0f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setRenderHint(QPainter::Antialiasing);
    p.drawEllipse(25, 25, 164, 164);
    p.drawLine(180, 180, 207, 207);
    p.end();
    start();
}

void
SearchIndicator::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setOpacity(0.5f);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.translate(128, 128);
    p.scale((float)m_step/100, (float)m_step/100);
    p.translate(-128, -128);
    p.drawTiledPixmap(rect(), m_pixmap);
    p.end();
}

void
SearchIndicator::animate()
{
    if (m_step>99)
        m_in=false;
    if (m_step<80)
        m_in=true;

    if (m_in)
        ++m_step;
    else
        --m_step;
    update();
}

void
SearchIndicator::start()
{
    setVisible(true);
    m_timer->start();
}

void
SearchIndicator::stop()
{
    setVisible(false);
    m_timer->stop();
}

//--------------------------------------------------------------------------

SearchTypeSelector::SearchTypeSelector(SearchBox *parent) : Button(parent), m_searchBox(parent)
{
    setFixedSize(16, 16);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover);
    QMenu *m = new QMenu(this);
    m->setSeparatorsCollapsible(false);
    m->addSeparator()->setText(tr("Searching Options:"));
    QAction *filter = m->addAction(tr("Filter quickly by name"));
    QAction *search = m->addAction(tr("Search recursively in current path"));
    m->addSeparator();
    QAction *cancel = m->addAction(tr("Cancel Search"));
    QAction *close = m->addAction(tr("Close Search"));
    setOpacity(0.5f);

    connect(filter, SIGNAL(triggered()), this, SLOT(filter()));
    connect(search, SIGNAL(triggered()), this, SLOT(search()));
    connect(cancel, SIGNAL(triggered()), this, SLOT(cancel()));
    connect(close, SIGNAL(triggered()), this, SLOT(closeSearch()));
    setMenu(m);
    QTimer::singleShot(0, this, SLOT(updateIcon()));
    connect(MainWindow::window(this), SIGNAL(settingsChanged()), this, SLOT(updateIcon()));
}

void
SearchTypeSelector::filter()
{
    closeSearch();
    m_searchBox->clear();
    emit modeChanged(Filter);
}

void
SearchTypeSelector::updateIcon()
{
    setIcon(IconProvider::icon(IconProvider::Search, 16, parentWidget()->palette().color(QPalette::Text), Store::config.behaviour.systemIcons));
}

void
SearchTypeSelector::cancel()
{
    MainWindow *mw = MainWindow::window(this);
    mw->currentContainer()->model()->cancelSearch();
}

void
SearchTypeSelector::closeSearch()
{
    MainWindow *mw = MainWindow::window(this);
    mw->currentContainer()->model()->endSearch();
}

//--------------------------------------------------------------------------

ClearSearch::ClearSearch(QWidget *parent) : Button(parent)
{
    setFixedSize(16, 16);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover);
    QTimer::singleShot(0, this, SLOT(updateIcon()));
    connect(MainWindow::window(this), SIGNAL(settingsChanged()), this, SLOT(updateIcon()));
}

void
ClearSearch::updateIcon()
{
    setIcon(IconProvider::icon(IconProvider::Clear, 16, parentWidget()->palette().color(QPalette::Text), Store::config.behaviour.systemIcons));
}

//--------------------------------------------------------------------------

SearchBox::SearchBox(QWidget *parent)
    : QLineEdit(parent)
    , m_margin(4)
    , m_selector(new SearchTypeSelector(this))
    , m_clearSearch(new ClearSearch(this))
    , m_mode(Filter)
{
    setPlaceholderText("Filter By Name");

//    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    setMaximumWidth(256);
    setMouseTracking(true);
    setAlignment(Qt::AlignCenter);
//    QFont f(font());
//    f.setBold(true);
//    setFont(f);

    connect(this, SIGNAL(textChanged(QString)), this, SLOT(setClearButtonEnabled(QString)));
    connect(this, SIGNAL(textChanged(QString)), this, SLOT(correctSelectorPos()));
    connect(m_clearSearch, SIGNAL(clicked()), this, SLOT(clear()));
    connect(m_selector, SIGNAL(modeChanged(SearchMode)), this, SLOT(setMode(SearchMode)));
    connect(this, SIGNAL(returnPressed()), this, SLOT(search()));
    m_clearSearch->setVisible(false);
}

void
SearchBox::resizeEvent(QResizeEvent *event)
{
    QLineEdit::resizeEvent(event);
//    setTextMargins(m_selector->rect().width()+m_margin, 0, m_clearSearch->width()+m_margin, 0);
//    m_selector->move(rect().left() + m_margin, (rect().bottom()-m_selector->rect().bottom())>>1);
    m_selector->move(posFor());
    m_clearSearch->move(posFor(false));
//    m_clearSearch->move(rect().right()-(m_margin+m_clearSearch->width()), (rect().bottom()-m_clearSearch->rect().bottom())>>1);
}

void
SearchBox::setMode(const SearchMode mode)
{
    if (mode==Filter)
        setPlaceholderText("Filter by name");
    else if (mode==Search)
        setPlaceholderText("Search...");
    correctSelectorPos();
    m_mode = mode;
}

void
SearchBox::search()
{
    if (text().isEmpty()||m_mode == Filter)
        return;

    FS::Model *fsModel = MainWindow::window(this)->currentContainer()->model();
    if (!fsModel)
        return;
    fsModel->search(text());
}

QPoint
SearchBox::posFor(bool selector) const
{
    const QRect tr(fontMetrics().boundingRect(rect(), alignment(), text().isEmpty()?placeholderText():text()));
    const QSize sz(selector?m_selector->size():m_clearSearch->size());
    const int top((size().height()/2)-(sz.height()/2));
    return QPoint(selector?tr.left()-(sz.width()+m_margin):tr.right()+m_margin, top);
}

void
SearchBox::correctSelectorPos()
{
    m_selector->move(posFor(true));
    m_clearSearch->move(posFor(false));
}
