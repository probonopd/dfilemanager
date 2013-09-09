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

#include <QWidget>
#include <QDialog>
#include <QCheckBox>
#include <QLayout>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QThread>
#include <QTimer>
#include <QComboBox>
#include <QLineEdit>

class SizeThread : public QThread
{
    Q_OBJECT
public:
    SizeThread( QObject *parent = 0, const QString &file = QString() );

signals:
    void newSize( const QString &size );

private slots:
    void emitSize();

protected:
    void run();
    void calculate( const QString &file );

private:
    QTimer *m_timer;
    QString m_file;
    quint64 m_size;
    uint m_subDirs, m_files;
};



class GeneralInfo : public QGroupBox
{
    Q_OBJECT
public:
    GeneralInfo( QWidget *parent = 0, const QString &file = QString() );
    inline QString newName() const { return m_nameEdit->text(); }

private:
    QLineEdit *m_nameEdit;
};



class Rights : public QGroupBox
{
    Q_OBJECT
public:
    enum Perm { UserRead = 0,
                UserWrite,
                UserExe,
                GroupRead,
                GroupWrite,
                GroupExe,
                OthersRead,
                OthersWrite,
                OthersExe };
    Rights( QWidget *parent = 0, const QString &file = QString() );
    inline QCheckBox *box( Perm p ) { return m_box[p]; }

private:
    QCheckBox *m_box[OthersExe+1];
};

class PreViewWidget : public QGroupBox
{
    Q_OBJECT
public:
    PreViewWidget( QWidget *parent = 0, const QString &file = QString() );
};

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
    Rights *m_r;
    GeneralInfo *m_g;
    QString m_currentFile;
    
};

#endif // PROPERTIESDIALOG_H
