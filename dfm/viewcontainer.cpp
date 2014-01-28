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


#include "viewcontainer.h"
#include "operations.h"
#include "iojob.h"
#include "deletedialog.h"
#include "mainwindow.h"
#include <QProcess>
#include <QMap>
#include <QInputDialog>
#include <QCompleter>
#include <QMenu>
#include <QDesktopServices>

using namespace DFM;

#define VIEWS(RUN)\
    m_iconView->RUN;\
    m_detailsView->RUN;\
    m_columnsWidget->RUN;\
    m_flowView->RUN

//#define VIEWS(RUN)\
//    m_iconView->RUN;\
//    m_columnsWidget->RUN;

ViewContainer::ViewContainer(QWidget *parent, QString rootPath)
    : QFrame(parent)
    , m_model(new FileSystemModel(this))
    , m_viewStack(new QStackedWidget(this))
    , m_iconView(new IconView(this))
    , m_columnsWidget(new ColumnsWidget(this))
    , m_detailsView(new DetailsView(this))
    , m_flowView(new FlowView(this))
    , m_breadCrumbs(new BreadCrumbs(this, m_model))
    , m_searchIndicator(new SearchIndicator(this))

{
    m_searchIndicator->setVisible(false);
    connect(m_model, SIGNAL(searchFinished()), m_searchIndicator, SLOT(stop()));
    connect(m_model, SIGNAL(searchStarted()), m_searchIndicator, SLOT(start()));

    m_breadCrumbs->setVisible(Store::settings()->value("pathVisible", true).toBool());
    m_myView = Icon;
    m_back = false;

    m_viewStack->layout()->setSpacing(0);
    setFrameStyle(0/*QFrame::StyledPanel | QFrame::Sunken*/);
    setAutoFillBackground(false);

    setModel(m_model);
    setSelectionModel(new QItemSelectionModel(m_model));

    connect( m_model, SIGNAL(rootPathChanged(QString)), this, SLOT(setRootPath(QString)) );
    connect( m_model, SIGNAL(directoryLoaded(QString)), this, SLOT(dirLoaded(QString)) );
//    connect( m_model, SIGNAL(rootPathAboutToChange(QString)), this, SLOT(rootPathChanged(QString)) );
    connect( m_iconView, SIGNAL(iconSizeChanged(int)), this, SIGNAL(iconSizeChanged(int)) );
    connect( m_iconView, SIGNAL(newTabRequest(QModelIndex)), this, SLOT(genNewTabRequest(QModelIndex)) );
    connect( m_detailsView, SIGNAL(newTabRequest(QModelIndex)), this, SLOT(genNewTabRequest(QModelIndex)) );
    connect( m_flowView->detailsView(), SIGNAL(newTabRequest(QModelIndex)), this, SLOT(genNewTabRequest(QModelIndex)) );
    connect( m_model, SIGNAL(sortingChanged(int,int)), this, SIGNAL(sortingChanged(int,int)) );
    connect( m_model, SIGNAL(sortingChanged(int,int)), this, SLOT(modelSort(int,int)) );

    foreach (QAbstractItemView *v, QList<QAbstractItemView *>() << m_iconView << m_detailsView /*<< m_columnsView*/ << m_flowView->detailsView())
    {
        v->setMouseTracking( true );
        connect( v, SIGNAL(entered(QModelIndex)), this, SIGNAL(entered(QModelIndex)));
        connect( v, SIGNAL(viewportEntered()), this, SIGNAL(viewportEntered()));
        connect( v, SIGNAL(opened(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)));
//        connect( m_model, SIGNAL(paintRequest()), v->viewport(), SLOT(update()) );
    }

    m_viewStack->addWidget(m_iconView);
    m_viewStack->addWidget(m_detailsView);
    m_viewStack->addWidget(m_columnsWidget);
    m_viewStack->addWidget(m_flowView);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(m_viewStack, 1);
    layout->addWidget(m_breadCrumbs);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    setLayout(layout);

    setView((View)Store::config.behaviour.view, false);
    m_model->setPath(rootPath);

    emit iconSizeChanged(Store::config.views.iconView.iconSize*16);
}

