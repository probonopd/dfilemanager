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
#include <QDebug>
#include <QCompleter>
#include <QDirModel>

using namespace DFM;

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
    if ( !underMouse() )
        p.setOpacity(0.5f);
    p.setBrush(hl);
    p.drawPath(triangle.translated(0, y));
    p.setBrush(fg);
    p.drawPath(triangle);
    p.end();
}

void
PathSeparator::setPath()
{
    QAction *action = static_cast<QAction *>(sender());
    const QString &path = action->data().toString();
    m_fsModel->setRootPath(path);
}

void
PathSeparator::mousePressEvent(QMouseEvent *event)
{
    Menu menu;
    foreach ( const QFileInfo &info, QDir( m_path ).entryInfoList( QDir::AllDirs | QDir::NoDotAndDotDot ) )
    {
        QAction *action = new QAction( info.fileName(), this );
        action->setText( info.fileName() );
        action->setData( info.filePath() );
        if ( m_nav->path().startsWith(info.filePath()) )
        {
//            QFont fnt = action->font();
//            fnt.setBold(true);
//            fnt.setPointSize(fnt.pointSize()*1.2);
            action->setEnabled(false);
//            action->setFont(fnt);
        }
        menu.addAction( action );
        connect( action, SIGNAL( triggered() ), this, SLOT( setPath() ) );
    }
    menu.exec(event->globalPos());
}

void
Menu::mousePressEvent(QMouseEvent *e)
{
    if ( e->button() == Qt::LeftButton )
        QMenu::mousePressEvent(e);
    else if ( e->button() == Qt::MidButton )
    {
        e->accept();
        QAction *action = qobject_cast<QAction *>(actionAt(e->pos()));
        const QString &path = action->data().toString();
        MainWindow::currentWindow()->addTab(path);
    }
    else
        e->accept();
}

NavButton::NavButton(QWidget *parent, const QString &path, const QString &text)
    : QToolButton(parent)
    , m_path(path)
    , m_nav(static_cast<PathNavigator *>(parent))
    , m_hasData(false)
    , m_margin(4)
{
    setText(text);
    setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(this, SIGNAL(released()), this, SLOT(emitPath()));
    setFixedHeight(23);
    if ( m_nav->path() != path )
        setMinimumWidth(23);
    setIconSize(QSize(16, 16));
    setAcceptDrops(true);
    setForegroundRole(QPalette::Text);
}

QSize
NavButton::sizeHint() const
{
    return QSize(18+fontMetrics().boundingRect(text()).width()+m_margin*2, 23);
}

void
NavButton::dragEnterEvent(QDragEnterEvent *e)
{
    if ( m_path != m_nav->path() )
    if ( e->mimeData()->hasUrls() )
    {
        e->acceptProposedAction();
        m_hasData = true;
        update();
    }
}

void
NavButton::dropEvent(QDropEvent *e)
{
    IO::Job::copy(e->mimeData()->urls(), m_path, true, true);
    m_hasData = false;
    update();
}

void
NavButton::leaveEvent(QEvent *e)
{
    QToolButton::leaveEvent(e);
    m_hasData = false;
    update();
}

void
NavButton::dragLeaveEvent(QDragLeaveEvent *e)
{
    QToolButton::dragLeaveEvent(e);
    m_hasData = false;
    update();
}

void
NavButton::paintEvent(QPaintEvent *e)
{
//    QToolButton::paintEvent(e);
    const QPixmap &pix = m_nav->model()->fileIcon(m_nav->model()->index(m_path)).pixmap(16);
    QPainter p(this);
    p.drawTiledPixmap(m_margin, 4, 16, 16, pix);

    QRect textRect = QRect(18+m_margin, 4, width()-(18+m_margin), height()-5);
    const QColor &fg=palette().color(foregroundRole());
    const QColor &bg=palette().color(backgroundRole());
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
    if ( rect().width() < sizeHint().width()-m_margin )
        p.setPen(QPen(lgb, 1.0f));
    else
        p.setPen(emb);
    p.drawText(textRect.translated(0, 1), Qt::AlignLeft|Qt::AlignVCenter, text());
    if ( rect().width() < sizeHint().width()-m_margin )
        p.setPen(QPen(lgf, 1.0f));
    else
        p.setPen(fg);
    p.drawText(textRect, Qt::AlignLeft|Qt::AlignVCenter, text());
    p.end();

    if ( m_hasData )
    {
        QRectF r(rect());
        r.adjust(0.5f, 0.5f, -0.5f, -0.5f);
        QPainter p(this);
        p.setPen(palette().color(foregroundRole()));
        p.drawRect(r);
        p.end();
    }
}

