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

enum Action { GoBack = 0,
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
               MoveToTrashAction,
               RestoreFromTrashAction,
               RemoveFromTrashAction,
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
    FileIsDirRole,
    FilePermissionsRole,
    FileHasThumbRole,
    FileCategoryRole,
    WatchDirectoryRole,
    CategoryRole,
    CategoryIndexesRole,
    MimeTypeRole,
    FileTypeRole,
    OwnderRole,
    UrlRole,
    LastModifiedRole
};
}

enum { Steps = 8 };

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

}

#endif // GLOBALS_H
