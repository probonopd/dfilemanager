#include "helpers.h"
#include "mainwindow.h"

MimeProvider::MimeProvider()
{
#if defined(HASMAGIC)
    m_mime = magic_open(MAGIC_MIME_TYPE);
    magic_load(m_mime, NULL);
    m_all = magic_open(MAGIC_CONTINUE);
    magic_load(m_all, NULL);
#endif
}

MimeProvider::~MimeProvider()
{
#if defined(HASMAGIC)
    magic_close(m_mime);
    magic_close(m_all);
#endif
}

QString
MimeProvider::getMimeType(const QString &file) const
{
    QMutexLocker locker(&m_mutex);
#if defined(HASMAGIC)
    return magic_file(m_mime, file.toLocal8Bit().data());
#elif defined(ISWINDOWS)
    const QString &suffix = file.mid(file.lastIndexOf("."));
    if (!suffix.startsWith("."))
        return QString("Unknown");
    const QString &regCommand = QString("HKCR\\%1").arg(suffix);
    const QVariant &var = QSettings(regCommand, QSettings::NativeFormat).value("Content Type");
    if (var.isValid())
        return var.toString();
    else
        return QString("Unknown");
#endif
    return QString("Unknown");
}

QString
MimeProvider::getFileType(const QString &file) const
{
    QMutexLocker locker(&m_mutex);
#if defined(HASMAGIC)
    return magic_file(m_all, file.toLocal8Bit().data());
#elif defined(ISWINDOWS)
    const QString &suffix = file.mid(file.lastIndexOf("."));
    if (!suffix.startsWith("."))
        return QString("Unknown");
    const QString &regCommand = QString("HKCR\\%1").arg(suffix);
    const QVariant &var = QSettings(regCommand, QSettings::NativeFormat).value(".");
    if (var.isValid())
    {
        QSettings s("HKCR", QSettings::NativeFormat);
        s.beginGroup(var.toString());
        const QString &type = s.value(".").toString();
        if (!type.isEmpty())
            return type;
        else
            return var.toString();
    }
    else
        return QString("Unknown");
#endif
    return QString("Unknown");
}

//-----------------------------------------------------------------------------

void
DFM::ViewBase::keyPressEvent(QKeyEvent *ke)
{
    SearchBox *searchBox = DFM::MainWindow::currentWindow()->searchBox();
    if (!ke->text().isEmpty() && !ke->modifiers() && ke->key() != Qt::Key_Escape)
    {
        searchBox->setMode(Filter);
        searchBox->setText(QString("%1%2").arg(searchBox->text(), ke->text()));
        searchBox->setFocus();
    }
}
