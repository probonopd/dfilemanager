#include "thumbstext.h"
#include <QFile>
#include <QFont>
#include <QPainter>
#include <QTextStream>
#include <QTextCodec>
#include <QFileInfo>

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(thumbstext, ThumbsText)
#endif

bool
ThumbsText::thumb(const QString &file, const QString &mime, QImage &thumb, const int size)
{
    if (!mime.startsWith("text", Qt::CaseInsensitive))
        return false;
    
    QFile f(file);
    if (!f.open(QFile::ReadOnly|QIODevice::Text))
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
    while (!data.atEnd() && pos < size)
    {
        p.drawText(0, pos, data.readLine());
        pos += h;
    }
    p.end();
    f.close();
    thumb = img;
    return !thumb.isNull();
}
