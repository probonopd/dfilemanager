#ifndef THUMBSIMAGES_H
#define THUMBSIMAGES_H

#include <../../dfm/interfaces.h>
#include <QStringList>
#include <QImage>

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
    QStringList suffixes() const;
    QString name() const;
    QString description() const;
    bool canRead(const QString &file) const;
    bool thumb(const QString &file, const int size, QImage &thumb);
};


#endif // THUMBSIMAGES_H
