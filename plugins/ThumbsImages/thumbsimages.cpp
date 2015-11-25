#include "thumbsimages.h"
#include <QImageReader>
#include <QFileInfo>
#include <QDebug>
#include <QStringList>
#include <QImage>

#if defined(HASEXIV)
#include <exiv2/exiv2.hpp>
#endif

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(thumbsimages, ThumbsImages)
#endif

void
ThumbsImages::init()
{
    //called once when plugin loaded
}

ThumbsImages::~ThumbsImages()
{
    //destructor
}

static short getRotation(const QString &file)
{
#if defined(HASEXIV)
    try
    {
        Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(file.toLatin1().data());
        if (image.get())
        {
            image->readMetadata();
            Exiv2::ExifData exifData = image->exifData();
            Exiv2::ExifKey key("Exif.Image.Orientation");
            Exiv2::ExifData::iterator keypos = exifData.findKey(key);
            short rotation(1);
            if (keypos != exifData.end())
            {
                Exiv2::Value::AutoPtr v = Exiv2::Value::create(Exiv2::unsignedShort);
                v = keypos->getValue();
                rotation = v->toLong();
                return rotation;
            }
        }
    }
    catch (Exiv2::AnyError& e)
    {
        qDebug() << "Caught Exiv2 exception" << e.what();
    }
#endif
    return 1;
}



bool
ThumbsImages::thumb(const QString &file, const QString &mime, QImage &thumb, const int size)
{
    Q_UNUSED(mime);
    static QImageReader ir;
    ir.setFileName(file);
    if (!ir.canRead())
        return false;

    //sometimes ir.size() doesnt return a valid size,
    //this is at least true for xcf images,
    //then we have to scale them later after we have
    //the full image. This is slower.
    QSize thumbsize(ir.size());
    if (thumbsize.isValid() && qMax(thumbsize.width(), thumbsize.height()) > size)
        thumbsize.scale(size, size, Qt::KeepAspectRatio);
    ir.setScaledSize(thumbsize);

    QImage img(ir.read());

    //if the thumbsize isnt valid then we need to scale the
    //image after loading the whole image.
    if (qMax(img.size().width(), img.size().height()) > size)
        img = img.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
#if defined(HASEXIV)
    const short rotation(getRotation(file));
    if (!img.isNull() && rotation != 1)
    {
        QTransform t;
        int wt(img.width()/2), ht(img.height()/2);
        t.translate(wt, ht);
        switch (rotation)
        {
        case 8: t.rotate(-90); break;
        case 3: t.rotate(180); break;
        case 6: t.rotate(90); break;
        default: break;
        }
        t.translate(-wt, -ht);
        img = img.transformed(t);
    }
#endif
    thumb = img;
    return !thumb.isNull();
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
