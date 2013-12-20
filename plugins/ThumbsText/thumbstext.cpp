#include "thumbstext.h"
#include <QFile>
#include <QFont>
#include <QPainter>
#include <QTextStream>
#include <QTextCodec>
#include <QFileInfo>
#include <magic.h>

Q_EXPORT_PLUGIN2(thumbstext, ThumbsText)

static magic_t s_magicMime;
static QString getMimeType(const QString &file)
{
    const QFileInfo f(file);
    if ( !f.isReadable() )
        return QString();

    if ( f.isSymLink())
        return "inode/symlink";
    if ( f.isDir() )
        return "inode/directory";
    return QString( magic_file( s_magicMime, file.toLocal8Bit().data() ) );;
}

void
ThumbsText::init()
{
    s_magicMime = magic_open( MAGIC_MIME_TYPE );
    magic_load( s_magicMime, NULL );
}

bool
ThumbsText::canRead(const QString &file) const
{
    return getMimeType(file).startsWith("text", Qt::CaseInsensitive);
}

bool
ThumbsText::thumb(const QString &file, const int size, QImage &thumb)
{
    QFile f(file);
    if ( !f.open(QFile::ReadOnly|QIODevice::Text) || !canRead(file) )
        return false;

    QImage img(QSize(size, size), QImage::Format_RGB32);
    img.fill(Qt::white);
    QPainter p(&img);
    QFont font("Monospace", size/32, QFont::Black);
    p.setFont(font);
    int h = QFontMetrics(font).height();
    int pos = h;
    QTextStream data(&f);
    data.setCodec(QTextCodec::codecForLocale());
    while ( !data.atEnd() && pos < size )
    {
        p.drawText(0, pos, data.readLine());
        pos += h;
    }
    p.end();
    f.close();
    thumb = img;
    return !thumb.isNull();
}
