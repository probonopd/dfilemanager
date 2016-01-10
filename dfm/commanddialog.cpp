#include "commanddialog.h"
#include "helpers.h"
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDir>
#include <QSettings>
#include <QDebug>
#include <QCompleter>

static QMap<QString, QStringList> cmds;

static const QString confPath()
{
    static const QString cp = QString("%1/.config/dfm").arg(QDir::homePath());
    return cp;
}

static const char *separator(".");

bool CommandDialog::s_init(false);

CommandDialog::CommandDialog(QWidget *parent, const QString &file)
    : QDialog(parent)
    , m_file(file)
    , m_cmd()
    , m_mime()
    , m_box(new QComboBox(this))
    , m_ok(new QPushButton("Ok", this))
    , m_cancel(new QPushButton("Cancel", this))
{
    setMinimumWidth(256);
    if (!s_init)
    {
        QSettings settings(QString("%1/commandhistory.conf").arg(confPath()), QSettings::IniFormat);
        for (int i = 0; i < settings.childKeys().count(); ++i)
        {
            QString mime = settings.childKeys().at(i);
            const QStringList commands = settings.value(mime, QStringList()).toStringList();
            mime.replace(separator, "/");
            cmds.insert(mime, commands);
        }
        s_init = true;
    }

    connect(m_ok, SIGNAL(clicked(bool)), this, SLOT(accept()));
    connect(m_cancel, SIGNAL(clicked(bool)), this, SLOT(reject()));
    m_mime = DMimeProvider().getMimeType(file);
    QVBoxLayout *l = new QVBoxLayout();
    l->addWidget(m_box);
    QHBoxLayout *btns = new QHBoxLayout();
    btns->addStretch();
    btns->addWidget(m_ok);
    btns->addWidget(m_cancel);
    btns->addStretch();
    l->addLayout(btns);
    setLayout(l);

    m_box->setEditable(true);
    QStringList complete;
    const QStringList paths = QString(getenv("PATH")).split(":", QString::SkipEmptyParts);
    for (int i = 0; i < paths.count(); ++i)
        complete << QDir(paths.at(i)).entryList(QDir::Executable|QDir::Files);
    m_box->setCompleter(new QCompleter(complete, m_box));
    if (cmds.contains(m_mime))
        m_box->addItems(cmds.value(m_mime));
}

void
CommandDialog::accept()
{
    m_cmd = m_box->currentText();
    if (!m_cmd.isEmpty())
    {
        QStringList history;
        if (cmds.contains(m_mime))
            history << cmds.value(m_mime);
        if (!history.contains(m_cmd))
            history << m_cmd;
        cmds.insert(m_mime, history);

        QSettings settings(QString("%1/commandhistory.conf").arg(confPath()), QSettings::IniFormat);
        QString mime = m_mime;
        mime.replace("/", separator);
        settings.setValue(mime, history);
    }
    QDialog::accept();
}

QString
CommandDialog::command()
{
    return m_cmd;
}

QString
CommandDialog::getCommand(QWidget *parent, const QString &file, bool *ok)
{
    CommandDialog d(parent, file);
    d.setWindowTitle("Custom Command...");

    int ret = d.exec();
    if (ok)
        *ok = !!ret;
    if (ret) {
        return d.command();
    } else {
        return QString();
    }
}
