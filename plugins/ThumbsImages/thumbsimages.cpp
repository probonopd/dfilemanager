#include "thumbsimages.h"
#include <QImageReader>

Q_EXPORT_PLUGIN2(thumbsimages, ThumbsImages)

QImage
ThumbsImages::thumb(const QString &file, const int size) const
{
    QImageReader ir(file);
    if ( !ir.canRead() || file.endsWith( "xcf", Qt::CaseInsensitive ) )
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
