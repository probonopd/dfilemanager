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


#include "operations.h"
#include "mainwindow.h"
#include <QFileInfo>
#include <QVector>
#include <QDebug>
#include <QProcess>
#include <QApplication>
#include <QAction>
#include <QItemSelectionModel>
#include <QDesktopServices>

//#include <blkid/blkid.h> //dev block id info... maybe need later

using namespace DFM;

static Ops *s_instance = 0;
#ifdef Q_WS_X11
static magic_t s_magicMime;
static magic_t s_magicAll;
#endif

Ops
*Ops::instance()
{
    if ( !s_instance )
    {
        s_instance = new Ops(qApp);
#ifdef Q_OS_UNIX
        s_magicMime = magic_open( MAGIC_MIME_TYPE );
        magic_load( s_magicMime, NULL );
        s_magicAll = magic_open( MAGIC_CONTINUE );
        magic_load( s_magicAll, NULL );
#endif
    }
    return s_instance;
}

QString
Ops::getMimeType(const QString &file)
{
    const QFileInfo f(file);
    if ( !f.isReadable() || !instance() )
        return QString();

    if (f.isSymLink())
        return "inode/symlink";
    if (f.isDir())
        return "inode/directory";
#ifdef Q_OS_UNIX
    return QString( magic_file( s_magicMime, file.toStdString().c_str() ) );;
#else
    return QString();
#endif
}

QString
Ops::getFileType(const QString &file)
{
#ifdef Q_OS_UNIX
    if ( !QFileInfo(file).exists() || !instance() )
        return QString();
    return QString( magic_file( s_magicAll, file.toStdString().c_str() ) );
#else
    return QString();
#endif
}

QColor
Ops::colorMid(const QColor c1, const QColor c2, int i1, int i2)
{
    int r,g,b,a;
    int i3 = i1+i2;
    r = qMin(255,(i1*c1.red() + i2*c2.red())/i3);
    g = qMin(255,(i1*c1.green() + i2*c2.green())/i3);
    b = qMin(255,(i1*c1.blue() + i2*c2.blue())/i3);
    a = qMin(255,(i1*c1.alpha() + i2*c2.alpha())/i3);
    return QColor(r,g,b,a);
}

bool
Ops::openFile(const QString &file)
{
    const QFileInfo f(file);
    if ( !f.exists() )
        return false;

    if (f.isDir())
        return false;
    else if (f.isExecutable() && ( f.suffix().isEmpty() || f.suffix() == "sh" || f.suffix() == "exe" ))
        return QProcess::startDetached(f.filePath());
    else
        return QDesktopServices::openUrl(QUrl::fromLocalFile(f.filePath()));
}

void
Ops::openWith()
{
    QAction *action = static_cast<QAction *>( sender() );
    QString program;
    const QString &file = action->property("file").toString();
#if defined(Q_OS_UNIX)
    program = action->data().toString().split( " " ).at( 0 );
    QProcess::startDetached( program, QStringList() << file );
#else
    program = action->data().toString();
    program.prepend("cmd /C start ");
    wchar_t *command = (wchar_t *)program.toStdWString().data();

    if( !CreateProcess( NULL,
        command, //command to execute
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,  // dont show the cmd window....
        NULL,
        NULL,
        new STARTUPINFO(),
        new PROCESS_INFORMATION() )
    )
        qDebug() << "couldnt launch" << program;
#endif
}

void
Ops::customActionTriggered()
{
    QStringList action(static_cast<QAction *>(sender())->data().toString().split(" "));
    const QString &app = action.takeFirst();

    FileSystemModel *fsModel = MainWindow::currentContainer()->model();
    QItemSelectionModel *isModel = MainWindow::currentContainer()->selectionModel();

    if ( isModel->hasSelection() )
    {
        if ( isModel->selectedIndexes().count() )
            foreach ( const QModelIndex &index, isModel->selectedIndexes() )
                if ( !index.column() )
                    QProcess::startDetached(app, QStringList() << action << fsModel->filePath( index ));
    }
    else
    {
        QProcess::startDetached(app, QStringList() << action << fsModel->rootPath());
    }
}

