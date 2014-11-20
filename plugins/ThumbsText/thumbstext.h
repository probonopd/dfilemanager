#ifndef THUMBSTEXT_H
#define THUMBSTEXT_H

#include <../../dfm/interfaces.h>
#include <QStringList>
#include <QImage>

class ThumbsText : public QObject, ThumbInterface
{
    Q_OBJECT
#if QT_VERSION >= 0x050000
    Q_PLUGIN_METADATA(IID "dfm.ThumbInterface")
#endif
    Q_INTERFACES(ThumbInterface)

public:
    void init(){}
    ~ThumbsText(){}
    bool thumb(const QString &file, const QString &mime, QImage &thumb, const int size);

    QString name() const { return tr("TextThumbs"); }
    QString description() const { return tr("plugin for generating thumbs for text files"); }
};


#endif // THUMBSTEXT_H