void
ViewContainer::modelSort(const int column, const int order)
{
//    m_detailsView->header()->blockSignals(true);
//    m_detailsView->header()->setSortIndicator(column, (Qt::SortOrder)order);
//    m_detailsView->header()->blockSignals(false);
//    m_flowView->detailsView()->header()->blockSignals(true);
//    m_flowView->detailsView()->header()->setSortIndicator(column, (Qt::SortOrder)order);
//    m_flowView->detailsView()->header()->blockSignals(false);
}

PathNavigator *ViewContainer::pathNav() { return m_breadCrumbs->pathNav(); }

void ViewContainer::setModel(QAbstractItemModel *model) { VIEWS(setModel(model)); }

QList<QAbstractItemView *>
ViewContainer::views()
{
    return QList<QAbstractItemView *>() << m_iconView << m_detailsView << m_columnsWidget->currentView() << m_flowView->detailsView();
}

void
ViewContainer::setSelectionModel(QItemSelectionModel *selectionModel)
{
    m_selectModel = selectionModel;
    VIEWS(setSelectionModel(selectionModel));
}

FileSystemModel *ViewContainer::model() { return m_model; }

QItemSelectionModel *ViewContainer::selectionModel() { return m_selectModel; }

void
ViewContainer::setView(View view, bool store)
{
    m_myView = view;
    if (view == Icon)
        m_viewStack->setCurrentWidget(m_iconView);
    else if (view == Details)
        m_viewStack->setCurrentWidget(m_detailsView);
    else if (view == Columns)
        m_viewStack->setCurrentWidget(m_columnsWidget);
    else if (view == Flow)
        m_viewStack->setCurrentWidget(m_flowView);
    emit viewChanged();

#ifdef Q_WS_X11
    if ( Store::config.views.dirSettings && store )
    {
        QDir dir(m_model->rootPath());
        QSettings settings(dir.absoluteFilePath(".directory"), QSettings::IniFormat);
        settings.beginGroup("DFM");
        settings.setValue("view", (int)view);
        settings.endGroup();
    }
#endif
}

static QList<QAction *> s_actions;

QList<QAction *> ViewContainer::rightClickActions() { return s_actions; }

void
ViewContainer::addActions(QList<QAction *> actions)
{
    if ( s_actions.isEmpty() )
        s_actions = actions;
    VIEWS(addActions(actions));
    Store::settings()->beginGroup("Scripts");
    foreach ( const QString &string, Store::settings()->childKeys())
    {
        QStringList actions = Store::settings()->value(string).toStringList(); //0 == name, 1 == script, 2 == keysequence
        QAction *action = new QAction(actions[0], this);
        action->setData(actions[1]);
        if ( actions.count() > 2 )
            action->setShortcut(QKeySequence(actions[2]));
        connect (action, SIGNAL(triggered()), this, SLOT(scriptTriggered()));
        VIEWS(addAction(action));
    }
    Store::settings()->endGroup();
}

void
ViewContainer::customActionTriggered()
{
    QStringList action(static_cast<QAction *>(sender())->data().toString().split(" "));
    const QString &app = action.takeFirst();

    if ( selectionModel()->hasSelection() )
    {
        if ( selectionModel()->selectedRows().count() )
            foreach ( const QModelIndex &index, selectionModel()->selectedRows() )
                QProcess::startDetached(app, QStringList() << action << m_model->filePath( index ));
        else if ( selectionModel()->selectedIndexes().count() )
            foreach ( const QModelIndex &index, selectionModel()->selectedIndexes() )
                QProcess::startDetached(app, QStringList() << action << m_model->filePath( index ));
    }
    else
    {
        QProcess::startDetached(app, QStringList() << action << m_model->rootPath());
    }
}

void
ViewContainer::scriptTriggered()
{
    QStringList action(static_cast<QAction *>(sender())->data().toString().split(" "));
    const QString &app = action.takeFirst();
    qDebug() << "trying to launch" << app << "in" << m_model->rootPath();
    QProcess::startDetached(app, QStringList() << m_model->rootPath());
}

void
ViewContainer::activate(const QModelIndex &index)
{
    if ( !index.isValid() )
        return;

    const QFileInfo f(m_model->fileInfo(index));
    if ( Ops::openFile(f.filePath()) )
        return;

    if (m_model->isDir(index))
    {
        m_model->setPath(f.filePath());
        m_forwardList.clear();
    }
}

