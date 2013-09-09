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


#include "propertiesdialog.h"
#include "operations.h"
#include "placesview.h"
#include "mainwindow.h"
#include <QDateTime>
#include <QGroupBox>
#include <QDirIterator>
#include <QTextLayout>

SizeThread::SizeThread(QObject *parent, const QString &file)
    : QThread(parent)
    , m_timer(new QTimer(this))
    , m_file(file)
    , m_subDirs(0)
    , m_files(0)
    , m_size(0)
{
    connect( m_timer, SIGNAL(timeout()), this, SLOT(emitSize()) );
    m_timer->start(200);
    start();
}

void SizeThread::run() { calculate(m_file); }

void
SizeThread::calculate( const QString &file )
{
    const QFileInfo f(file);

    if ( f.isDir() )
    {
        const QDir::Filters &filter = QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System;
        const QFileInfoList &entries = QDir(file).entryInfoList(filter);
        foreach ( const QFileInfo &entry, entries )
        {
            QString file(entry.filePath());
            if (entry.isDir())
                ++m_subDirs;
            calculate(file);
        }
    }
    else
    {
        m_size += f.size();
        ++m_files;
    }
}

void
SizeThread::emitSize()
{
    QString s;
    s.append(DFM::Ops::prettySize(m_size));
    s.append(QString("\n%1 byte(s),").arg(QString::number(m_size)));
    s.append(QString("\n%1 file(s),").arg(QString::number(m_files)));
    s.append(QString("\n%1 SubDir(s)").arg(QString::number(m_subDirs)));
    emit newSize(s);
}



GeneralInfo::GeneralInfo(QWidget *parent, const QString &file)
    : QGroupBox(parent)
{
    setTitle(tr("General:"));
    QGridLayout *l = new QGridLayout(this);
    QFileInfo f(file);

    Qt::Alignment left = Qt::AlignLeft|Qt::AlignVCenter,
            right = Qt::AlignRight|Qt::AlignVCenter;

    int row = 0;

    l->addWidget(m_nameEdit = new QLineEdit(f.fileName()), row, 0, 1, -1, left);
    m_nameEdit->setMinimumWidth(256);

    l->addWidget(new QLabel(tr("Mimetype:"), this), ++row, 0, right);
    l->addWidget(new QLabel(DFM::Ops::getMimeType(file), this), row, 1, left);

    l->addWidget(new QLabel(tr("Location:"), this), ++row, 0, right);
    l->addWidget(new QLabel(f.path(), this), row, 1, left);

    l->addWidget(new QLabel(tr("Size:"), this), ++row, 0, right);
    QLabel *sl = new QLabel(DFM::Ops::prettySize(f.size()), this);
    l->addWidget(sl, row, 1, left);

    l->addWidget(new QLabel(tr("Created:"), this), ++row, 0, right);
    l->addWidget(new QLabel(f.created().toString(), this), row, 1, left);

    l->addWidget(new QLabel(tr("Read:"), this), ++row, 0, right);
    l->addWidget(new QLabel(f.lastRead().toString(), this), row, 1, left);

    l->addWidget(new QLabel(tr("Modified:"), this), ++row, 0, right);
    l->addWidget(new QLabel(f.lastModified().toString(), this), row, 1, left);

    l->addWidget(new QLabel(tr("MountPath:"), this), ++row, 0, right);
    l->addWidget(new QLabel(DFM::MainWindow::places()->deviceManager()->deviceItemForFile(file)->mountPath(), this), row, 1, left);

    if ( QFileInfo(file).isDir() )
        connect( (new SizeThread(this, file)), SIGNAL(newSize(QString)), sl, SLOT(setText(QString)));
}


Rights::Rights(QWidget *parent, const QString &file)
    : QGroupBox(parent)
{
    setTitle(tr("Permissions:"));
    QGridLayout *l = new QGridLayout(this);

    l->addWidget(new QLabel(tr("Read"), this), 0, 1);
    l->addWidget(new QLabel(tr("Write"), this), 0, 2);
    l->addWidget(new QLabel(tr("Exec"), this), 0, 3);

    l->addWidget(new QLabel(tr("User:"), this), 1, 0, Qt::AlignRight);
    l->addWidget(new QLabel(tr("Group:"), this), 2, 0, Qt::AlignRight);
    l->addWidget(new QLabel(tr("Other:"), this), 3, 0, Qt::AlignRight);

    l->addWidget(m_box[UserRead] = new QCheckBox(this), 1, 1);
    l->addWidget(m_box[UserWrite] = new QCheckBox(this), 1, 2);
    l->addWidget(m_box[UserExe] = new QCheckBox(this), 1, 3);

    l->addWidget(m_box[GroupRead] = new QCheckBox(this), 2, 1);
    l->addWidget(m_box[GroupWrite] = new QCheckBox(this), 2, 2);
    l->addWidget(m_box[GroupExe] = new QCheckBox(this), 2, 3);

    l->addWidget(m_box[OthersRead] = new QCheckBox(this), 3, 1);
    l->addWidget(m_box[OthersWrite] = new QCheckBox(this), 3, 2);
    l->addWidget(m_box[OthersExe] = new QCheckBox(this), 3, 3);

    QFileInfo f(file);
    m_box[UserRead]->setChecked(f.permission(QFile::ReadUser));
    m_box[UserWrite]->setChecked(f.permission(QFile::WriteUser));
    m_box[UserExe]->setEnabled(!f.isDir());
    m_box[UserExe]->setChecked(!f.isDir() && f.permission(QFile::ExeUser));

    m_box[GroupRead]->setChecked(f.permission(QFile::ReadGroup));
    m_box[GroupWrite]->setChecked(f.permission(QFile::WriteGroup));
    m_box[GroupExe]->setEnabled(!f.isDir());
    m_box[GroupExe]->setChecked(!f.isDir() && f.permission(QFile::ExeGroup));

    m_box[OthersRead]->setChecked(f.permission(QFile::ReadOther));
    m_box[OthersWrite]->setChecked(f.permission(QFile::WriteOther));
    m_box[OthersExe]->setEnabled(!f.isDir());
    m_box[OthersExe]->setChecked(!f.isDir() && f.permission(QFile::ExeOther));
}

