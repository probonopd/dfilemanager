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
    if (!mime.startsWith("video"))
        return false;
    std::vector<uint8_t> pixels;
    try
    {
        m_vt.generateThumbnail(qPrintable(file), Png, pixels);
    }
    catch (const std::exception &ex)
    {
        qDebug() << "catched" << ex.what();
        return false;
    }
    return thumb.loadFromData(pixels.data(), pixels.size());
}
