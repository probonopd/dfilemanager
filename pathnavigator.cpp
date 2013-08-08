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
#include "application.h"
#include "mainwindow.h"
#include <QDebug>
#include <QCompleter>
#include <QDirModel>

using namespace DFM;

void
Menu::mousePressEvent(QMouseEvent *e)
{
    if ( e->button() == Qt::LeftButton )
        QMenu::mousePressEvent(e);
    else if ( e->button() == Qt::MidButton )
    {
        e->accept();
        MAINWINDOW->addTab( qobject_cast<PathSubDir *>(actionAt(e->pos()))->path() );
    }
    else
        e->accept();
}

void
NavButton::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    p.setPen(palette().color(QPalette::Text));
    QRect r(rect());
    r.setLeft(r.left() + 5);
    QPixmap pix(icon().pixmap(iconSize().height()));
    if ( pix.size() != iconSize() )
        pix = pix.scaled(iconSize());
//    style()->drawItemPixmap(&p, r, Qt::AlignLeft | Qt::AlignVCenter, pix);
    style()->drawItemText(&p, r.adjusted(icon().isNull() ? 0 : 20,0,0,0), Qt::AlignLeft | Qt::AlignVCenter, palette(), true, text(), QPalette::Text);
    r.setSize(iconSize());
    r.moveCenter(rect().center());
    r.moveLeft(rect().left() + 5);
    p.drawTiledPixmap(r, pix);
    p.end();
//    QToolButton::paintEvent(event);
}

void
NavButton::mouseReleaseEvent(QMouseEvent *e)
{
    if ( e->button() == Qt::LeftButton )
        QToolButton::mouseReleaseEvent(e);
    else if ( e->button() == Qt::MidButton )
    {
        e->accept();
        MAINWINDOW->addTab( m_path );
    }
    else
        e->accept();
}

PathNavigator::PathNavigator( QWidget *parent, FileSystemModel *model ) : QToolBar( parent ), m_fsModel(model), m_hasPress(false)
{
    if ( qApp->styleSheet().isEmpty() )
        setStyle( new MyStyle( style()->objectName() ) );
    connect( this, SIGNAL( pathChanged( QString ) ), this, SLOT( genNavFromPath( QString ) ) );
    setForegroundRole( QPalette::Text );
    setBackgroundRole( QPalette::Base );
    setMaximumHeight( iconSize().height() + 5 );
    setContentsMargins( 5, 0, 0, 0 );
}

void
PathNavigator::setPath( const QString &path )
{
    if ( path == m_path )
        return;
    m_path = path;
    emit pathChanged( path );
}

void
PathNavigator::genNavFromPath( const QString &path )
{
    QModelIndex index = m_fsModel->index( path );
    m_pathList.clear();
    clear();

    while ( index.parent().isValid() )
    {
        index = index.parent();
        m_pathList.prepend( m_fsModel->filePath( index ) );
    }
    m_pathList << path;

    foreach ( QString newPath, m_pathList )
    {
        NavButton *nb = new NavButton( this, newPath );
        QString buttonText( QFileInfo( newPath ).fileName().isEmpty() ? "/." : QFileInfo( newPath ).fileName() );

        nb->setMaximumWidth( 192 );

        buttonText = nb->fontMetrics().elidedText( buttonText, Qt::ElideRight, 164 );
        nb->setText( buttonText );

        QFont font = nb->font();
        font.setBold( true );

        if( newPath == path )
            nb->setFont( font );
        else
            nb->setCursor( Qt::PointingHandCursor );

        if( nb->text() == "/." )
            nb->setIcon( QIcon::fromTheme( "folder-system", QIcon::fromTheme( "inode-directory" ) ) );
        else
            nb->setIcon( qvariant_cast<QIcon>( m_fsModel->data( m_fsModel->index( newPath ), Qt::DecorationRole ) ) );

        addWidget( nb );
        connect( nb, SIGNAL( navPath( QString ) ), m_fsModel, SLOT( setPath( QString ) ) );

        if ( newPath != path )
        {
            PathSeparator *separator = new PathSeparator( this, newPath, m_fsModel );
            addWidget( separator );
        }
    }
}

BreadCrumbs::BreadCrumbs(QWidget *parent, FileSystemModel *fsModel) : QStackedWidget(parent)
{
    m_pathNav = new PathNavigator(this, fsModel);
    m_pathBox = new PathBox(this);
    m_fsModel = fsModel;

    addWidget(m_pathNav);
    addWidget(m_pathBox);

    setMaximumHeight(m_pathNav->iconSize().height() + 5);
    setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Base);
    QPalette pal = palette();
    QColor midC = Operations::colorMid( pal.color( QPalette::Base ), pal.color( QPalette::Highlight ), 10, 1 );
    pal.setColor( QPalette::Base, Operations::colorMid( Qt::black, midC, 1, 10 ) );
    setPalette( pal );

    m_pathBox->setInsertPolicy(QComboBox::InsertAtBottom);
    m_pathBox->setEditable(true);
    m_pathBox->setDuplicatesEnabled(false);
    m_pathBox->setMaximumHeight(m_pathNav->iconSize().height() + 5);
    m_pathBox->installEventFilter(this);


    QCompleter *completer = new QCompleter(m_pathBox);
    QDirModel *dirModel = new QDirModel(completer); //this is no work w/ filesystemmodel?
    dirModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
    completer->setModel(dirModel);
    m_pathBox->setCompleter(completer);

    connect ( m_pathBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(setRootPath(QString)) );
    connect ( m_pathBox, SIGNAL(activated(QString)), this, SLOT(setRootPath(QString)) );
    connect ( m_pathBox, SIGNAL(cancelEdit()), this, SLOT(toggleEditable()) );
//    connect ( m_pathBox, SIGNAL(textChanged(QString)), this, SLOT(complete(QString)) );
    connect ( m_fsModel, SIGNAL(rootPathChanged(QString)), m_pathNav, SLOT(setPath(QString)) );
    connect ( m_fsModel, SIGNAL(rootPathChanged(QString)), this, SLOT(pathChanged(QString)) );
    connect ( m_pathNav, SIGNAL(pathChanged(QString)), this, SIGNAL(newPath(QString)) );
    connect ( m_pathNav, SIGNAL(edit()), this, SLOT(toggleEditable()) );
    setCurrentWidget(m_pathNav);
}

void
BreadCrumbs::pathChanged(const QString &path)
{
    bool exists = false;
    for(int i = 0; i < m_pathBox->count();i++)
    {
        if(m_pathBox->itemText(i) == path)
            exists = true;
    }
    if(!exists && m_fsModel->index(path).isValid())
        m_pathBox->addItem(path);

    m_pathBox->setEditText(path);
}

void
BreadCrumbs::complete( const QString &path )
{
    QString p = path;
    if ( !QDir(p).exists() )
        p = path.mid(0, path.lastIndexOf(QDir::separator()));
    if ( !p.endsWith(QDir::separator()) )
        p.append(QDir::separator());

    const QString &current = path.mid(path.lastIndexOf(QDir::separator())+1);
    QStringList paths;
    foreach ( const QString &subPath, QDir(p).entryList(QStringList() << "*"+current+"*", QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot, QDir::Name) )
        paths << p + subPath;

    QCompleter *comp = new QCompleter(paths, m_pathBox);
    comp->setCompletionMode(QCompleter::PopupCompletion);
    comp->setCaseSensitivity(Qt::CaseInsensitive);
    m_pathBox->setCompleter(comp);
}
