#include "thumbsmovies.h"
#include <QNetworkConfigurationManager>
#include <QFileInfo>
#include <QApplication>

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(thumbsmovies, ThumbsMovies)
#endif

ThumbsMovies::ThumbsMovies():m_mc(0),m_mgr(0)
{
    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(deleteLater()));
}

ThumbsMovies::~ThumbsMovies()
{
    m_loop->exit();
    m_loop->deleteLater();
    m_mgr->deleteLater();
    m_mc->deleteLater();
}

void
ThumbsMovies::init()
{
    m_mc = new MovieClient();
    m_mgr = new QNetworkConfigurationManager();
    m_loop = new QEventLoop();
    connect(m_mc, SIGNAL(slotPosterFinished(QImage)), this, SLOT(image(QImage))/*, Qt::DirectConnection*/);
}

bool
ThumbsMovies::thumb(const QString &file, const QString &mime, QImage &thumb, const int size)
{
    if (!m_mgr->isOnline())
    {
        qDebug() << "No network connection available";
        return false;
    }
    if (!canRead(file))
        return false;
    m_mc->setSize(size);
    m_image = QImage();
    m_mc->addSearch(file);
//    m_loop->exec();
//    qDebug() << file;
    if (!m_image.isNull())
    {
        thumb = m_image;
        if (m_image.size().width()>size||m_image.size().height()>size)
            thumb = m_image.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        return true;
    }
    return false;
}

void
ThumbsMovies::image(const QImage &img)
{
    m_image = img;
//    m_loop->exit();
}

#define END(_SUFFIX_) file.endsWith(_SUFFIX_, Qt::CaseInsensitive)

bool
ThumbsMovies::canRead(const QString &file) const
{
    return END("mkv")||END("avi")||END("mp4")||END("mpg")||QFileInfo(file).isDir();
}

#undef END
