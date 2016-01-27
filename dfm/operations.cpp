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
#include "application.h"
#include "helpers.h"
#include <QFileInfo>
#include <QVector>
#include <QDebug>
#include <QProcess>
#include <QApplication>
#include <QAction>
#include <QItemSelectionModel>
#include <QDesktopServices>
#include <QClipboard>
#include <QAbstractItemView>
#include <QPainter>
#include "viewcontainer.h"
#include "fsmodel.h"
#include <math.h>

#if defined(HASMAGIC)
#include <magic.h>
#endif

//#include <blkid/blkid.h> //dev block id info... maybe need later

using namespace DFM;

Ops *Ops::m_instance = 0;

Ops
*Ops::instance()
{
    if (!m_instance)
        m_instance = new Ops();
    return m_instance;
}
/*
// Exponential blur, Jani Huhtanen, 2006 ==========================
*  expblur(QImage &img, int radius)
*
*  In-place blur of image 'img' with kernel of approximate radius 'radius'.
*  Blurs with two sided exponential impulse response.
*
*  aprec = precision of alpha parameter in fixed-point format 0.aprec
*  zprec = precision of state parameters zR,zG,zB and zA in fp format 8.zprec
*/

template<int aprec, int zprec>
static inline void blurinner(unsigned char *bptr, int &zR, int &zG, int &zB, int &zA, int alpha)
{
    int R,G,B,A;
    R = *bptr;
    G = *(bptr+1);
    B = *(bptr+2);
    A = *(bptr+3);

    zR += (alpha * ((R<<zprec)-zR))>>aprec;
    zG += (alpha * ((G<<zprec)-zG))>>aprec;
    zB += (alpha * ((B<<zprec)-zB))>>aprec;
    zA += (alpha * ((A<<zprec)-zA))>>aprec;

    *bptr =     zR>>zprec;
    *(bptr+1) = zG>>zprec;
    *(bptr+2) = zB>>zprec;
    *(bptr+3) = zA>>zprec;
}

template<int aprec,int zprec>
static inline void blurrow( QImage & im, int line, int alpha)
{
    int zR,zG,zB,zA;

    QRgb *ptr = (QRgb *)im.scanLine(line);

    zR = *((unsigned char *)ptr    )<<zprec;
    zG = *((unsigned char *)ptr + 1)<<zprec;
    zB = *((unsigned char *)ptr + 2)<<zprec;
    zA = *((unsigned char *)ptr + 3)<<zprec;

    for(int index=1; index<im.width(); index++)
        blurinner<aprec,zprec>((unsigned char *)&ptr[index],zR,zG,zB,zA,alpha);

    for(int index=im.width()-2; index>=0; index--)
        blurinner<aprec,zprec>((unsigned char *)&ptr[index],zR,zG,zB,zA,alpha);
}

template<int aprec, int zprec>
static inline void blurcol( QImage & im, int col, int alpha)
{
    int zR,zG,zB,zA;

    QRgb *ptr = (QRgb *)im.bits();
    ptr+=col;

    zR = *((unsigned char *)ptr    )<<zprec;
    zG = *((unsigned char *)ptr + 1)<<zprec;
    zB = *((unsigned char *)ptr + 2)<<zprec;
    zA = *((unsigned char *)ptr + 3)<<zprec;

    for(int index=im.width(); index<(im.height()-1)*im.width(); index+=im.width())
        blurinner<aprec,zprec>((unsigned char *)&ptr[index],zR,zG,zB,zA,alpha);

    for(int index=(im.height()-2)*im.width(); index>=0; index-=im.width())
        blurinner<aprec,zprec>((unsigned char *)&ptr[index],zR,zG,zB,zA,alpha);
}

