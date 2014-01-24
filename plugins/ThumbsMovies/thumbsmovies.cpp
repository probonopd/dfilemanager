#include "thumbsmovies.h"
#include <QFileInfo>
#include <QApplication>

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(thumbsmovies, ThumbsMovies)
#endif

ThumbsMovies::ThumbsMovies():m_mc(0),m_loopTimer(0),m_evLoop(0)
{
    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(deleteLater()));
}

void
ThumbsMovies::init()
{
    m_mc = new MovieClient();
    m_evLoop = new QEventLoop();
    m_loopTimer = new QTimer();
    m_loopTimer->setSingleShot(true);
    connect(m_mc, SIGNAL(slotPosterFinished(QImage)), this, SLOT(image(QImage)));
    connect(m_loopTimer, SIGNAL(timeout()), m_evLoop, SLOT(quit()));
}

bool
ThumbsMovies::thumb(const QString &file, const int size, QImage &thumb)
{
    if (!canRead(file))
        return false;
    m_image = QImage();
    m_mc->addSearch(file);
    m_loopTimer->start(50);
    m_evLoop->exec();
    if (!m_image.isNull())
    {
        thumb = m_image.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        return true;
    }
    return false;
}

void
ThumbsMovies::image(const QImage &img)
{
    m_image = img;
    m_loopTimer->stop();
    m_evLoop->quit();
}

#define END(_SUFFIX_) file.endsWith(_SUFFIX_, Qt::CaseInsensitive)

bool
ThumbsMovies::canRead(const QString &file) const
{
    return END("mkv")||END("avi")||END("mp4")||END("mpg")||QFileInfo(file).isDir();
}

#undef END
