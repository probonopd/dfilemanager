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

bool
ThumbsPDF::thumb(const QString &file, const int size, QImage &thumb)
{   
    if ( QFileInfo(file).size() > maxSize || !canRead(file) )
        return false;

    try
    {
        m_image.read(QString("%1[0]").arg(file).toLocal8Bit().data());
        Magick::Blob blob;
        m_image.magick( "PNG" );
        m_image.write( &blob );
        if ( thumb.loadFromData((uchar*)blob.data(), blob.length(), "PNG") )
            return true;
    }
    catch ( Magick::Exception &error )
    {
        qDebug() << error.what();
        return false;
    }
    return false;
}
