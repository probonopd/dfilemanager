#ifndef THUMBSPDF_H
#define THUMBSPDF_H

#include <../../interfaces.h>
#include <QStringList>
#include <QImage>
#include <QFileInfo>
#include <Magick++/Image.h>

class ThumbsPDF : public QObject, ThumbInterface
{
    Q_OBJECT
    Q_INTERFACES(ThumbInterface)

public:
    void init();
    ~ThumbsPDF(){}

    QString name() const { return tr("PDFThumbs"); }
    QString description() const { return tr("Thumbs generator for pdf files using graphicsmagick"); }

    bool canRead(const QString &file) const { return QFileInfo(file).suffix() == "pdf"; }
    QImage thumb(const QString &file, const int size = 256);

private:
    Magick::Image m_image;
};


#endif // THUMBSPDF_H
