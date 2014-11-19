#ifndef THUMBSPDF_H
#define THUMBSPDF_H

#include <../../dfm/interfaces.h>
#include <QStringList>
#include <QImage>
#include <QFileInfo>
//#include <Magick++/Image.h>
#include <poppler-qt4.h>

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

    bool canRead(const QString &file) const { return QFileInfo(file).suffix() == "pdf"; }
    bool thumb(const QString &file, const int size, QImage &thumb);

private:
    Poppler::Document* document;
};


#endif // THUMBSPDF_H
