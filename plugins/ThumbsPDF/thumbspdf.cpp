#include "thumbspdf.h"
#include <QDebug>
#include <Magick++.h>
#include <Magick++/Blob.h>
#include <QDir>
#include <QFileInfo>
//#include <vector>

Q_EXPORT_PLUGIN2(thumbspdf, ThumbsPDF)

static quint64 maxSize = 10485760;

void
ThumbsPDF::init()
{
    Magick::InitializeMagick(QDir::homePath().toLocal8Bit().data());
}

QImage
ThumbsPDF::thumb(const QString &file, const int size)
{   
    if ( QFileInfo(file).size() > maxSize )
        return QImage();


    try
    {
        m_image.read(QString("%1[0]").arg(file).toLocal8Bit().data());
        Magick::Blob blob;
        m_image.magick( "PNG" );
        m_image.write( &blob );
        QImage img;
        if ( img.loadFromData((uchar*)blob.data(), blob.length(), "PNG") )
            return img.scaled(QSize(size, size), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    catch ( Magick::Exception &error )
    {
        qDebug() << error.what();
        return QImage();
    }

    return QImage();
}
