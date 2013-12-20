#ifndef THUMBSVIDEOS_H
#define THUMBSVIDEOS_H

#include <../../dfm/interfaces.h>
#include <QStringList>
#include <QImage>
#include <movieclient.h>
#include <QEventLoop>

class ThumbsMovies : public QObject, ThumbInterface
{
    Q_OBJECT
    Q_INTERFACES(ThumbInterface)

public:
    void init();
    ~ThumbsMovies(){delete m_mc; delete m_evLoop;}

    bool canRead(const QString &file) const;
    bool thumb(const QString &file, const int size, QImage &thumb);

    QString name() const { return tr("MovieThumbs"); }
    QString description() const { return tr("Plugin for loading thumbnails for videos using MovieThumbs"); }

private slots:
    void image(const QImage &img);

private:
    MovieClient *m_mc;
    QImage m_image;
    QEventLoop *m_evLoop;
    QTimer *m_loopTimer;
};


#endif // THUMBSVIDEOS_H
