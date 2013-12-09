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

int main(int argc, char *argv[])
{
    //    Q_INIT_RESOURCE(resources);

#if QT_VERSION < 0x050000
    QApplication::setGraphicsSystem("raster");
#endif
    Application app(argc, argv);

    if ( app.isRunning() )
    {
        QStringList message;
        if ( argc>1 )
            for (int i = 0; i<argc; ++i)
                if ( i )
                    message << QString(argv[i]);
        app.setMessage(message);
        return 0;
    }

    DFM::Store::readConfig();

    if ( app.font().pointSize() < DFM::Store::config.behaviour.minFontSize )
    {
        QFont font = app.font();
        font.setPointSize(DFM::Store::config.behaviour.minFontSize);
        app.setFont(font);
    }

    if ( !DFM::Store::config.styleSheet.isEmpty() )
    {
        QFile file(DFM::Store::config.styleSheet);
        file.open(QFile::ReadOnly);
        app.setStyleSheet(file.readAll());
        file.close();
    }

    DFM::MainWindow *mainWin = new DFM::MainWindow(app.arguments());
    QObject::connect(&app, SIGNAL(lastMessage(QStringList)), mainWin, SLOT(receiveMessage(QStringList)));
    mainWin->show();
    return app.exec();
}