void
ViewContainer::setRootPath(const QString &path)
{
    const QModelIndex &rootIndex = m_model->index(path);
    setRootIndex(rootIndex);
    if ( Store::config.views.dirSettings )
    {
        const QDir dir(path);
        QSettings settings(dir.absoluteFilePath(".directory"), QSettings::IniFormat);
        settings.beginGroup("DFM");
        QVariant var = settings.value("view");
        if ( var.isValid() )
        {
            View view = (View)var.value<int>();
            setView(view, false);
        }
        settings.endGroup();
    }
    m_detailsView->setItemsExpandable(false);
    for ( int i = 0; i < views().count(); ++i )
        if ( views().at(i) )
        {
            views().at(i)->setAttribute(Qt::WA_Hover, false);
            views().at(i)->setMouseTracking(false);
        }
    m_selectModel->clearSelection();
    if (!m_back && QFileInfo(path).isDir() && (m_backList.isEmpty() || m_backList.count() &&  path != m_backList.last()))
        m_backList.append(path);
    emit rootPathChanged(path);
    m_back = false;
}

void
ViewContainer::dirLoaded(const QString &path)
{
    for ( int i = 0; i < views().count(); ++i )
        if ( views().at(i) )
        {
            views().at(i)->setAttribute(Qt::WA_Hover, true);
            views().at(i)->setMouseTracking(true);
        }

    m_detailsView->setItemsExpandable(true);
}

void
ViewContainer::goBack()
{
    if (m_backList.count() == 1)
        return;

    m_back = true;
    m_forwardList << m_backList.takeLast();
    m_model->setPath(m_backList.last());
}

void
ViewContainer::goForward()
{
    if (m_forwardList.isEmpty())
        return;

    m_model->setPath(m_forwardList.takeLast());
}

bool
ViewContainer::goUp()
{
    if (m_breadCrumbs->currentPath() == "/")
        return false;
    m_model->setPath(QFileInfo(m_model->rootPath()).dir().path());
    return true;
}

void ViewContainer::goHome() { m_model->setPath(QDir::homePath()); }

void ViewContainer::refresh() { m_model->refresh(); }

void
ViewContainer::rename()
{
    switch (m_myView)
    {
    case Icon : m_iconView->edit(m_iconView->currentIndex()); break;
    case Details : m_detailsView->edit(m_detailsView->currentIndex()); break;
    case Columns : m_columnsWidget->edit(m_columnsWidget->currentIndex()); break;
    case Flow : m_flowView->detailsView()->edit(m_flowView->detailsView()->currentIndex()); break;
    default: break;
    }
}

void
ViewContainer::setFilter(QString filter)
{
//    VIEWS(setFilter(filter));
    FileSystemModel::Node *node = model()->fromIndex(model()->index(model()->rootPath()));
    if ( !node )
        return;
    node->setFilter(filter);
    m_dirFilter = filter;
    emit filterChanged();
}

void
ViewContainer::deleteCurrentSelection()
{
    QModelIndexList delUs;
    if(m_selectModel->selectedRows(0).count())
        delUs = m_selectModel->selectedRows(0);
    else
        delUs = m_selectModel->selectedIndexes();

    if ( DeleteDialog(delUs).result() == 0 )
        return;

    QStringList delList;
    for (int i = 0; i < delUs.count(); ++i)
    {
        QFileInfo file(m_model->filePath(delUs.at(i)));
        if (file.isWritable())
            delList << file.filePath();
    }
    if ( !delList.isEmpty() )
        IO::Job::remove(delList);
}

void
ViewContainer::scrollToSelection()
{
    if(m_selectModel->selectedIndexes().isEmpty())
    {
        m_iconView->scrollToTop();
        m_detailsView->scrollToTop();
//        m_columnsWidget->scrollToTop();
        m_flowView->detailsView()->scrollToTop();
    }
    else
    {
        m_iconView->scrollTo(m_selectModel->selectedIndexes().first());
        m_detailsView->scrollTo(m_selectModel->selectedIndexes().first());
//        m_columnsWidget->scrollTo(m_selectModel->selectedIndexes().first());
        m_flowView->detailsView()->scrollTo(m_selectModel->selectedIndexes().first());
    }
}