void
Ops::expblur(QImage &img, int radius, Qt::Orientations o)
{
    if(radius<1)
        return;

    static const int aprec = 16; static const int zprec = 7;

    // Calculate the alpha such that 90% of the kernel is within the radius. (Kernel extends to infinity)
    int alpha = (int)((1<<aprec)*(1.0f-expf(-2.3f/(radius+1.f))));

    if (o & Qt::Horizontal) {
        for(int row=0;row<img.height();row++)
            blurrow<aprec,zprec>(img,row,alpha);
    }

    if (o & Qt::Vertical) {
        for(int col=0;col<img.width();col++)
            blurcol<aprec,zprec>(img,col,alpha);
    }
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
    if (!f.exists())
        return false;

    if (f.isDir())
        return false;
    else if (f.isExecutable() && (f.suffix().isEmpty() || f.suffix() == "sh" || f.suffix() == "exe" || DMimeProvider().getMimeType(file).contains("application", Qt::CaseInsensitive)))
        return QProcess::startDetached(f.filePath());
    else
        return QDesktopServices::openUrl(QUrl::fromLocalFile(f.filePath()));
}

void
Ops::openWith()
{
    QAction *action = static_cast<QAction *>(sender());
    QString program;
    const QString &file = action->property("file").toString();
#if defined(ISUNIX)
    program = action->data().toString().split(" ").at(0);
    QProcess::startDetached(program, QStringList() << file);
#else
    program = action->data().toString();
    program.prepend("cmd /C start ");
    wchar_t *command = (wchar_t *)program.toStdWString().data();

    if(!CreateProcess(NULL,
        command, //command to execute
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,  // dont show the cmd window....
        NULL,
        NULL,
        new STARTUPINFO(),
        new PROCESS_INFORMATION())
   )
        qDebug() << "couldnt launch" << program;
#endif
}

void
Ops::customActionTriggered()
{
    QStringList action(static_cast<QAction *>(sender())->data().toString().split(" "));
    const QString &app = action.takeFirst();
    ViewContainer *container = MainWindow::currentContainer();
    FS::Model *fsModel = container->model();
    QItemSelectionModel *isModel = container->currentView()->selectionModel();

    if (isModel->hasSelection())
    {
        if (isModel->selectedIndexes().count())
            foreach (const QModelIndex &index, isModel->selectedIndexes())
            {
                if (index.column())
                    continue;
                const QFileInfo &fi = fsModel->fileInfo(index);
                if (fi.exists())
                {
                    qDebug() << "trying to launch" << app << "in" << fi.filePath();
                    QProcess::startDetached(app, QStringList() << action << fi.filePath());
                }
            }
    }
    else
    {
        const QAbstractItemView *view = container->currentView();
        QFileInfo fi(fsModel->rootUrl().toLocalFile());
        if (view || !fi.exists())
            fi = QFileInfo(view->rootIndex().data(FS::FilePathRole).toString());
        if (fi.exists())
        {
            qDebug() << "trying to launch" << app << "in" << fi.filePath();
            QProcess::startDetached(app, QStringList() << action << fi.filePath());
        }
    }

}

void
Ops::setRootPath()
{
    QAction *action = static_cast<QAction *>(sender());
    MainWindow::currentContainer()->model()->setUrl(QUrl::fromLocalFile(action->data().toString()));
}

QString
Ops::prettySize(quint64 bytes)
{
  if (bytes & (0x3ffful<<50))
     return QString::number((bytes>>50) + ((bytes>>40) & (0x3fful)) / 1024.0, 'f', 2) + " PB";
  else if (bytes & (0x3fful<<40))
     return QString::number((bytes>>40) + ((bytes>>30) & (0x3fful)) / 1024.0, 'f', 2) + " TB";
  else if (bytes & (0x3fful<<30))
     return QString::number((bytes>>30) + ((bytes>>20) & (0x3fful)) / 1024.0, 'f', 2) + " GB";
  else if (bytes & (0x3fful<<20))
     return QString::number((bytes>>20) + ((bytes>>10) & (0x3fful)) / 1024.0, 'f', 2) + " MB";
  else if (bytes & (0x3fful<<10))
     return QString::number((bytes>>10) + ((bytes) & (0x3fful)) / 1024.0, 'f', 2) + " kB";
  else
     return QString::number(bytes, 'f', 0) + " B ";
}

/* blurring function below from:
 * http://stackoverflow.com/questions/3903223/qt4-how-to-blur-qpixmap-image
 * unclear to me who wrote it.
 */
