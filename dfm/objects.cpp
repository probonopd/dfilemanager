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
