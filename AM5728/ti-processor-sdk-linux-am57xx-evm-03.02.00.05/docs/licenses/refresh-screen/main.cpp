/*
 * refresh-screen
 *
 * application to force a screen refresh
 *
 * Copyright (C) 2011,2012 Texas Instruments Incorporated - http://www.ti.com/ 
 * 
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/

#include <qglobal.h>
#if QT_VERSION < 0x050000
#include <QtGui/QApplication>
#else
#include <QtWidgets/QApplication>
#endif

#include "mainwindow.h"
#include <QStringList>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
	bool transparentBackground = true;

    w.setContextMenuPolicy(Qt::NoContextMenu);
    w.setWindowFlags(Qt::FramelessWindowHint);


    QStringList args = a.arguments();

    for ( int x = 0; x < args.count() ; x++) {
        if(args[x].contains("background-black", Qt::CaseInsensitive)) {
            transparentBackground = false;
            break;
       } 
    }

    if ( transparentBackground == true )
        w.setAttribute(Qt::WA_TranslucentBackground ,true );
    else
        w.setStyleSheet("QMainWindow {background: 'black';}");

    w.showFullScreen();
    w.show();

    return a.exec();
}
