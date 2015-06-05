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
#include <QMessageBox>
#include <QScrollBar>
#include <QTimer>

#include "fsworkers.h"
#include "objects.h"
#include "application.h"

using namespace DFM;

DThread::DThread(QObject *parent) : QThread(parent)
  , m_quit(false)
  , m_pause(false)
{}

void
DThread::setPause(bool p)
{
    emit pauseToggled(p);
    m_pausingMutex.lock();
    m_pause=p;
    m_pausingMutex.unlock();
    if (!p)
        m_pausingCondition.wakeAll();
}

bool
DThread::isPaused() const
{
    QMutexLocker locker(&m_pausingMutex);
    return m_pause;
}

void
DThread::pause()
{
    m_pausingMutex.lock();
    if (m_pause)
        m_pausingCondition.wait(&m_pausingMutex);
    m_pausingMutex.unlock();
}

void
DThread::discontinue()
{
    m_pausingMutex.lock();
    m_quit=true;
    m_pausingMutex.unlock();
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

    const bool isDir = index.data(FS::FileIsDirRole).value<bool>();
    QTextCursor tc = edit->textCursor();
    const int last = (isDir||!oldName.contains("."))?oldName.size():oldName.lastIndexOf(".");
    tc.setPosition(last, QTextCursor::KeepAnchor);
    edit->setTextCursor(tc);
}

void
FileItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    if (QTextEdit *edit = qobject_cast<QTextEdit *>(editor))
        model->setData(index, edit->toPlainText());
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

//-------------------------------------------------------------------------------------------

ScrollAnimator::ScrollAnimator(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
    , m_up(false)
    , m_delta(0)
    , m_step(0)
{
    if (!parent)
    {
        deleteLater();
        return;
    }
    connect(m_timer, SIGNAL(timeout()), this, SLOT(updateScrollValue()));
    static_cast<QAbstractScrollArea *>(parent)->viewport()->installEventFilter(this);
}

void
ScrollAnimator::manage(QAbstractScrollArea *area)
{
    if (!area || area->findChild<ScrollAnimator *>())
        return;
    new ScrollAnimator(area);
}

#define MAXDELTA 80

void
ScrollAnimator::updateScrollValue()
{

    if (m_delta > 0)
    {
        QScrollBar *bar = static_cast<QAbstractScrollArea *>(parent())->verticalScrollBar();
        if ((m_up && bar->value() == bar->minimum())
               || (!m_up && bar->value() == bar->maximum()))
        {
            m_delta = 0;
            m_timer->stop();
            return;
        }
        bar->setValue(bar->value() + (m_up?-m_delta:m_delta));
        m_delta -= m_step;
    }
    else
    {
        m_delta = 0;
        m_timer->stop();
    }
}

bool
ScrollAnimator::processWheelEvent(QWheelEvent *e)
{
    if (e->orientation() == Qt::Horizontal || e->modifiers())
        return false;
    const bool wasUp(m_up);
    m_up = e->delta() > 0;
    if (m_up != wasUp)
        m_delta = 0;

    m_delta+=10;
    m_step=m_delta/10;
    if (!m_timer->isActive())
        m_timer->start(20);
    return true;
}

bool
ScrollAnimator::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::Wheel)
        return processWheelEvent(static_cast<QWheelEvent *>(e));
    return false;
}
