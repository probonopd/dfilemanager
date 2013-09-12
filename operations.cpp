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

//#include <blkid/blkid.h> //dev block id info... maybe need later

using namespace DFM;

static Ops *s_instance = 0;

Ops
*Ops::instance()
{
    if ( !s_instance )
        s_instance = new Ops(qApp);
    return s_instance;
}

QString
Ops::getMimeType(const QString &file)
{
    QFileInfo f(file);
    if ( !f.exists() )
        return QString();

    if ( f.isSymLink())
        return "inode/symlink";
    if ( f.isDir() )
        return "inode/directory";
#ifdef Q_WS_X11
    magic_t magic = magic_open( MAGIC_MIME_TYPE );
    magic_load( magic, NULL );
    return QString( magic_file( magic, file.toLatin1() ) );
#else
    return QString();
#endif
}

QString
Ops::getFileType(const QString &file)
{
#ifdef Q_WS_X11
    if ( !QFileInfo(file).exists() )
        return QString();
    magic_t mgcMime = magic_open( MAGIC_CONTINUE ); //print anything we can get
    magic_load( mgcMime, NULL );
    return QString( magic_file( mgcMime, file.toStdString().c_str() ) );
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

void
Ops::openFile(const QString &file)
{
    if(!QFileInfo(file).exists())
        return;

    QProcess process;
    if(QFileInfo(file).isExecutable() && (QFileInfo(file).suffix().isEmpty() ||
                                                         QFileInfo(file).suffix() == "sh" ||
                                                         QFileInfo(file).suffix() == "exe")) // windows executable
    {
        process.startDetached(file);
    }
    else
    {
        QStringList list;
        list << file;

#ifdef Q_WS_X11 //linux
        process.startDetached("xdg-open",list);
#else //windows
        process.startDetached("cmd /c start " + list.at(0)); //todo: fix, this works but shows a cmd window real quick
#endif
    }
}

void
Ops::openWith()
{
    QAction *action = static_cast<QAction *>( sender() );
    QString program( action->data().toString().split( " " ).at( 0 ) );
    QProcess::startDetached( program, QStringList() << action->property("file").toString() );
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
        if ( isModel->selectedRows().count() )
            foreach ( const QModelIndex &index, isModel->selectedRows() )
                QProcess::startDetached(app, QStringList() << action << fsModel->filePath( index ));
        else if ( isModel->selectedIndexes().count() )
            foreach ( const QModelIndex &index, isModel->selectedIndexes() )
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
    MainWindow::currentContainer()->model()->setRootPath(action->data().toString());
}

QString
Ops::prettySize(quint64 bytes)
{
  if (bytes & (0x3fff<<50))
     return QString::number( (bytes>>50) + ((bytes>>40) & (0x3ff)) / 1024.0, 'f', 2 ) + " PiB";
  else if (bytes & (0x3ff<<40))
     return QString::number( (bytes>>40) + ((bytes>>30) & (0x3ff)) / 1024.0, 'f', 2 ) + " TiB";
  else if (bytes & (0x3ff<<30))
     return QString::number( (bytes>>30) + ((bytes>>20) & (0x3ff)) / 1024.0, 'f', 2 ) + " GiB";
  else if (bytes & (0x3ff<<20))
     return QString::number( (bytes>>20) + ((bytes>>10) & (0x3ff)) / 1024.0, 'f', 2 ) + " MiB";
  else if (bytes & (0x3ff<<10))
     return QString::number( (bytes>>10) + ((bytes) & (0x3ff)) / 1024.0, 'f', 2 ) + " KiB";
  else
     return QString::number( bytes, 'f', 0 ) + " Bytes";
}

/* blurring function below from:
 * http://stackoverflow.com/questions/3903223/qt4-how-to-blur-qpixmap-image
 * unclear to me who wrote it.
 */
QImage
Ops::blurred(const QImage& image, const QRect& rect, int radius, bool alphaOnly)
{
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
   QImage refl(SIZE, SIZE, QImage::Format_ARGB32);
   refl.fill( Qt::transparent);
   QRect r = img.rect();
   r.moveCenter(refl.rect().center());
   r.moveTop(refl.rect().top());
   QPainter p(&refl);
   p.setBrushOrigin(r.topLeft());
   if ( !img.isNull() )
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

