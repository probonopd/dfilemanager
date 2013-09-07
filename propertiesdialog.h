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


#ifndef PROPERTIESDIALOG_H
#define PROPERTIESDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QLayout>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QPushButton>

class PropertiesDialog : public QDialog
{
    Q_OBJECT
public:
    PropertiesDialog( QWidget *parent = 0, QString filePath = QString() );
    
protected:
    void enableButtons(QFileInfo fileInfo);

private slots:
    void accept();

private:
    QCheckBox *ownerRead,*ownerWrite,*ownerExecute,*groupRead,*groupWrite,*groupExecute,*othersRead,*othersWrite,*othersExecute;
    QString currentFile;
    
};

#endif // PROPERTIESDIALOG_H
