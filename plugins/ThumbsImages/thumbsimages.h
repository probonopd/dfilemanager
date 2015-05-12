#ifndef THUMBSIMAGES_H
#define THUMBSIMAGES_H

#include <../../dfm/interfaces.h>
#include <QObject>

class ThumbsImages : public QObject, ThumbInterface
{
    Q_OBJECT
#if QT_VERSION >= 0x050000
    Q_PLUGIN_METADATA(IID "dfm.ThumbInterface")
#endif
    Q_INTERFACES(ThumbInterface)

public:
    ~ThumbsImages();
    void init();
    QString name() const;
    QString description() const;
    bool thumb(const QString &file, const QString &mime, QImage &thumb, const int size);
};


#endif // THUMBSIMAGES_H
