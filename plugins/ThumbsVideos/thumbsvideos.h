#ifndef THUMBSVIDEOS_H
#define THUMBSVIDEOS_H

#include <../../dfm/interfaces.h>
#include <QStringList>
#include <QImage>
#include <libffmpegthumbnailer/videothumbnailer.h>

class ThumbsVideos : public QObject, ThumbInterface
{
    Q_OBJECT
#if QT_VERSION >= 0x050000
    Q_PLUGIN_METADATA(IID "dfm.ThumbInterface")
#endif
    Q_INTERFACES(ThumbInterface)

public:
    void init();
    ~ThumbsVideos(){}

    bool canRead(const QString &file) const;
    bool thumb(const QString &file, const int size, QImage &thumb);

    QString name() const { return tr("VideoThumbs"); }
    QString description() const { return tr("Plugin for loading thumbnails for videos using ffmpegthumbnailer"); }

private:
    ffmpegthumbnailer::VideoThumbnailer m_vt;
};


#endif // THUMBSVIDEOS_H
