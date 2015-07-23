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

#include "application.h"
#include "mainwindow.h"
#include "config.h"
#include "iojob.h"
#include "operations.h"
#include <typeinfo>
//#include <dsp/settings.h>

int main(int argc, char *argv[])
{
    //    Q_INIT_RESOURCE(resources);
#if QT_VERSION < 0x050000
    QApplication::setGraphicsSystem("raster");
#endif
    Application app(argc, argv);
    if (app.isRunning())
    {
        app.setMessage(app.arguments());
        return 0;
    }

    switch (app.appType())
    {
    case Application::Browser:
    {
        if (app.font().pointSize() < DFM::Store::config.behaviour.minFontSize)
        {
            QFont font = app.font();
            font.setPointSize(DFM::Store::config.behaviour.minFontSize);
            app.setFont(font);
        }

        if (!DFM::Store::config.styleSheet.isEmpty())
        {
            QFile file(DFM::Store::config.styleSheet);
            file.open(QFile::ReadOnly);
            app.setStyleSheet(file.readAll());
            file.close();
        }
        DFM::MainWindow *mainWin = new DFM::MainWindow(app.arguments());
        QObject::connect(&app, SIGNAL(lastMessage(QStringList)), mainWin, SLOT(receiveMessage(QStringList)));
        mainWin->show();
        break;
    }
    case Application::IOJob:
    {
        DFM::IO::Manager *manager = new DFM::IO::Manager();
        if (DFM::Store::config.behaviour.useIOQueue)
            QObject::connect(&app, SIGNAL(lastMessage(QStringList)), manager, SLOT(getMessage(QStringList)));
        DFM::IOJobData ioJobData;
        if (DFM::Ops::extractIoData(app.arguments(), ioJobData))
            manager->queue(ioJobData);
        break;
    }
    default: break;
    } //end t

//    Settings::initiate();
//    qDebug() << Settings::readVal(Settings::Inputgrad);


    return app.exec();
}

