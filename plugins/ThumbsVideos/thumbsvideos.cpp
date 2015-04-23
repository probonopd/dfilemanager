#include "thumbsvideos.h"
#include <QDebug>
#include <QString>

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(thumbsvideos, ThumbsVideos)
#endif

void
ThumbsVideos::init()
{
    m_vt.setThumbnailSize(256);
}

bool
ThumbsVideos::thumb(const QString &file, const QString &mime, QImage &thumb, const int size)
{
    if (!canRead(mime))
        return false;
    std::vector<uint8_t> pixels;
    try
    {
        m_vt.generateThumbnail(file.toUtf8().constData(), Png, pixels);
    }
    catch (const std::exception& ex)
    {
        qDebug() << "catched" << ex.what();
        return false;
    }
    return thumb.loadFromData(pixels.data(), pixels.size());
}


bool
ThumbsVideos::canRead(const QString &mime) const
{
    return mime.startsWith("video");
}
