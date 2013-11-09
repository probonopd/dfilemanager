#include "thumbsimages.h"
#include <QImageReader>
#include <QFileInfo>

Q_EXPORT_PLUGIN2(thumbsimages, ThumbsImages)

void
ThumbsImages::init()
{
    //called once when plugin loaded
}

ThumbsImages::~ThumbsImages()
{
    //destructor
}

QImage
ThumbsImages::thumb(const QString &file, const int size)
{
    QImageReader ir(file);
    if ( !ir.canRead() )
        return QImage();

    QSize thumbsize(ir.size());
    if ( qMax( thumbsize.width(), thumbsize.height() ) > size )
        thumbsize.scale( size, size, Qt::KeepAspectRatio );
    ir.setScaledSize(thumbsize);

    return ir.read();
}

static QStringList suf;

QStringList
ThumbsImages::suffixes() const
{
    if ( suf.isEmpty() )
        for ( int i = 0; i < QImageReader::supportedImageFormats().count(); ++i )
            suf << QString(QImageReader::supportedImageFormats().at(i));
    return suf;
}

bool
ThumbsImages::canRead(const QString &file) const
{
    return suffixes().contains(QFileInfo(file).suffix(), Qt::CaseInsensitive);
}

QString
ThumbsImages::name() const
{
    return QString("ImageThumbs");
}

QString
ThumbsImages::description() const
{
    return QString("Thumbnailer for images using only Qt classes.");
}
