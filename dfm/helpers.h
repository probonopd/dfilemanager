#ifndef HELPERS_H
#define HELPERS_H

#include <QString>
#include <QMutex>
#include <QKeyEvent>

#if defined(Q_OS_UNIX)
#include <magic.h>
#endif

class MimeProvider
{
public:
    MimeProvider();
    ~MimeProvider();
    QString getMimeType(const QString &file) const;
    QString getFileType(const QString &file) const;

private:
#if defined(Q_OS_UNIX)
    magic_t m_mime, m_all;
#endif
    mutable QMutex m_mutex;
};

namespace DFM
{
class ViewBase
{
public:
    ViewBase(){}
    ~ViewBase(){}

protected:
    virtual void keyPressEvent(QKeyEvent *ke);
};
}

#endif // HELPERS_H
