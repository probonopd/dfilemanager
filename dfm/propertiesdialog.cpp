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
#include "icondialog.h"
#include "dataloader.h"
#include "viewcontainer.h"
#include "fsmodel.h"
#include "helpers.h"
#include "globals.h"
#include <QDateTime>
#include <QGroupBox>
#include <QProgressBar>

SizeThread::SizeThread(QObject *parent, const QStringList &files)
    : QThread(parent)
    , m_timer(new QTimer(this))
    , m_files(files)
    , m_subDirs(0)
    , m_fileCount(0)
    , m_size(0)
{
    connect(m_timer, SIGNAL(timeout()), this, SLOT(emitSize()));
    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
    connect(this, SIGNAL(finished()), m_timer, SLOT(stop()));
    connect(this, SIGNAL(finished()), this, SLOT(emitSize()));
    m_timer->start(200);
    start();
}

void SizeThread::run() { while (m_files.count()) calculate(m_files.takeFirst()); }

void
SizeThread::calculate(const QString &file)
{
    const QFileInfo f(file);

    if (f.isDir())
    {
        const QDir::Filters &filter = QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System;
        const QFileInfoList &entries = QDir(file).entryInfoList(filter);
        foreach (const QFileInfo &entry, entries)
        {
            if (entry.isDir())
                ++m_subDirs;
            calculate(entry.filePath());
        }
    }
    else
    {
        m_size += f.size();
        ++m_fileCount;
    }
}

void
SizeThread::emitSize()
{
    QString s;
    s.append(DFM::Ops::prettySize(m_size));
    s.append(QString("\n%1 byte(s),").arg(QString::number(m_size)));
    s.append(QString("\n%1 file(s),").arg(QString::number(m_fileCount)));
    s.append(QString("\n%1 SubDir(s)").arg(QString::number(m_subDirs)));
    if (sender() == m_timer)
        s.append("\n ... still calculating.");
    emit newSize(s);
}



GeneralInfo::GeneralInfo(QWidget *parent, const QStringList &files)
    : QGroupBox(parent)
{
    setTitle(tr("General:"));
    QGridLayout *l = new QGridLayout(this);

    Qt::Alignment left = Qt::AlignLeft|Qt::AlignVCenter,
            right = Qt::AlignRight|Qt::AlignVCenter;

    int row = 0;
    const bool many = bool(files.count() > 1);
    const QString &file = files[0];
    QFileInfo f(file);

    DFM::FS::Model *fsModel = DFM::MainWindow::currentContainer()->model();
    QString location;
    DFM::DeviceItem *di = 0;
    if (fsModel->rootUrl().isLocalFile() || (!many && f.exists()))
    {
        location = f.path();
        di = DFM::MainWindow::places()->deviceManager()->deviceItemForFile(location);
    }
    else
        location = "unknown";
    if (f.isSymLink())
    {
        location.append(QString("\n(points to: %1)").arg(f.symLinkTarget()));
        di = DFM::MainWindow::places()->deviceManager()->deviceItemForFile(f.symLinkTarget());
    }

    if (many)
    {
        int fileCount = 0, dirCount = 0;
        foreach (const QString &file, files)
            if (QFileInfo(file).isDir())
                ++dirCount;
            else
                ++fileCount;

        QString s;
        if (dirCount && fileCount)
            s = QString("%1 file(s) and %2 dir(s) selected").arg(QString::number(fileCount), QString::number(dirCount));
        else if (dirCount)
            s = QString("%1 dir(s) selected").arg(QString::number(dirCount));
        else
            s = QString("%1 file(s) selected").arg(QString::number(fileCount));
        window()->setWindowTitle(s);
        l->addWidget(new QLabel(s, this), row, 0, 1, 2, left);
    }
    else
    {
        window()->setWindowTitle(f.fileName());
        QToolButton *iconLabel = new QToolButton(this);
        iconLabel->setIcon(fsModel->index(QUrl::fromLocalFile(f.filePath())).data(DFM::FS::FileIconRole).value<QIcon>());
        iconLabel->setToolButtonStyle(Qt::ToolButtonIconOnly);
        iconLabel->setIconSize(QSize(32, 32));
        if (f.isDir())
        {
            iconLabel->setProperty("file", f.filePath());
            connect(iconLabel, SIGNAL(clicked()), this, SLOT(setIcon()));
        }
        else
            iconLabel->setEnabled(false);
        l->addWidget(iconLabel, row, 0, right);
        l->addWidget(m_nameEdit = new QLineEdit(f.fileName()), row, 1, left);
        m_nameEdit->setMinimumWidth(128);

        l->addWidget(new QLabel(tr("Mimetype:"), this), ++row, 0, right);
        l->addWidget(new QLabel(DMimeProvider().getMimeType(file), this), row, 1, left);

        l->addWidget(new QLabel(tr("Created:"), this), ++row, 0, right);
        l->addWidget(new QLabel(f.created().toString(), this), row, 1, left);

        l->addWidget(new QLabel(tr("Read:"), this), ++row, 0, right);
        l->addWidget(new QLabel(f.lastRead().toString(), this), row, 1, left);

        l->addWidget(new QLabel(tr("Modified:"), this), ++row, 0, right);
        l->addWidget(new QLabel(f.lastModified().toString(), this), row, 1, left);
    }

    l->addWidget(new QLabel(tr("Size:"), this), ++row, 0, right);
    QLabel *sl = new QLabel(this);
    l->addWidget(sl, row, 1, left);
    if (many || f.isDir())
        connect((new SizeThread(this, files)), SIGNAL(newSize(QString)), sl, SLOT(setText(QString)));
    else
        sl->setText(DFM::Ops::prettySize(f.size()));

    l->addWidget(new QLabel(tr("Location:"), this), ++row, 0, right);
    l->addWidget(new QLabel(location, this), row, 1, left);

    l->addWidget(new QLabel(tr("MountPath:"), this), ++row, 0, right);
    l->addWidget(new QLabel(di?di->mountPath():"", this), row, 1, left);

    QProgressBar *pb = new QProgressBar(this);
    pb->setMinimum(0);
    pb->setMaximum(100);
    pb->setValue(di?di->used():100);
    QString free = QString("%1 Free").arg(DFM::Ops::prettySize(di?di->freeBytes():0));
    pb->setFormat(free);
    pb->setMinimumWidth(128);
    l->addWidget(new QLabel(tr("Device Usage:"), this), ++row, 0, right);
    l->addWidget(pb, row, 1, left);
}

