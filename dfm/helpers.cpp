#include "helpers.h"
#include "mainwindow.h"

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
#if defined(Q_OS_UNIX)
    return magic_file(m_mime, file.toLocal8Bit().data());
#else
    return QString();
#endif
}

QString
MimeProvider::getFileType(const QString &file) const
{
    QMutexLocker locker(&m_mutex);
#ifdef Q_OS_UNIX
    return magic_file(m_all, file.toLocal8Bit().data());
#else
    return QString();
#endif
}

//-----------------------------------------------------------------------------

void
DFM::ViewBase::keyPressEvent(QKeyEvent *ke)
{
    SearchBox *searchBox = DFM::MainWindow::currentWindow()->searchBox();
    if (!ke->text().isEmpty() && !ke->modifiers())
    {
        searchBox->setText(QString("%1%2").arg(searchBox->text(), ke->text()));
        searchBox->setFocus();
    }
}
