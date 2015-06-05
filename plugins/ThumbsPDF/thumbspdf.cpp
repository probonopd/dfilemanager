#include "thumbspdf.h"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QImage>

#if defined(POPPLER_VERSION)
#if POPPLER_VERSION < 5
#include <poppler-qt4.h>
#else
#include <poppler-qt5.h>
#endif
#endif

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
#if !defined(POPPLER_VERSION)
    Q_UNUSED(file)
    Q_UNUSED(mime)
    Q_UNUSED(thumb)
    Q_UNUSED(size)
    return false;
#else
    if (QFileInfo(file).size() > maxSize || !canRead(mime))
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
#endif
}
