#ifndef THUMBSIMAGES_H
#define THUMBSIMAGES_H

#include <../../interfaces.h>
#include <QStringList>
#include <QImage>

class ThumbsImages : public QObject, ThumbInterface
{
    Q_OBJECT
    Q_INTERFACES(ThumbInterface)

public:
    QStringList suffixes() const;
    QImage thumb(const QString &file, const int size = 256) const;
};


#endif // THUMBSIMAGES_H
