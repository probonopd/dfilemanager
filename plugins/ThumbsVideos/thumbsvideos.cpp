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
    m_vt.generateThumbnail(file.toUtf8().constData(), Png, pixels);
    if (thumb.loadFromData(pixels.data(), pixels.size(), "PNG"))
        return true;
    return false;
}


bool
ThumbsVideos::canRead(const QString &mime) const
{
    return mime.startsWith("video");
}
