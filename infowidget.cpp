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

InfoWidget::InfoWidget(QWidget *parent) : QFrame(parent)
  , tw(new ThumbWidget(this)), fileName(new TextLabel(this))
  , ownerLbl(new QLabel(this)), typeLbl(new QLabel(this))
  , mimeLbl(new QLabel(this)), sizeLbl(new QLabel(this))
  , owner(new QLabel(this)), typ(new QLabel(this))
  , mime(new QLabel(this)), m_size(new QLabel(this))
{
    QPalette pal = palette();
    QColor midC = Operations::colorMid( pal.color( QPalette::Base ), pal.color( QPalette::Highlight ), 10, 1 );
    pal.setColor( QPalette::Base, Operations::colorMid( Qt::black, midC, 1, 10 ) );
    setPalette( pal );
    setAutoFillBackground( true );
    setBackgroundRole( QPalette::Base );
    setForegroundRole( QPalette::Text );

    setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
    QFont f(font());
    f.setBold(true);
    f.setPointSize(f.pointSize()+2);
    fileName->setFont(f);

    f.setPointSize(f.pointSize()-2);

    foreach (QLabel *l, QList<QLabel *>() << owner << typ << mime << m_size)
    {
        l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        l->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    }

    foreach(QLabel *l, QList<QLabel *>() << ownerLbl << typeLbl <<  mimeLbl << sizeLbl)
    {
        l->setFont(f);
        l->setWordWrap(true);
        l->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    setMaximumHeight( 68 );
    QGridLayout *gdl = new QGridLayout();
    gdl->setMargin(1);
    gdl->addWidget(tw, 0, 0, 2, 1);
    gdl->addWidget(fileName, 0, 1, 2, 1);
    gdl->addWidget(owner, 0, 2);
    gdl->addWidget(typ, 1, 2);
    gdl->addWidget(ownerLbl, 0, 3);
    gdl->addWidget(typeLbl, 1, 3);
    gdl->addWidget(mime, 0, 4);
    gdl->addWidget(m_size, 1, 4);
    gdl->addWidget(mimeLbl, 0, 5);
    gdl->addWidget(sizeLbl, 1, 5);
    gdl->setVerticalSpacing(0);
    setLayout(gdl);
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
    if (owner->text().isEmpty())
    {
        owner->setText(tr("Owner:"));
        typ->setText(tr("Type:"));
        mime->setText(tr("MimeType:"));
        m_size->setText(tr("Size:"));
    }
    const FileSystemModel *fsModel = static_cast<const FileSystemModel*>(index.model());
    QIcon icon = QPixmap::fromImage(ThumbsLoader::thumb(fsModel->filePath(index)));
    if ( icon.isNull() )
        icon = fsModel->iconPix(fsModel->fileInfo(fsModel->index(fsModel->filePath(index), 0)), 64);
    if (icon.isNull())
        icon = qvariant_cast<QIcon>(fsModel->data(fsModel->index(fsModel->filePath(index), 0), Qt::DecorationRole));
    QPixmap pix = icon.pixmap(64);
    const bool dir = fsModel->fileInfo(index).isDir();
    tw->setPixmap(pix);
    fileName->setText(fsModel->data(fsModel->index(fsModel->filePath(index), 0)).toString());
    ownerLbl->setText(fsModel->fileInfo(index).owner());
    typeLbl->setText(Operations::getFileType(fsModel->filePath(index)));
    mimeLbl->setText(Operations::getMimeType(fsModel->filePath(index)));
    int t = 0;
    float size = realSize((float)(fsModel->fileInfo(index).size()), &t);
    sizeLbl->setText(dir ? qvariant_cast<QString>(fsModel->data(fsModel->index(fsModel->filePath(index), 1))) : QString::number(size) + type[t]);
}