void
Ops::setRootPath()
{
    QAction *action = static_cast<QAction *>(sender());
    MainWindow::currentContainer()->model()->setPath(action->data().toString());
}

QString
Ops::prettySize(quint64 bytes)
{
  if (bytes & (0x3fff<<50))
     return QString::number( (bytes>>50) + ((bytes>>40) & (0x3ff)) / 1024.0, 'f', 2 ) + " PB";
  else if (bytes & (0x3ff<<40))
     return QString::number( (bytes>>40) + ((bytes>>30) & (0x3ff)) / 1024.0, 'f', 2 ) + " TB";
  else if (bytes & (0x3ff<<30))
     return QString::number( (bytes>>30) + ((bytes>>20) & (0x3ff)) / 1024.0, 'f', 2 ) + " GB";
  else if (bytes & (0x3ff<<20))
     return QString::number( (bytes>>20) + ((bytes>>10) & (0x3ff)) / 1024.0, 'f', 2 ) + " MB";
  else if (bytes & (0x3ff<<10))
     return QString::number( (bytes>>10) + ((bytes) & (0x3ff)) / 1024.0, 'f', 2 ) + " kB";
  else
     return QString::number( bytes, 'f', 0 ) + " B ";
}

/* blurring function below from:
 * http://stackoverflow.com/questions/3903223/qt4-how-to-blur-qpixmap-image
 * unclear to me who wrote it.
 */
QImage
Ops::blurred(const QImage& image, const QRect& rect, int radius, bool alphaOnly)
{
    if ( image.isNull() )
        return QImage();
   int tab[] = { 14, 10, 8, 6, 5, 5, 4, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2 };
   int alpha = (radius < 1)  ? 16 : (radius > 17) ? 1 : tab[radius-1];

   QImage result = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
   int r1 = rect.top();
   int r2 = rect.bottom();
   int c1 = rect.left();
   int c2 = rect.right();

   int bpl = result.bytesPerLine();
   int rgba[4];
   unsigned char* p;

   int i1 = 0;
   int i2 = 3;

   if (alphaOnly)
       i1 = i2 = (QSysInfo::ByteOrder == QSysInfo::BigEndian ? 0 : 3);

   for (int col = c1; col <= c2; col++) {
       p = result.scanLine(r1) + col * 4;
       for (int i = i1; i <= i2; i++)
           rgba[i] = p[i] << 4;

       p += bpl;
       for (int j = r1; j < r2; j++, p += bpl)
           for (int i = i1; i <= i2; i++)
               p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
   }

   for (int row = r1; row <= r2; row++) {
       p = result.scanLine(row) + c1 * 4;
       for (int i = i1; i <= i2; i++)
           rgba[i] = p[i] << 4;

       p += 4;
       for (int j = c1; j < c2; j++, p += 4)
           for (int i = i1; i <= i2; i++)
               p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
   }

   for (int col = c1; col <= c2; col++) {
       p = result.scanLine(r2) + col * 4;
       for (int i = i1; i <= i2; i++)
           rgba[i] = p[i] << 4;

       p -= bpl;
       for (int j = r1; j < r2; j++, p -= bpl)
           for (int i = i1; i <= i2; i++)
               p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
   }

   for (int row = r1; row <= r2; row++) {
       p = result.scanLine(row) + c2 * 4;
       for (int i = i1; i <= i2; i++)
           rgba[i] = p[i] << 4;

       p -= 4;
       for (int j = c1; j < c2; j++, p -= 4)
           for (int i = i1; i <= i2; i++)
               p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
   }

   return result;
}

QImage
Ops::flowImg( const QImage &image )
{
   QImage p(SIZE, SIZE, QImage::Format_ARGB32);
   p.fill(Qt::transparent);
   QPainter pt(&p);
   QRect r = image.rect();
   r.moveCenter(p.rect().center());
   r.moveBottom(p.rect().bottom()-1);
   pt.setBrushOrigin(r.topLeft());
   pt.fillRect(r, image);
   pt.end();
   return p;
}

