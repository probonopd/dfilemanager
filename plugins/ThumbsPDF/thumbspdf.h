#ifndef THUMBSPDF_H
#define THUMBSPDF_H

#include <../../dfm/interfaces.h>

namespace Poppler{class Document;}

class ThumbsPDF : public QObject, ThumbInterface
{
    Q_OBJECT
#if QT_VERSION >= 0x050000
    Q_PLUGIN_METADATA(IID "dfm.ThumbInterface")
#endif
    Q_INTERFACES(ThumbInterface)

public:
    void init();
    ~ThumbsPDF(){}

    QString name() const { return tr("PDFThumbs"); }
    QString description() const { return tr("Thumbs generator for pdf files using poppler"); }

    bool canRead(const QString &mime) const { return mime.endsWith("pdf"); }
    bool thumb(const QString &file, const QString &mime, QImage &thumb, const int size);

private:
    Poppler::Document *document;
};

#endif // THUMBSPDF_H