void
GeneralInfo::setIcon()
{
    QToolButton *tb = qobject_cast<QToolButton *>(sender());
    QIcon i = QIcon::fromTheme(DFM::IconDialog::iconName());
    if (!i.isNull() && QIcon::hasThemeIcon(i.name()))
    {
        const QString &file = tb->property("file").toString();
        QSettings settings(QDir(file).absoluteFilePath(".directory"), QSettings::IniFormat);
        settings.setValue("Desktop Entry/Icon", i.name());
        tb->setIcon(i);

        if (DFM::DDataLoader::data(file))
            DFM::DDataLoader::removeData(file);
        DFM::DDataLoader::data(file);
    }
}

Rights::Rights(QWidget *parent, const QStringList &files)
    : QGroupBox(parent)
{
    setTitle(tr("Permissions:"));
    QGridLayout *l = new QGridLayout(this);

    l->addWidget(new QLabel(tr("Read"), this), 0, 1, Qt::AlignCenter);
    l->addWidget(new QLabel(tr("Write"), this), 0, 2, Qt::AlignCenter);
    l->addWidget(new QLabel(tr("Exec"), this), 0, 3, Qt::AlignCenter);

    l->addWidget(new QLabel(tr("User:"), this), 1, 0, Qt::AlignRight);
    l->addWidget(new QLabel(tr("Group:"), this), 2, 0, Qt::AlignRight);
    l->addWidget(new QLabel(tr("Other:"), this), 3, 0, Qt::AlignRight);

    l->addWidget(m_box[UserRead] = new QCheckBox(this), 1, 1, Qt::AlignCenter);
    l->addWidget(m_box[UserWrite] = new QCheckBox(this), 1, 2, Qt::AlignCenter);
    l->addWidget(m_box[UserExe] = new QCheckBox(this), 1, 3, Qt::AlignCenter);

    l->addWidget(m_box[GroupRead] = new QCheckBox(this), 2, 1, Qt::AlignCenter);
    l->addWidget(m_box[GroupWrite] = new QCheckBox(this), 2, 2, Qt::AlignCenter);
    l->addWidget(m_box[GroupExe] = new QCheckBox(this), 2, 3, Qt::AlignCenter);

    l->addWidget(m_box[OthersRead] = new QCheckBox(this), 3, 1, Qt::AlignCenter);
    l->addWidget(m_box[OthersWrite] = new QCheckBox(this), 3, 2, Qt::AlignCenter);
    l->addWidget(m_box[OthersExe] = new QCheckBox(this), 3, 3, Qt::AlignCenter);

    QFileInfo f(files.first());
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

PreViewWidget::PreViewWidget(QWidget *parent, const QStringList &files)
    : QGroupBox(parent)
{
    setTitle(tr("Preview:"));
    QHBoxLayout *l = new QHBoxLayout(this);
    QLabel *label = new QLabel(this);
    l->addStretch();
    l->addWidget(label);
    l->addStretch();
    label->setFixedSize(256, 256);
    const QString &file = files.first();
    DFM::FS::Model *fsModel = DFM::MainWindow::currentContainer()->model();
    const QModelIndex &index = fsModel->index(QUrl::fromLocalFile(file));
    if (index.isValid() && index.data(DFM::FS::FileHasThumbRole).toBool())
    {
        const QPixmap &pix = qvariant_cast<QPixmap>(fsModel->data(index, Qt::DecorationRole));
        if (!pix.isNull())
        {
            label->setPixmap(pix);
            label->setFixedSize(pix.size());
            return;
        }
    }

    QFile f(file);
    QString s;
    if (f.open(QFile::ReadOnly) && DMimeProvider().getMimeType(file).contains("text"))
    {
        while (!f.atEnd())
            s.append(QString(f.readLine()));
    }
    f.close();
    label->setText(s.isEmpty() ? tr("Preview not available") : s);
    label->setAlignment(Qt::AlignTop|Qt::AlignLeft);
    if (s.isEmpty())
        hide();
}

PropertiesDialog::PropertiesDialog(QWidget *parent, const QStringList &files)
    : QDialog(parent)
    , m_files(files)
{
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
    vbox->addWidget(m_g = new GeneralInfo(this, files));
    if (files.count() == 1)
    {
        vbox->addWidget(new PreViewWidget(this, files));
        vbox->addWidget(m_r = new Rights(this, files));
    }
    vbox->addLayout(btns);
}

void
PropertiesDialog::accept()
{
    if (m_files.count() > 1)
    {
        QDialog::accept();
        return;
    }

    QFile file(m_files.first());
    QFileInfo fileInfo(m_files.first());
    if (fileInfo.isWritable())
    {
        QFile::Permissions permissions = file.permissions();
        if (m_r->box(Rights::UserRead)->isChecked()) permissions |= QFile::ReadUser; else permissions &= ~QFile::ReadUser;
        if (m_r->box(Rights::GroupRead)->isChecked()) permissions |= QFile::ReadGroup; else permissions &= ~QFile::ReadGroup;
        if (m_r->box(Rights::OthersRead)->isChecked()) permissions |= QFile::ReadOther; else permissions &= ~QFile::ReadOther;

        if (m_r->box(Rights::UserWrite)->isChecked()) permissions |= QFile::WriteUser; else permissions &= ~QFile::WriteUser;
        if (m_r->box(Rights::GroupWrite)->isChecked()) permissions |= QFile::WriteGroup; else permissions &= ~QFile::WriteGroup;
        if (m_r->box(Rights::OthersWrite)->isChecked()) permissions |= QFile::WriteOther; else permissions &= ~QFile::WriteOther;

        if (!fileInfo.isDir())
        {
            if (m_r->box(Rights::UserExe)->isChecked()) permissions |= QFile::ExeUser; else permissions &= ~QFile::ExeUser;
            if (m_r->box(Rights::GroupExe)->isChecked()) permissions |= QFile::ExeGroup; else permissions &= ~QFile::ExeGroup;
            if (m_r->box(Rights::OthersExe)->isChecked()) permissions |= QFile::ExeOther; else permissions &= ~QFile::ExeOther;
        }
        file.setPermissions(permissions);
        const QString &newFile = fileInfo.dir().absoluteFilePath(m_g->newName());
        if (newFile != m_files.first())
            QFile::rename(fileInfo.filePath(), newFile);
    }
    QDialog::accept();
}

void
PropertiesDialog::forFile(const QString &file)
{
    PropertiesDialog(DFM::MainWindow::currentWindow(), QStringList() << file).exec();
}

void
PropertiesDialog::forFiles(const QStringList &files)
{
    if (files.count())
        PropertiesDialog(DFM::MainWindow::currentWindow(), files).exec();
}
