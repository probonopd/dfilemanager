#include "thumbsimages.h"
#include <QImageReader>
#include <QFileInfo>
#include <QDebug>

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

bool
ThumbsImages::thumb(const QString &file, const int size, QImage &thumb)
{
    QImageReader ir(file);
    if ( !ir.canRead()||!canRead(file) )
        return false;

    //sometimes ir.size() doesnt return a valid size,
    //this is at least true for xcf images,
    //then we have to scale them later after we have
    //the full image. This is slower.
    QSize thumbsize(ir.size());
    if ( thumbsize.isValid() && qMax( thumbsize.width(), thumbsize.height() ) > size )
        thumbsize.scale( size, size, Qt::KeepAspectRatio );
    ir.setScaledSize(thumbsize);

    QImage img(ir.read());

    //if the thumbsize isnt valid then we need to scale the
    //image after loading the whole image.
    if ( qMax( img.size().width(), img.size().height() ) > size )
    {
        QSize sz(img.size());
        sz.scale( size, size, Qt::KeepAspectRatio );
        img = img.scaled(sz, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    thumb = img;
    return !thumb.isNull();
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
