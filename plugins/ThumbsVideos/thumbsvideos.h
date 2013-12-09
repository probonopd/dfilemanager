#ifndef THUMBSVIDEOS_H
#define THUMBSVIDEOS_H

#include <../../dfm/interfaces.h>
#include <QStringList>
#include <QImage>
#include <libffmpegthumbnailer/videothumbnailer.h>

class ThumbsVideos : public QObject, ThumbInterface
{
    Q_OBJECT
    Q_INTERFACES(ThumbInterface)

public:
    void init();
    ~ThumbsVideos(){}

    bool canRead(const QString &file) const;
    QImage thumb(const QString &file, const int size = 256);

    QString name() const { return tr("VideoThumbs"); }
    QString description() const { return tr("Plugin for loading thumbnails for videos using ffmpegthumbnailer"); }

private:
    ffmpegthumbnailer::VideoThumbnailer m_vt;
};


#endif // THUMBSVIDEOS_H