void
ViewContainer::customCommand()
{
    QModelIndexList selectedItems;
    if (m_selectModel->selectedRows(0).count())
        selectedItems = m_selectModel->selectedRows(0);
    else
        selectedItems = m_selectModel->selectedIndexes();

    QModelIndex idx;
    if (selectedItems.count() == 1)
        idx = selectedItems.first();
    bool ok;
    QString text = QInputDialog::getText(this, tr("Open With"), tr("Custom Command:"), QLineEdit::Normal, QString(), &ok);
    if (!ok)
        return;

    QStringList args(text.split(" "));
    args << (m_model->filePath(idx).isNull() ? m_model->rootPath() : m_model->filePath(idx));
    QString program = args.takeFirst();
    QProcess::startDetached(program, args);
}

void
ViewContainer::sort(const int column, const Qt::SortOrder order)
{
    if ( column == m_model->sortColumn() && order == m_model->sortOrder() )
        return;
    m_model->sort(column, order);
}

void
ViewContainer::createDirectory()
{
    QModelIndex newFolder = m_model->mkdir(m_model->index(m_model->rootPath()), "new_directory");
    currentView()->scrollTo(newFolder);
    currentView()->edit(newFolder);
}

QAbstractItemView
*ViewContainer::currentView() const
{
    if ( m_myView == Columns )
        return m_columnsWidget->currentView();
    if ( m_myView != Flow )
        return static_cast<QAbstractItemView*>(m_viewStack->currentWidget());
    return static_cast<QAbstractItemView*>(m_flowView->detailsView());
}

void
ViewContainer::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    m_searchIndicator->move(width()-256, height()-256);
}

void ViewContainer::setRootIndex(const QModelIndex &index) { VIEWS(setRootIndex(index)); }

QModelIndex ViewContainer::indexAt(const QPoint &p) const { return currentView()->indexAt(mapFromParent(p)); }

bool ViewContainer::setPathVisible(bool visible) { m_breadCrumbs->setVisible(visible); }
bool ViewContainer::pathVisible() { return m_breadCrumbs->isVisible(); }

bool ViewContainer::canGoBack() { return m_backList.count() > 1; }
bool ViewContainer::canGoForward() { return m_forwardList.count() >= 1; }

void ViewContainer::genNewTabRequest(QModelIndex index) { emit newTabRequest(m_model->filePath(index)); }

BreadCrumbs *ViewContainer::breadCrumbs() { return m_breadCrumbs; }

void ViewContainer::setPathEditable(bool editable) { m_breadCrumbs->setEditable(editable); }

void ViewContainer::animateIconSize(int start, int stop) { m_iconView->setNewSize(stop); }
QSize ViewContainer::iconSize() { return m_iconView->iconSize(); }

QString ViewContainer::currentFilter() { return m_dirFilter; }

QString ViewContainer::rootPath() const { return m_model->rootPath(); }

//-------------------------------------------------------------------------------------------

QWidget
*FileItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QTextEdit *editor = new QTextEdit(parent);
    editor->setContentsMargins(0, 0, 0, 0);
    return editor;
}

void
FileItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QTextEdit *edit = qobject_cast<QTextEdit *>(editor);
    if (!edit||!index.isValid())
        return;

    edit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    const QString &oldName = index.data(Qt::EditRole).toString();
    edit->setText(oldName);

    const bool isDir = static_cast<const FileSystemModel *>(index.model())->isDir(index);
    QTextCursor tc = edit->textCursor();
    const int last = (isDir||!oldName.contains("."))?oldName.size():oldName.lastIndexOf(".");
    tc.setPosition(last, QTextCursor::KeepAnchor);
    edit->setTextCursor(tc);
}

void
FileItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QTextEdit *edit = qobject_cast<QTextEdit *>(editor);
    FileSystemModel *fsModel = static_cast<FileSystemModel *>(model);
    if ( !edit||!fsModel )
        return;
    FileSystemModel::Node *node = fsModel->fromIndex(index);
    const QString &newName = edit->toPlainText();
    if ( node->name() == newName )
        return;
    if ( !node->rename(newName) )
        QMessageBox::warning(edit->window(), "Failed to rename", QString("%1 to %2").arg(node->name(), newName));
}

bool
FileItemDelegate::eventFilter(QObject *object, QEvent *event)
{
    QWidget *editor = qobject_cast<QWidget *>(object);
    if (editor && event->type() == QEvent::KeyPress)
        if (static_cast<QKeyEvent *>(event)->key()==Qt::Key_Return)
        {
            emit commitData(editor);
            emit closeEditor(editor, QAbstractItemDelegate::NoHint);
            return true;
        }
    return QStyledItemDelegate::eventFilter(object, event);
}
