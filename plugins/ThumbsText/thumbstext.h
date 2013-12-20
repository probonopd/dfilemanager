#ifndef THUMBSTEXT_H
#define THUMBSTEXT_H

#include <../../dfm/interfaces.h>
#include <QStringList>
#include <QImage>

class ThumbsText : public QObject, ThumbInterface
{
    Q_OBJECT
    Q_INTERFACES(ThumbInterface)

public:
    void init();
    ~ThumbsText(){}

    bool canRead(const QString &file) const;
    bool thumb(const QString &file, const int size, QImage &thumb);

    QString name() const { return tr("TextThumbs"); }
    QString description() const { return tr("plugin for generating thumbs for text files"); }
};


#endif // THUMBSTEXT_H