PreViewWidget::PreViewWidget(QWidget *parent, const QString &file)
    : QGroupBox(parent)
{
    setTitle(tr("Preview:"));
    QHBoxLayout *l = new QHBoxLayout(this);
    QLabel *label = new QLabel(this);
    l->addStretch();
    l->addWidget(label);
    l->addStretch();
    label->setFixedSize(256, 256);
    DFM::FileSystemModel *fsModel = DFM::MainWindow::currentContainer()->model();
    const QModelIndex &index = fsModel->index(file);
    if ( index.isValid() )
    {
        const QImage &img = qvariant_cast<QImage>(fsModel->data(index, DFM::FileSystemModel::Thumbnail));
        if ( !img.isNull() )
        {
            label->setPixmap(QPixmap::fromImage(img));
            label->setFixedSize(img.size());
            return;
        }
    }

    QFile f(file);
    QString s;
    if ( f.open(QFile::ReadOnly) && DFM::Ops::getMimeType(file).contains("text") )
    {
        while ( !f.atEnd() )
            s.append(QString(f.readLine()));
    }
    f.close();
    label->setText(s.isEmpty() ? tr("Preview not available") : s);
    label->setAlignment(Qt::AlignTop|Qt::AlignLeft);
    if ( s.isEmpty() )
        hide();
}

PropertiesDialog::PropertiesDialog(QWidget *parent, QString filePath)
    : QDialog(parent)
    , m_currentFile(filePath)
{
    QFileInfo fileInfo(filePath);
    setWindowTitle(fileInfo.fileName());
    setSizeGripEnabled(false);

    QPushButton *ok = new QPushButton(tr("OK"),this);
    QPushButton *cancel = new QPushButton(tr("Cancel"),this);

    connect(ok,SIGNAL(released()),this,SLOT(accept()));
    connect(cancel,SIGNAL(released()),this,SLOT(reject()));

    QHBoxLayout *btns = new QHBoxLayout();
    btns->addStretch();
    btns->addWidget(ok);
    btns->addWidget(cancel);
    btns->addStretch();

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setSizeConstraint(QLayout::SetFixedSize);
    vbox->addWidget(m_g = new GeneralInfo(this, filePath));
    vbox->addWidget(new PreViewWidget(this, filePath));
    vbox->addWidget(m_r = new Rights(this, filePath));
    vbox->addLayout(btns);
}


void
PropertiesDialog::accept()
{
    QFile file(m_currentFile);
    QFileInfo fileInfo(m_currentFile);
    if (fileInfo.isWritable())
    {
        QFile::Permissions permissions = file.permissions();
        if (m_r->box(Rights::UserRead)->isChecked()) permissions |= QFile::ReadUser; else permissions &= ~QFile::ReadUser;
        if (m_r->box(Rights::UserWrite)->isChecked()) permissions |= QFile::WriteUser; else permissions &= ~QFile::WriteUser;
        if (m_r->box(Rights::UserExe)->isChecked()) permissions |= QFile::ExeUser; else permissions &= ~QFile::ExeUser;
        if (m_r->box(Rights::GroupRead)->isChecked()) permissions |= QFile::ReadGroup; else permissions &= ~QFile::ReadGroup;
        if (m_r->box(Rights::GroupWrite)->isChecked()) permissions |= QFile::WriteGroup; else permissions &= ~QFile::WriteGroup;
        if (m_r->box(Rights::GroupExe)->isChecked()) permissions |= QFile::ExeGroup; else permissions &= ~QFile::ExeGroup;
        if (m_r->box(Rights::OthersRead)->isChecked()) permissions |= QFile::ReadOther; else permissions &= ~QFile::ReadOther;
        if (m_r->box(Rights::OthersWrite)->isChecked()) permissions |= QFile::WriteOther; else permissions &= ~QFile::WriteOther;
        if (m_r->box(Rights::OthersExe)->isChecked()) permissions |= QFile::ExeOther; else permissions &= ~QFile::ExeOther;
        file.setPermissions(permissions);
        const QString &newFile = QString("%1%2%3").arg(fileInfo.path(), QDir::separator(), m_g->newName());
        if ( newFile != m_currentFile )
            QFile::rename(fileInfo.filePath(), newFile);
    }
    QDialog::accept();
}
