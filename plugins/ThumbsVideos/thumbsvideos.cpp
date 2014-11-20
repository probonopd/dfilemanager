#include "thumbsvideos.h"
#include <QDebug>

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
    if (!canRead(file))
        return false;
    try
    {
        std::vector<uint8_t> pixels;
        m_vt.generateThumbnail(file.toStdString(), Png, pixels);
        if (thumb.loadFromData(pixels.data(), pixels.size(), "PNG"))
            return true;
    }
    catch (std::exception &e)
    {
        qDebug() << e.what();
        return false;
    }

    return false;
}

#define END(_SUFFIX_) file.endsWith(_SUFFIX_, Qt::CaseInsensitive)

bool
ThumbsVideos::canRead(const QString &file) const
{
    return END("mkv")||END("avi")||END("mp4")||END("mpg");
}

#undef END
