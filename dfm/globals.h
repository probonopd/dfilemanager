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


#ifndef GLOBALS_H
#define GLOBALS_H

#include <QString>
#include <QDir>
#include <QUrl>

namespace DFM
{

enum Actions { GoBack = 0,
               GoUp,
               GoForward,
               GoHome,
               Exit,
               About,
               AboutQt,
               AddPlace,
               RenamePlace,
               RemovePlace,
               Views_Icon,
               Views_Detail,
               Views_Column,
               Views_Flow,
               DeleteSelection,
               ShowHidden,
               MkDir,
               Copy,
               Cut,
               Paste,
               Rename,
               AddPlaceContainer,
               SetPlaceIcon,
               Refresh,
               CustomCommand,
               ShowMenuBar,
               ShowToolBar,
               ShowStatusBar,
               ShowPathBar,
               AddTab,
               OpenInTab,
               Configure,
               Properties,
               EditPath,
               NewWindow,
               SortName,
               SortSize,
               SortDate,
               SortType,
               SortDescending,
               GetFilePath,
               GetFileName,
               ActionCount
             };

enum IOTask { CopyTask = 0, MoveTask = 1, RemoveTask = 2 };

enum SearchMode { Filter = 0, Search = 1 };

namespace Docks { enum Pos { Left = 1, Right = 2, Bottom = 4 }; }

namespace FS
{
enum Roles
{
    FileIconRole = Qt::DecorationRole,
    FileNameRole = Qt::DisplayRole,
    FilePathRole = Qt::UserRole + 1,
    FilePermissionsRole = Qt::UserRole + 2,
    CategoryRole = Qt::UserRole + 3,
    MimeTypeRole = Qt::UserRole + 4,
    FileTypeRole = Qt::UserRole + 5,
    OwnderRole = Qt::UserRole + 6,
    UrlRole = Qt::UserRole + 7,
    LastModifiedRole = Qt::UserRole + 8
};
}

struct IOJobData
{
    QString inPaths, outPath; //used for interprocess communication
    QStringList inList;
    IOTask ioTask;
};

static int defaultInteger = 0;
static bool defaultBool = false;
static QUrl defaultUrl = QUrl();
static QDir::Filters allEntries = QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System;

template<typename T, typename Class>
static inline T call(Class *object, T (Class::*function)())
{
    return (object->*function)();
}

template<typename T, typename Class, typename FirstType>
static inline T call(Class *object, T (Class::*function)(FirstType), FirstType arg1)
{
    return (object->*function)(arg1);
}

template<typename T, typename Class, typename FirstType, typename SecondType>
static inline T call(Class *object, T (Class::*function)(FirstType, SecondType), FirstType arg1, SecondType arg2)
{
    return (object->*function)(arg1, arg2);
}

template<typename T, typename Class, typename FirstType, typename SecondType, typename ThirdType>
static inline T call(Class *object, T (Class::*function)(FirstType, SecondType, ThirdType), FirstType arg1, SecondType arg2, ThirdType arg3)
{
    return (object->*function)(arg1, arg2, arg3);
}

template<typename T, typename Class, typename FirstType, typename SecondType, typename ThirdType, typename FourthType>
static inline T call(Class *object, T (Class::*function)(FirstType, SecondType, ThirdType, FourthType), FirstType arg1, SecondType arg2, ThirdType arg3, FourthType arg4)
{
    return (object->*function)(arg1, arg2, arg3, arg4);
}

}

#endif // GLOBALS_H
