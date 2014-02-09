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

#include <QTextEdit>
#include "filesystemmodel.h"
#include "fsworkers.h"
#include "objects.h"


using namespace DFM;

Thread::Thread(QObject *parent) : QThread(parent)
  , m_quit(false)
  , m_pause(false)
{}

void
Thread::setPause(bool p)
{
    m_mutex.lock();
    m_pause=p;
    m_mutex.unlock();
    if (!p)
        m_pauseCond.wakeAll();
}

bool
Thread::isPaused()
{
    QMutexLocker locker(&m_mutex);
    return m_pause;
}

void
Thread::pause()
{
    m_mutex.lock();
    if (m_pause)
        m_pauseCond.wait(&m_mutex);
    m_mutex.unlock();
}

void
Thread::discontinue()
{
    m_mutex.lock();
    m_quit=true;
    m_mutex.unlock();
    setPause(false);
}

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

    const bool isDir = static_cast<const FS::Model *>(index.model())->isDir(index);
    QTextCursor tc = edit->textCursor();
    const int last = (isDir||!oldName.contains("."))?oldName.size():oldName.lastIndexOf(".");
    tc.setPosition(last, QTextCursor::KeepAnchor);
    edit->setTextCursor(tc);
}

void
FileItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QTextEdit *edit = qobject_cast<QTextEdit *>(editor);
    FS::Model *fsModel = static_cast<FS::Model *>(model);
    if (!edit||!fsModel)
        return;
    FS::Node *node = fsModel->nodeFromIndex(index);
    const QString &newName = edit->toPlainText();
    if (node->name() == newName)
        return;
    if (!node->rename(newName))
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

void
FileItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    QStyleOption textOpt;
    textOpt.initFrom(editor);
    int frameWidth = QApplication::style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &textOpt, editor);
    opt.rect = opt.rect.adjusted(-frameWidth, -frameWidth, frameWidth, frameWidth);
    QStyledItemDelegate::updateEditorGeometry(editor, opt, index);
}
