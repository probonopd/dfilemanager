#include "helpers.h"

MimeProvider::MimeProvider()
{
#if defined(Q_OS_UNIX)
    m_mime = magic_open(MAGIC_MIME_TYPE);
    magic_load(m_mime, NULL);
    m_all = magic_open(MAGIC_CONTINUE);
    magic_load(m_all, NULL);
#endif
}

MimeProvider::~MimeProvider()
{
#if defined(Q_OS_UNIX)
    magic_close(m_mime);
    magic_close(m_all);
#endif
}

QString
MimeProvider::getMimeType(const QString &file) const
{
    QMutexLocker locker(&m_mutex);
    if (file.isEmpty())
        return QString();
#if defined(Q_OS_UNIX)
    return magic_file(m_mime, file.toStdString().c_str());
#else
    return QString();
#endif
}

QString
MimeProvider::getFileType(const QString &file) const
{
    QMutexLocker locker(&m_mutex);
    if (file.isEmpty())
        return QString();
#ifdef Q_OS_UNIX
    return magic_file(m_all, file.toStdString().c_str());
#else
    return QString();
#endif
}
