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
    setAutoFillBackground( true );
    setBackgroundRole(QPalette::WindowText);
    setForegroundRole(QPalette::Window);
    setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );

    QFont f(font());
    f.setBold(true);
    f.setPointSize(f.pointSize()+2);
    m_fileName->setFont(f);
    m_fileName->setBackgroundRole(QPalette::WindowText);
    m_fileName->setForegroundRole(QPalette::Window);

    f.setPointSize(f.pointSize()-2);
    for ( int i = 0; i < 2; ++i )
    {
        m_lastMod[i] = new QLabel(this);
        m_perm[i] = new QLabel(this);
    }

    foreach (QLabel *l, QList<QLabel *>() << m_owner << m_type << m_mime << m_size << m_lastMod[0] << m_perm[0])
    {
        l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        l->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        l->setBackgroundRole(QPalette::WindowText);
        l->setForegroundRole(QPalette::Window);
    }

    foreach(QLabel *l, QList<QLabel *>() << m_ownerLbl << m_typeLbl <<  m_mimeLbl << m_sizeLbl << m_lastMod[1] << m_perm[1])
    {
        l->setFont(f);
        l->setWordWrap(true);
        l->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        l->setBackgroundRole(QPalette::WindowText);
        l->setForegroundRole(QPalette::Window);
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
}

void
InfoWidget::paintEvent(QPaintEvent *event)
{
    QLinearGradient lg(rect().topLeft(), rect().bottomLeft());
    const QColor &bgBase = Operations::colorMid(palette().color(QPalette::Highlight), palette().color(backgroundRole()), 1, 5);
    lg.setColorAt(0.0f, Operations::colorMid(Qt::black, bgBase, 1, 3));
    lg.setColorAt(0.1f, Operations::colorMid(Qt::white, bgBase, 1, 2));
    lg.setColorAt(0.4999f, Operations::colorMid(Qt::white, bgBase, 1, 20));
    lg.setColorAt(0.5f, Operations::colorMid(Qt::black, bgBase, 1, 20));
    lg.setColorAt(1.0f, Operations::colorMid(Qt::black, bgBase, 1, 3));
    QPainter p(this);
    p.fillRect(rect(), lg);
    p.end();
    QFrame::paintEvent(event);
}

static inline float realSize(const float &size, int *t)
{
    float s = size;
    while (s > 1024)
    {
        if (*t == 5)
            break;
        s = s/1024;
        ++(*t);
    }
    return s;
}

static QString type[6] = { " Byte", " KiB", " MiB", " GiB", " TiB", " PiB" };

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
    QIcon icon = QPixmap::fromImage(ThumbsLoader::thumb(fsModel->filePath(index)));
    if ( icon.isNull() && fsModel->fileInfo(index).isDir() )
        icon = fsModel->iconPix(fsModel->fileInfo(fsModel->index(index.row(), 0, index.parent())), 64);
    if ( icon.isNull() )
        icon = qvariant_cast<QIcon>(fsModel->data(fsModel->index(index.row(), 0, index.parent()), Qt::DecorationRole));
    QPixmap pix = icon.pixmap(64);
    const bool dir = fsModel->fileInfo(index).isDir();
    m_tw->setPixmap(pix);
    m_fileName->setText(fsModel->data(fsModel->index(fsModel->filePath(index), 0)).toString());
    m_ownerLbl->setText(fsModel->fileInfo(index).owner());
    m_typeLbl->setText(Operations::getFileType(fsModel->filePath(index)));
    m_mimeLbl->setText(Operations::getMimeType(fsModel->filePath(index)));

    m_lastMod[1]->setText(fsModel->data(fsModel->index(index.row(), 3, index.parent())).toString());
    const QString &s(fsModel->data(fsModel->index(index.row(), 4, index.parent())).toString());
    m_perm[1]->setText(s.mid(0, s.lastIndexOf(",")));

    int t = 0;
    float size = realSize((float)(fsModel->fileInfo(index).size()), &t);
    m_sizeLbl->setText(dir ? qvariant_cast<QString>(fsModel->data(fsModel->index(fsModel->filePath(index), 1))) : QString::number(size) + type[t]);
}
