#ifndef HELPERS_H
#define HELPERS_H

#include <QString>
#include <QMutex>
#include <QKeyEvent>
#include <QHash>
#include <QQueue>

#if defined(HASMAGIC)
#include <magic.h>
#endif

class DMimeProvider
{
public:
    DMimeProvider();
    ~DMimeProvider();
    QString getMimeType(const QString &file) const;
    QString getFileType(const QString &file) const;

private:
#if defined(HASMAGIC)
    magic_t m_mime, m_all;
#endif
    mutable QMutex m_mutex;
};

namespace DFM
{

class DTrash
{
public:
    enum TrashPath { TrashFiles = 0, TrashInfo, TrashRoot, TrashNPaths };
    static bool moveToTrash(const QStringList &files);
    static bool restoreFromTrash(const QStringList &files);
    static bool removeFromTrash(const QStringList &files);
    static QString trashPath(const TrashPath &location = TrashRoot);

protected:
    static QString restorePath(QString file);
    static QString trashInfoFile(QString file);
    static QString trashFilePath(QString file);
};

class DViewBase
{
public:
    DViewBase(){}
    ~DViewBase(){}

protected:
    virtual void keyPressEvent(QKeyEvent *ke);
};

//convenience wrapper for thread-safe hashing...
template <typename Key, typename Val>
class DHash
{
public:
    void insert(Key key, Val val)           { QMutexLocker locker(&m_mutex); m_hash.insert(key, val); }
    Val value(Key key, Val defaultValue)    { QMutexLocker locker(&m_mutex); return m_hash.value(key, defaultValue); }
    int count()                             { QMutexLocker locker(&m_mutex); return m_hash.count(); }
    bool contains(Key key)                  { QMutexLocker locker(&m_mutex); return m_hash.contains(key); }
    void destroy(Key key)                   { QMutexLocker locker(&m_mutex); if (m_hash.contains(key)) delete m_hash.take(key); }

private:
    mutable QMutex m_mutex;
    QHash<Key, Val> m_hash;
};

template <typename T>
class DQueue
{
public:
    bool enqueue(T t)                       { QMutexLocker locker(&m_mutex); if (m_queue.contains(t)) return false; m_queue.enqueue(t); return true; }
    T dequeue()                             { QMutexLocker locker(&m_mutex); return m_queue.dequeue(); }
    bool contains(T t)                      { QMutexLocker locker(&m_mutex); return m_queue.contains(t); }
    void clear()                            { QMutexLocker locker(&m_mutex); m_queue.clear(); }
    bool isEmpty()                          { QMutexLocker locker(&m_mutex); return m_queue.isEmpty(); }

private:
    mutable QMutex m_mutex;
    QQueue<T> m_queue;
};

template <typename T>
class DList
{
public:
    typedef typename QList<T>::const_iterator const_iterator;

    void lock()                             { m_mutex.lock(); }
    void unlock()                           { m_mutex.unlock(); }
    void append(T t)                        { QMutexLocker locker(&m_mutex); m_list.append(t); }
    void clear()                            { QMutexLocker locker(&m_mutex); m_list.clear(); }
    void insert(const int i, T t)           { QMutexLocker locker(&m_mutex); m_list.insert(i, t); }

    bool contains(T t)                      { QMutexLocker locker(&m_mutex); return m_list.contains(t); }
    bool isEmpty()                          { QMutexLocker locker(&m_mutex); return m_list.isEmpty(); }
    bool removeOne(T t)                     { QMutexLocker locker(&m_mutex); return m_list.removeOne(t); }
    int size()                              { QMutexLocker locker(&m_mutex); return m_list.size(); }
    int indexOf(T t)                        { QMutexLocker locker(&m_mutex); return m_list.indexOf(t); }
    T at(const int i)                       { QMutexLocker locker(&m_mutex); if (-1 < i && i < m_list.size()) return m_list.at(i); return 0; }

    void operator <<(T t)                   { QMutexLocker locker(&m_mutex); m_list << t; }

    //NEED to call lock() before touching these.
    const_iterator begin()                  { return m_list.begin(); }
    const_iterator end()                    { return m_list.end(); }

private:
    mutable QMutex m_mutex;
    QList<T> m_list;
};


}

#endif // HELPERS_H
