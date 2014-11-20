#include "thumbspdf.h"
#include <QDebug>
#include <poppler-qt4.h>
#include <QDir>
#include <QFileInfo>

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(thumbspdf, ThumbsPDF)
#endif

static quint64 maxSize = 10485760;

void
ThumbsPDF::init()
{
}

bool
ThumbsPDF::thumb(const QString &file, const QString &mime, QImage &thumb, const int size)
{   
    if (QFileInfo(file).size() > maxSize || !canRead(file))
        return false;
    
    document = Poppler::Document::load(file.toLocal8Bit().data());
    if (document == 0) {
        return false;
    }
    
    Poppler::Page *pdfPage = document->page(0);
    if (pdfPage == 0) {
        delete document;
        return false;
    }
    
    thumb = pdfPage->renderToImage();
    if (thumb.isNull()) {
        delete document;
        return false;
    }
    
    delete document;
    return true;
}