QImage
Ops::reflection( const QImage &img )
{
    if ( img.isNull() )
        return QImage();
    QImage refl(QSize(SIZE, SIZE), QImage::Format_ARGB32);
    refl.fill( Qt::transparent);
    QRect r = img.rect();
    if (!r.isValid())
        return QImage();

    r.moveCenter(refl.rect().center());
    r.moveTop(refl.rect().top());
    QPainter p(&refl);
    p.setBrushOrigin(r.topLeft());
    p.fillRect(r, img.mirrored());
    p.end();
    int size = refl.width() * refl.height();
    QRgb *pixel = reinterpret_cast<QRgb *>(refl.bits());
    QColor bg = DFM::Ops::colorMid(qApp->palette().color(QPalette::Highlight), Qt::black);
    bg.setHsv(bg.hue(), qMin(64, bg.saturation()), bg.value(), bg.alpha());
    for ( int i = 0; i < size; ++i )
        if ( qAlpha(pixel[i]) )
        {
            QColor c = QColor(pixel[i]);
            c = DFM::Ops::colorMid(c, bg, 1, 4);
            pixel[i] = qRgba(c.red(), c.green(), c.blue(), qAlpha(pixel[i]));
        }
    return Ops::blurred( refl, refl.rect(), 5 );
}

void
Ops::getSorting(const QString &file, int &sortCol, Qt::SortOrder &order)
{
    const QDir dir(file);
    QSettings settings(dir.absoluteFilePath(".directory"), QSettings::IniFormat);
    settings.beginGroup("DFM");
    QVariant varCol = settings.value("sortCol");
    QVariant varOrd = settings.value("sortOrd");
    if ( varCol.isValid() && varOrd.isValid() )
    {
        sortCol = varCol.value<int>();
        order = (Qt::SortOrder)varOrd.value<int>();
    }
    settings.endGroup();
}

static QStringList check = QStringList() << "~" << "$HOME";
static QStringList replace = QStringList() << QDir::homePath() << QDir::homePath();

QString
Ops::sanityChecked(const QString &file)
{
    if (QFileInfo(file).exists())
        return file;

    QString newFile = QDir::fromNativeSeparators(file);
    for (int i=0; i<check.count(); ++i)
        if (newFile.contains(check.at(i), Qt::CaseInsensitive))
        {
            newFile.replace(check.at(i), replace.at(i), Qt::CaseInsensitive);
            if (QFileInfo(newFile).exists())
                return newFile;
        }
    return newFile;
}

IOTask Ops::taskFromString(const QString &task)
{
    if (task.toLower() == "--cp")
        return DFM::CopyTask;
    else if (task.toLower() == "--mv")
        return DFM::MoveTask;
    else if (task.toLower() == "--rm")
        return DFM::RemoveTask;
    return DFM::CopyTask;
}

QString Ops::taskToString(const IOTask &task)
{
    switch (task)
    {
    case CopyTask: return "--cp";
    case MoveTask: return "--mv";
    case RemoveTask: return "--rm";
    }
    return QString();
}

bool Ops::extractIoData(const QStringList &args, IOJobData &ioJobData)
{
    /* the syntax for copying files w/ dfm should
     * look something like this:
     * dfm --iojob --cp /path/to/infile /path/to/outfile
     * so if we have less then 5 args to play
     * w/ then we can safely assume we are not
     * a iojob. 4 if we are a remove job tho.
     */
    if (args.count() < 4) //we are not a IOJob
        return false;
    if (args.at(1).toLower() != "--iojob")
        return false;

    ioJobData.ioTask = taskFromString(args.at(2));
    ioJobData.inPaths = args.at(3);
    ioJobData.inList = args.at(3).split(",", QString::SkipEmptyParts);
    ioJobData.outPath = args.at(4);
    return true;
}

QStringList
Ops::fromIoJobData(const IOJobData &data)
{
    QStringList args = QStringList() << "--iojob";
    args << taskToString(data.ioTask);
    args << data.inPaths;
    args << data.outPath;
    return args;
}

