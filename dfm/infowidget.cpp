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


#include "infowidget.h"
#include <QLayout>
#include "operations.h"
#include "filesystemmodel.h"
#include "thumbsloader.h"
#include "config.h"

using namespace DFM;

InfoWidget::InfoWidget(QWidget *parent)
    : QFrame(parent)
    , m_tw(new ThumbWidget(this))
    , m_fileName(new TextLabel(this))
    , m_ownerLbl(new QLabel(this))
    , m_typeLbl(new QLabel(this))
    , m_mimeLbl(new QLabel(this))
    , m_sizeLbl(new QLabel(this))
    , m_owner(new QLabel(this))
    , m_type(new QLabel(this))
    , m_mime(new QLabel(this))
    , m_size(new QLabel(this))
{
//    QPalette pal = palette();
//    QColor midC = Operations::colorMid( pal.color( QPalette::Base ), pal.color( QPalette::Highlight ), 10, 1 );
//    pal.setColor( QPalette::Base, Operations::colorMid( Qt::black, midC, 1, 10 ) );
//    setPalette( pal );
    setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );

    QFont f(font());
    f.setBold(true);
    f.setPointSize(f.pointSize()+2);
    m_fileName->setFont(f);
//    m_fileName->setBackgroundRole(QPalette::WindowText);
//    m_fileName->setForegroundRole(QPalette::Window);

    f.setPointSize(qMax(Store::config.behaviour.minFontSize, f.pointSize()-2));
    for ( int i = 0; i < 2; ++i )
    {
        m_lastMod[i] = new QLabel(this);
        m_perm[i] = new QLabel(this);
    }

    foreach (QLabel *l, QList<QLabel *>() << m_owner << m_type << m_mime << m_size << m_lastMod[0] << m_perm[0])
    {
        l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        l->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
//        l->setBackgroundRole(QPalette::WindowText);
//        l->setForegroundRole(QPalette::Window);
    }

    foreach(QLabel *l, QList<QLabel *>() << m_ownerLbl << m_typeLbl <<  m_mimeLbl << m_sizeLbl << m_lastMod[1] << m_perm[1])
    {
        l->setFont(f);
        l->setWordWrap(true);
        l->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
//        l->setBackgroundRole(QPalette::WindowText);
//        l->setForegroundRole(QPalette::Window);
    }

    setMaximumHeight( 68 );
    QGridLayout *gdl = new QGridLayout();
    gdl->setMargin(1);
    gdl->addWidget(m_tw, 0, 0, 2, 1);
    gdl->addWidget(m_fileName, 0, 1, 2, 1);
    gdl->addWidget(m_owner, 0, 2);
    gdl->addWidget(m_type, 1, 2);
    gdl->addWidget(m_ownerLbl, 0, 3);
    gdl->addWidget(m_typeLbl, 1, 3);
    gdl->addWidget(m_mime, 0, 4);
    gdl->addWidget(m_size, 1, 4);
    gdl->addWidget(m_mimeLbl, 0, 5);
    gdl->addWidget(m_sizeLbl, 1, 5);
    gdl->addWidget(m_lastMod[0], 0, 6);
    gdl->addWidget(m_perm[0], 1, 6);
    gdl->addWidget(m_lastMod[1], 0, 7);
    gdl->addWidget(m_perm[1], 1, 7);
    gdl->setVerticalSpacing(0);
    setLayout(gdl);

    QTimer::singleShot(50, this, SLOT(paletteOps()));
}

void
InfoWidget::paletteOps()
{
    setAutoFillBackground( true );
    QPalette::ColorRole bg = backgroundRole(), fg = foregroundRole();
    QPalette pal = palette();
    const QColor fgc = pal.color(fg), bgc = pal.color(bg);
    if (Store::config.behaviour.sideBarStyle == 1)
    {
        //base color... slight hihglight tint
        QColor midC = Ops::colorMid( pal.color( QPalette::Base ), qApp->palette().color( QPalette::Highlight ), 10, 1 );
        pal.setColor( bg, Ops::colorMid( Qt::black, midC, 1, 10 ) );
        pal.setColor( fg, qApp->palette().color(fg) );
    }
    else if (Store::config.behaviour.sideBarStyle == 2)
    {
        pal.setColor(bg, Ops::colorMid( fgc, pal.color( QPalette::Highlight ), 10, 1 ));
        pal.setColor(fg, bgc);
    }
    else if (Store::config.behaviour.sideBarStyle == 3)
    {
        const QColor &wtext = pal.color(QPalette::WindowText), w = pal.color(QPalette::Window);
        pal.setColor(bg, Ops::colorMid( wtext, pal.color( QPalette::Highlight ), 10, 1 ));
        pal.setColor(fg, w);
    }
    setPalette(pal);
}

void
InfoWidget::paintEvent(QPaintEvent *event)
{
#if 0
    QLinearGradient lg(rect().topLeft(), rect().bottomLeft());
    const QColor &bgBase = palette().color(backgroundRole());
    lg.setColorAt(0.0f, Ops::colorMid(Qt::white, bgBase, 1, 4));
    lg.setColorAt(1.0f, Ops::colorMid(Qt::black, bgBase, 1, 4));
    QPainter p(this);
    p.fillRect(rect(), lg);
    p.end();
#endif
    QFrame::paintEvent(event);
}

void
InfoWidget::hovered(const QModelIndex &index)
{
    if (!isVisible())
        return;
    if (m_owner->text().isEmpty())
    {
        m_owner->setText(tr("Owner:"));
        m_type->setText(tr("Type:"));
        m_mime->setText(tr("MimeType:"));
        m_size->setText(tr("Size:"));
        m_lastMod[0]->setText("Last Modified:");
        m_perm[0]->setText("Permissions:");
    }
    const FileSystemModel *fsModel = static_cast<const FileSystemModel*>(index.model());
    const QFileInfo i = fsModel->fileInfo(index);
    m_tw->setPixmap(fsModel->fileIcon(index.sibling(index.row(), 0)).pixmap(64));
    m_fileName->setText(i.fileName());
    m_ownerLbl->setText(i.owner());
    m_typeLbl->setText(Ops::getFileType(i.filePath()));
    m_mimeLbl->setText(Ops::getMimeType(i.filePath()));

    m_lastMod[1]->setText(i.lastModified().toString());
    const QString &s(fsModel->data(index, FileSystemModel::FilePermissions).toString());
    m_perm[1]->setText(s);

    m_sizeLbl->setText(i.isDir() ? "--" : Ops::prettySize(i.size()));
}
