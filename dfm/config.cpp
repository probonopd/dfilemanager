/**************************************************************************
*   Copyright (C) 2013 by Robert Metsaranta                               *
*   therealestrob@gmail.com                                               *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/


#include "config.h"
#include "mainwindow.h"
#include "application.h"
#include "helpers.h"
#include "operations.h"
#include <QDir>
#include <QProcess>
#include <QTextStream>

using namespace DFM;

Store *Store::m_instance = 0;
Config Store::config;

Store::Store(QObject *parent) : QObject(parent)
{
#if defined(ISUNIX)
    m_settings = new QSettings("dfm", "dfm");
#else
    m_settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "dfm", "dfm");
#endif
    m_customActionsMenu.setTitle(tr("Custom Actions"));
}

Store::~Store()
{
    delete m_settings;
    m_instance = 0;
}

Store
*Store::instance()
{
    if (!m_instance)
        m_instance = new Store();
    return m_instance;
}

void
Store::readConfiguration()
{
    config.pluginPath = settings()->value("pluginPath", QString()).toString();
    config.docks.lock = settings()->value("docks.lock", 0).toInt();
    config.startPath = settings()->value("startPath", QDir::homePath()).toString();
    config.styleSheet = settings()->value("styleSheet", QString()).toString();

    config.behaviour.hideTabBarWhenOnlyOneTab = settings()->value("hideTabBarWhenOnlyOne", false).toBool();
    config.behaviour.systemIcons = settings()->value("useSystemIcons", false).toBool();
    config.behaviour.devUsage = settings()->value("drawDevUsage", false).toBool();
    config.behaviour.view = settings()->value("start.view", 0).toInt();
    config.behaviour.pathBarStyle = settings()->value("pathBarStyle", 1).toInt();
    config.behaviour.sideBarStyle = settings()->value("sideBarStyle", 1).toInt();
    config.behaviour.pathBarPlace = settings()->value("behaviour.pathBarPlace", 0).toInt();
    config.behaviour.useIOQueue = settings()->value("behaviour.useIOQueue", true).toBool();
    config.behaviour.showCloseTabButton = settings()->value("behaviour.showCloseTabButton", false).toBool();

    config.views.showThumbs = settings()->value("showThumbs", false).toBool();
    config.views.activeThumbIfaces = settings()->value("activeThumbIfaces", QStringList()).toStringList();
    config.views.singleClick = settings()->value("views.singleClick", false).toBool();
    config.views.flowSize = settings()->value("flowSize", QByteArray()).toByteArray();

    //IconView
    config.views.iconView.textWidth = settings()->value("textWidth", 16).toInt();
    config.views.iconView.iconSize = settings()->value("iconView.iconSize", 3).toInt();
    config.views.iconView.lineCount = settings()->value("iconView.lineCount", 3).toInt();
    config.views.iconView.categorized = settings()->value("iconView.categorized", false).toBool();
    config.views.iconView.categoryStyle = settings()->value("iconView.categoryStyle", 0).toInt();

    //DetailsView
    config.views.detailsView.rowPadding = settings()->value("detailsView.rowPadding", 0).toInt();
    config.views.detailsView.altRows = settings()->value("detailsView.altRows", false).toBool();

    //ColumnView
    config.views.columnsView.colWidth = settings()->value("columnsView.colWidth", 200).toInt();

    config.behaviour.gayWindow = settings()->value("behaviour.gayWindow", false).toBool();
    config.behaviour.tabShape = settings()->value("behaviour.gayWindow.tabShape", 0).toInt();
    config.behaviour.tabRoundness = settings()->value("behaviour.gayWindow.tabRoundness", 6).toInt();
    config.behaviour.tabHeight = settings()->value("behaviour.gayWindow.tabHeight", 22).toInt();
    config.behaviour.tabWidth = settings()->value("behaviour.gayWindow.tabWidth", 150).toInt();
    config.behaviour.frame = settings()->value("behaviour.gayWindow.frame", false).toBool();
    config.behaviour.newTabButton = settings()->value("behaviour.gayWindow.newTabButton", false).toBool();
    config.behaviour.tabOverlap = settings()->value("behaviour.gayWindow.tabOverlap", 8).toInt();
    config.behaviour.windowsStyle = settings()->value("behaviour.gayWindow.windowsStyle", false).toBool();
    config.behaviour.invertedColors = settings()->value("behaviour.gayWindow.invertedColors", false).toBool();

    config.behaviour.minFontSize = settings()->value("behaviour.minFontSize", 8).toInt();

    config.behaviour.sortingCol = settings()->value("behaviour.sortingCol", 0).toInt();
    config.behaviour.sortingOrd = (Qt::SortOrder)settings()->value("behaviour.sortingOrd", 0).toInt();

    config.behaviour.capsContainers = settings()->value("behaviour.capsContainers", false).toBool();
    config.views.dirSettings = settings()->value("views.dirSettings", false).toBool();

    config.behaviour.invActBookmark = settings()->value("behaviour.invActBookmark", false).toBool();
    config.behaviour.invAllBookmarks = settings()->value("behaviour.invAllBookmarks", false).toBool();

    settings()->beginGroup("CustomActions");
    foreach (const QString &string, settings()->childKeys())
    {
        QStringList actions = settings()->value(string).toStringList(); //0 == name, 1 == cmd, 2 == keysequence
        QAction *action = new QAction(actions[0], this);
        connect (action, SIGNAL(triggered()), Ops::instance(), SLOT(customActionTriggered()));
        action->setData(actions[1]);
        if (actions.count() > 2)
            action->setShortcut(QKeySequence(actions[2]));
        m_customActions << action;
        m_customActionsMenu.addAction(action);
    }
    settings()->endGroup();

//    dApp->loadPlugins();

//    Configuration::settings()->beginGroup("Scripts");
//    foreach (const QString &string, Configuration::settings()->childKeys())
//    {
//        QStringList actions = Configuration::settings()->value(string).toStringList(); //0 == name, 1 == script, 2 == keysequence
//        QAction *action = new QAction(actions[0], this);
//        action->setData(actions[1]);
//        if (actions.count() > 2)
//            action->setShortcut(QKeySequence(actions[2]));
//        connect (action, SIGNAL(triggered()), this, SLOT(scriptTriggered()));
//        VIEWS(addAction(action));
//    }
//    Configuration::settings()->endGroup();
}

void
Store::writeConfiguration()
{
    settings()->setValue("startPath", config.startPath);
    settings()->setValue("docks.lock", config.docks.lock);
    settings()->setValue("showThumbs", config.views.showThumbs);
    settings()->setValue("drawDevUsage", config.behaviour.devUsage);
    settings()->setValue("textWidth", config.views.iconView.textWidth);

    settings()->setValue("behaviour.pathBarPlace", config.behaviour.pathBarPlace);
    settings()->setValue("behaviour.useIOQueue", config.behaviour.useIOQueue);
    settings()->setValue("behaviour.showCloseTabButton", config.behaviour.showCloseTabButton);

    settings()->setValue("detailsView.rowPadding", config.views.detailsView.rowPadding);
    settings()->setValue("detailsView.altRows", config.views.detailsView.altRows);

    settings()->setValue("start.view", config.behaviour.view);
    settings()->setValue("iconView.iconSize", config.views.iconView.iconSize);
    settings()->setValue("iconView.lineCount", config.views.iconView.lineCount);
    settings()->setValue("iconView.categorized", config.views.iconView.categorized);
    settings()->setValue("flowSize", config.views.flowSize);
    settings()->setValue("behaviour.capsContainers", config.behaviour.capsContainers);

    settings()->setValue("behaviour.sortingCol", qBound(0, config.behaviour.sortingCol, 4));
    settings()->setValue("behaviour.sortingOrd", config.behaviour.sortingOrd);

    settings()->setValue("views.dirSettings", config.views.dirSettings);

    if (!config.behaviour.gayWindow)
        settings()->setValue("hideTabBarWhenOnlyOne", config.behaviour.hideTabBarWhenOnlyOneTab);
    settings()->setValue("styleSheet", config.styleSheet);
    settings()->setValue("views.singleClick", config.views.singleClick);

    settings()->setValue("useSystemIcons", config.behaviour.systemIcons);
    settings()->setValue("behaviour.invActBookmark", config.behaviour.invActBookmark);
    settings()->setValue("behaviour.invAllBookmarks", config.behaviour.invAllBookmarks);

    settings()->setValue("pluginPath", config.pluginPath);

    settings()->setValue("activeThumbIfaces", config.views.activeThumbIfaces);

    settings()->setValue("columnsView.colWidth", config.views.columnsView.colWidth);

//    settings()->setValue("behaviour.gayWindow", config.behaviour.gayWindow);
//    settings()->setValue("behaviour.gayWindow.tabShape", config.behaviour.tabShape);
//    settings()->setValue("behaviour.gayWindow.tabRoundness", config.behaviour.tabRoundness);
//    settings()->setValue("behaviour.gayWindow.tabRoundness", config.behaviour.tabHeight);
//    settings()->setValue("behaviour.gayWindow.tabWidth", config.behaviour.tabWidth);
//    settings()->setValue("behaviour.gayWindow.frame", config.behaviour.frame);
//    settings()->setValue("behaviour.gayWindow.newTabButton", config.behaviour.newTabButton);
//    settings()->setValue("behaviour.gayWindow.tabOverlap", config.behaviour.tabOverlap);
//    settings()->setValue("behaviour.gayWindow.windowsStyle", config.behaviour.windowsStyle);
//    settings()->setValue("behaviour.gayWindow.invertedColors", config.behaviour.invertedColors);
}

static inline QString desktopInfo(const QString &desktop, bool name)
{
    QString appsPath("/usr/share/applications/");
    QFileInfo desktopFileInfo(appsPath + desktop);
    if (!desktopFileInfo.exists())
    {
        QString addToPath = desktop.split("-").at(0);
        QString newDesktop = desktop.split("-").at(1);
        QStringList tmp = QDir(appsPath).entryList(QStringList() << addToPath, QDir::Dirs);
        if(tmp.count())
            desktopFileInfo = QFileInfo(appsPath + tmp[0] + "/" + newDesktop);
        if(!desktopFileInfo.exists())
            return QString();
    }
    QFile desktopFile(desktopFileInfo.filePath());
    desktopFile.open(QFile::ReadOnly); //we should only get here if file exists...
    QTextStream stream(&desktopFile);
    QString n, e;
    while (!stream.atEnd())
    {
        QString line = stream.readLine();
        if (line.startsWith("name=", Qt::CaseInsensitive))
        {
            if (line.split("=").count() >= 2)
                n = line.split("=").at(1);
        }
        else if (line.startsWith("exec=", Qt::CaseInsensitive))
        {
            if (line.split("=").count() >= 2)
                e = line.split("=").at(1);
        }
        else
            continue;
    }
    desktopFile.close();
    return name ? n : e;
}

#if defined(ISWINDOWS)

static QMap<QString, QString> apps()
{
    const QString &regKey = QString("HKEY_LOCAL_MACHINE\\SOFTWARE\\RegisteredApplications");
    QSettings settings(regKey, QSettings::NativeFormat);
    QMap<QString, QString> apps;

    foreach (const QString &appName, settings.childKeys())
    {
        QString capabilities = QString("HKEY_LOCAL_MACHINE\\%1").arg(settings.value(appName).toString());
        QSettings capSetting(capabilities, QSettings::NativeFormat);
        if (!capSetting.childGroups().contains("FileAssociations"))
            continue;

        capSetting.beginGroup("FileAssociations");
        foreach (const QString &suffix, capSetting.childKeys())
            apps.insert(capSetting.value(suffix).toString(), appName);
    }
    return apps;
}

QString getRegistry(const QString &key, const QString &path)
{
    wchar_t value[256];
    DWORD bufferSize = 8192;
    RegGetValue(HKEY_CLASSES_ROOT, path.toStdWString().data(), key.toStdWString().data(), RRF_RT_ANY, NULL, (void *)&value, &bufferSize);
    return QString::fromWCharArray(value);
}



#define MUICACHE "Local Settings\\Software\\Microsoft\\Windows\\Shell\\MuiCache"

static QMap<QString, QString> appsExe()
{
    const QString &regKey = QString("HKEY_CLASSES_ROOT\\Local Settings\\Software\\Microsoft\\Windows\\Shell\\MuiCache");
    QSettings settings(regKey, QSettings::NativeFormat);
    QMap<QString, QString> apps;

    foreach (QString ck, settings.childKeys())
    {
        ck.replace("/", "\\");
        if (QFileInfo(ck).exists())
            apps.insert(QFileInfo(ck).fileName().toLower(), getRegistry(ck, QString(MUICACHE)));
    }
    return apps;
}

static QMap<QString, QString> map(const QString &command)
{
    QMap<QString, QString> typeMap;
    QProcess p;
    p.start("cmd", QStringList() << "/C" << command, QIODevice::ReadOnly);
    p.waitForFinished();

    QTextStream output(&p);
    while (!output.atEnd())
    {
        QString line = output.readLine();
        QStringList types = line.split("=");
        if (types.count()<2)
            continue;
        typeMap.insert(types.at(0), types.at(1));
    }
    return typeMap;
}

#endif

QList<QAction *>
Store::openWithActions(const QString &file)
{
    QList<QAction *> actionList;
#if defined(ISUNIX)
    QFile mimeTypes("/usr/share/mime/globs");
    mimeTypes.open(QFile::ReadOnly);
    QStringList mimeTypesList = QString(mimeTypes.read(QFileInfo(mimeTypes).size())).split("\n", QString::SkipEmptyParts);
    mimeTypes.close();

    QMap<QString, QString> mimeMap;
    foreach (QString mimeType, mimeTypesList)
    {
        QStringList tmp = mimeType.split(":");
        if(tmp.count() >= 2)
        {
            const QString &mt = tmp.at(0);
            QString fn = tmp.at(1);
            fn.remove("*"); fn.remove(".");
            mimeMap[fn] = mt;
        }
    }

    QFile mimeInfo("/usr/share/applications/mimeinfo.cache");
    mimeInfo.open(QFile::ReadOnly);
    QStringList mimeList = QString(mimeInfo.read(QFileInfo(mimeInfo).size())).split("\n", QString::SkipEmptyParts);
    mimeInfo.close();

    QMap<QString, QStringList> appsMap;

    foreach (QString mimeString, mimeList)
    {
        QStringList tmp = mimeString.split("=");
        if(tmp.count() >= 2)
        {
            QString mt = tmp[0]; QString apps = tmp[1];
            QStringList appsList = apps.split(";");
            appsList.removeOne("");
            appsMap[mt] = appsList;
        }
    }

    QStringList apps = QStringList() << appsMap[DMimeProvider().getMimeType(file)] << appsMap[mimeMap[QFileInfo(file).suffix()]];
    apps.removeDuplicates();

    foreach(QString app, apps)
    {
        const QString &name = desktopInfo(app, true);
        if (name.isEmpty())
            continue;
        QAction *action = new QAction(name, instance());
        action->setProperty("file", file);
        connect(action, SIGNAL(triggered()), Ops::instance(), SLOT(openWith())) ;
        QVariant var;
        var.setValue(desktopInfo(app, false));
        action->setData(var);
        actionList << action;
    }
#else
    QFileInfo f(file);
    if (f.suffix().isEmpty()) //windows doesnt know files w/o suffix...
        return actionList;

    const QString &suffix = QString(".%1").arg(f.suffix());
//    QMap<QString, QString> assoc = map("assoc");
//    QMap<QString, QString> ftype = map("ftype");
//    QMap<QString, QString> set = map("set");
    QMap<QString, QString> appMap = apps();
    QMap<QString, QString> appExeMap = appsExe();

    const QString &regKey = QString("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%1\\OpenWithProgids").arg(suffix);
    QSettings settings(regKey, QSettings::NativeFormat);

    foreach (const QString &progId, settings.childKeys())
    {
        const QString &regCommand = QString("HKCR\\%1\\Shell\\Open\\Command").arg(progId);
        QString app = QSettings(regCommand, QSettings::NativeFormat).value(".").toString();

        QString appName;
        if (appMap.contains(progId))
            appName = appMap.value(progId);
        else
        {
            QString appExeDll;
            if (app.contains(".dll", Qt::CaseInsensitive))
            {
                QStringList list = app.split("\\");
                foreach (const QString &string, list)
                {
                    if (string.contains(".dll", Qt::CaseInsensitive))
                    {
                        appExeDll = string.toLower();
                        int start = appExeDll.indexOf("dll")+3;
                        int end = appExeDll.size()-start;
                        appExeDll.remove(start, end);
                        break;
                    }
                }
            }
            else if (app.contains(".exe", Qt::CaseInsensitive))
            {
                QStringList list = app.split("\\");
                foreach (const QString &string, list)
                {
                    if (string.contains(".exe", Qt::CaseInsensitive))
                    {
                        appExeDll = string.toLower();
                        int start = appExeDll.indexOf("exe")+3;
                        int end = appExeDll.size()-start;
                        appExeDll.remove(start, end);
                        break;
                    }
                }
            }
            appName = appExeMap.value(appExeDll);
        }

        if (app.contains("%1"))
            app = app.arg(QDir::toNativeSeparators(file));
        else
            app.append(QString(" %1").arg(QDir::toNativeSeparators(file)));


        QAction *action = new QAction(appName, instance());
        action->setProperty("file", QDir::toNativeSeparators(file));
        connect(action, SIGNAL(triggered()), Ops::instance(), SLOT(openWith())) ;
        action->setData(QVariant::fromValue(app));
        actionList << action;
    }
#endif
    return actionList;
}
