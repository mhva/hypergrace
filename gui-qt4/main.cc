/*
   Copyright (C) 2009 Anton Mihalyov <anton@glyphsense.com>

   This  library is  free software;  you can  redistribute it  and/or
   modify  it under  the  terms  of the  GNU  Library General  Public
   License  (LGPL)  as published  by  the  Free Software  Foundation;
   either version  2 of the  License, or  (at your option)  any later
   version.

   This library  is distributed in the  hope that it will  be useful,
   but WITHOUT  ANY WARRANTY;  without even  the implied  warranty of
   MERCHANTABILITY or FITNESS  FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy  of the GNU Library General Public
   License along with this library; see the file COPYING.LIB. If not,
   write to the  Free Software Foundation, Inc.,  51 Franklin Street,
   Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDebug>
#include <QLocale>
#include <QMainWindow>
#include <QString>
#include <QToolBar>
#include <QTranslator>

#include "torrentview.hh"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QTranslator translator;
    translator.load(QString("hypergrace_") + QLocale::system().name());
    app.installTranslator(&translator);

    QMainWindow *mainWindow = new QMainWindow();
    QToolBar *mainToolbar = new QToolBar(mainWindow);
    TorrentView *mainWidget = new TorrentView(mainWindow);
    QAction *openAction = new QAction(QObject::tr("Open"), mainToolbar);
    QAction *startAction = new QAction(QObject::tr("Start"), mainToolbar);
    QAction *pauseAction = new QAction(QObject::tr("Pause"), mainToolbar);
    QAction *removeAction = new QAction(QObject::tr("Remove"), mainToolbar);

    QObject::connect(openAction, SIGNAL(triggered()), mainWidget, SLOT(initiateLocalOpen()));

    mainToolbar->addAction(openAction);
    mainToolbar->addAction(startAction);
    mainToolbar->addAction(pauseAction);
    mainToolbar->addAction(removeAction);
    mainToolbar->setFloatable(false);
    mainToolbar->setMovable(false);
    mainToolbar->setContextMenuPolicy(Qt::PreventContextMenu);
    mainToolbar->setWindowTitle("Main Toolbar");

    //mainWindow->resize(605, 393);
    mainWindow->resize(465, 309);
    mainWindow->setWindowTitle("Hypergrace");
    mainWindow->addToolBar(mainToolbar);
    mainWindow->setCentralWidget(mainWidget);
    mainWindow->show();

    return app.exec();
}
