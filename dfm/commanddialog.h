#ifndef COMMANDDIALOG_H
#define COMMANDDIALOG_H

#include <QDialog>

class QComboBox;
class CommandDialog : public QDialog
{
    Q_OBJECT
public:
    CommandDialog(QWidget *parent, const QString &file);
    static QString getCommand(QWidget *parent, const QString &file, bool *ok = 0);

public slots:
    void accept();

protected:
    QString command();

private:
    static bool s_init;
    QString m_cmd, m_file, m_mime;
    QComboBox *m_box;
    QPushButton *m_ok, *m_cancel;
};

#endif // COMMANDDIALOG_H