QImage
Ops::blurred(const QImage& image, const QRect& rect, int radius, bool alphaOnly)
{
    if (image.isNull())
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

#define SIZE 258.0f

QImage
Ops::flowImg(const QImage &image)
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
Ops::reflection(const QImage &img)
{
    if (img.isNull())
        return QImage();
    QImage refl(QSize(SIZE, SIZE), QImage::Format_ARGB32);
    refl.fill(Qt::transparent);
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
    for (int i = 0; i < size; ++i)
        if (qAlpha(pixel[i]))
        {
            QColor c = QColor(pixel[i]);
            c = DFM::Ops::colorMid(c, bg, 1, 4);
            pixel[i] = qRgba(c.red(), c.green(), c.blue(), qAlpha(pixel[i]));
        }
    return Ops::blurred(refl, refl.rect(), 5);
}

void
Ops::getSorting(const QString &file, int &sortCol, Qt::SortOrder &order)
{
    const QDir dir(file);
    QSettings settings(dir.absoluteFilePath(".directory"), QSettings::IniFormat);
    settings.beginGroup("DFM");
    QVariant varCol = settings.value("sortCol");
    QVariant varOrd = settings.value("sortOrd");
    if (varCol.isValid() && varOrd.isValid())
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
    QFileInfo fi(file);
    if (fi.exists())
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

IOTask
Ops::taskFromString(const QString &task)
{
    if (task.toLower() == "--cp")
        return DFM::CopyTask;
    else if (task.toLower() == "--mv")
        return DFM::MoveTask;
    else if (task.toLower() == "--rm")
        return DFM::RemoveTask;
    return DFM::CopyTask;
}

QString
Ops::taskToString(const IOTask &task)
{
    switch (task)
    {
    case CopyTask: return "--cp";
    case MoveTask: return "--mv";
    case RemoveTask: return "--rm";
    }
    return QString();
}

bool
Ops::extractIoData(const QStringList &args, IOJobData &ioJobData)
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
    ioJobData.inList = args.at(3).split(SEP, QString::SkipEmptyParts);
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

bool
Ops::sameDisk(const QString &file1, const QString &file2)
{
#if defined(HASSYS)
    const quint64 id1 = getDriveInfo<Id>(file1);
    const quint64 id2 = getDriveInfo<Id>(file2);
#else //on windows we can simply match the drive letter... the first char in the strings
    const QChar &id1 = file1.at(0);
    const QChar &id2 = file2.at(0);
#endif
    return bool(id1==id2);

}

QString
Ops::getStatusBarMessage(const QUrl &url, FS::Model *model)
{
    QString messaage;
    if (url.isLocalFile() || url.scheme() == "search")
    {
        quint64 size = 0;
        int dirCount = 0, fileCount = 0;
        const QModelIndex &parent = model->index(url);
        for (int i = 0; i < model->rowCount(parent); ++i)
        {
            const QModelIndex &index = model->index(i, 0, parent);
            if (!index.isValid())
                continue;
            const QFileInfo &f = model->fileInfo(index);
            if (f.isDir())
            {
                ++dirCount;
            }
            else
            {
                ++fileCount;
                size += f.size();
            }
        }
        if (dirCount)
            messaage.append(QString("%1 %2%3").arg(QString::number(dirCount), dirCount==1?"Folder":"Folders", fileCount?", ":""));
        if (fileCount)
            messaage.append(QString("%1 %2").arg(QString::number(fileCount), fileCount==1?"File":"Files"));
        if (size && url.isLocalFile())
            messaage.append(QString(" ( %1 )").arg(Ops::prettySize(size)));
    }
    else
        messaage.append(url.scheme());
    return messaage;
}

void
Ops::getPathToClipBoard()
{
    MainWindow *mw = MainWindow::currentWindow();
    if (!mw)
        return;
    int role = FS::FilePathRole;
    if (sender() == mw->action(GetFileName))
        role = FS::FileNameRole;
    ViewContainer *c = mw->activeContainer();
    FS::Model *m = c->model();
    const QModelIndexList &selection = c->currentView()->selectionModel()->selectedRows(0);
    if (selection.isEmpty())
        QApplication::clipboard()->setText(m->index(m->rootUrl()).data(role).toString());
    else
    {
        QString files;
        foreach (const QModelIndex &index, selection)
        {
            if (!files.isEmpty())
                files.append(QString("\n"));
            files.append(index.data(role).toString());
        }
        QApplication::clipboard()->setText(files);
    }
}