void
NavButton::mouseReleaseEvent(QMouseEvent *e)
{
    if ( e->button() == Qt::LeftButton )
        QToolButton::mouseReleaseEvent(e);
    else if ( e->button() == Qt::MidButton )
    {
        e->accept();
        MainWindow::currentWindow()->addTab( m_path );
    }
    else
        e->accept();
}

PathNavigator::PathNavigator( QWidget *parent, FileSystemModel *model )
    : QFrame( parent )
    , m_fsModel(model)
    , m_hasPress(false)
    , m_layout(new QHBoxLayout(this))
{
    connect( this, SIGNAL( pathChanged( QString ) ), this, SLOT( genNavFromPath( QString ) ) );
    setForegroundRole( QPalette::Text );
    setBackgroundRole( QPalette::Base );
    setMaximumHeight( 23 );
    setContentsMargins( 0, 0, 0, 0 );
    m_layout->setContentsMargins( 0, 0, 0, 0 );
    m_layout->setSpacing( 0 );
    setLayout(m_layout);
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
PathNavigator::clear()
{
    while (QLayoutItem *item = m_layout->takeAt(0))
    {
        delete item->widget();
        delete item;
    }
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

    foreach ( const QString &newPath, m_pathList )
    {
        QString buttonText( QFileInfo( newPath ).fileName().isEmpty() ? "/." : QFileInfo( newPath ).fileName() );
        NavButton *nb = new NavButton( this, newPath, buttonText );

        QFont f(font());
        f.setPointSize(qMax<int>(Store::config.behaviour.minFontSize, f.pointSize()*0.8));

        if( newPath == path )
        {
            f.setBold( true );
            nb->setFont( f );
        }
        else
        {
            setFont(f);
            nb->setCursor( Qt::PointingHandCursor );
        }

//        if( nb->text() == "/." )
//            nb->setIcon( QIcon::fromTheme( "folder-system", QIcon::fromTheme( "inode-directory" ) ) );
//        else
//            nb->setIcon( m_fsModel->fileIcon(m_fsModel->index(newPath)) );

        m_layout->addWidget( nb );
        connect( nb, SIGNAL( navPath( QString ) ), m_fsModel, SLOT( setPath( QString ) ) );

        if ( newPath != path )
        {
            PathSeparator *separator = new PathSeparator( this, newPath, m_fsModel );
            m_layout->addWidget( separator );
        }
    }
    m_layout->addStretch();
}

BreadCrumbs::BreadCrumbs(QWidget *parent, FileSystemModel *fsModel) : QStackedWidget(parent)
{
    m_pathNav = new PathNavigator(this, fsModel);
    m_pathBox = new PathBox(this);
    m_fsModel = fsModel;

    addWidget(m_pathNav);
    addWidget(m_pathBox);

    setMaximumHeight(23);
    setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Base);
    QPalette pal = palette();
    QColor midC = Ops::colorMid( pal.color( QPalette::Base ), pal.color( QPalette::Highlight ), 10, 1 );
    pal.setColor( QPalette::Base, Ops::colorMid( Qt::black, midC, 1, 10 ) );
    setPalette( pal );

    m_pathBox->setInsertPolicy(QComboBox::InsertAtBottom);
    m_pathBox->setEditable(true);
    m_pathBox->setDuplicatesEnabled(false);
    m_pathBox->setMaximumHeight(23);
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

void BreadCrumbs::setRootPath( const QString &rootPath ) { m_fsModel->setRootPath( rootPath ); setEditable(false); }

void
BreadCrumbs::pathChanged(const QString &path)
{
    bool exists = false;
    for (int i = 0; i < m_pathBox->count();i++)
    {
        if(m_pathBox->itemText(i) == path)
            exists = true;
    }
    if (!exists && m_fsModel->index(path).isValid())
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
