/*
 * matrix-browser
 *
 * Simple web viewer used by Matrix application launcher
 *
 * Copyright (C) 2011-2015 Texas Instruments Incorporated - http://www.ti.com/
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

#include <QtWidgets>
#include <iostream> 
#include <QApplication>
#include "MatrixWebView.h"
#include "signal.h"

static int setup_unix_signal_handlers()
{
    struct sigaction term;

    term.sa_handler = MatrixWebView::termSignalHandler;
    sigemptyset(&term.sa_mask);
    term.sa_flags = SA_RESTART;

    if (sigaction(SIGTERM, &term, 0) > 0)
       return 1;

    return 0;
}

using std::endl;

int main(int argc, char **argv)
{
  QApplication app(argc, argv);
  QStringList args = app.arguments();
  if (args.count() < 2)
  {
    std::cerr << "You need to pass the URL you want to view\n" << endl;
    std::cerr << "Usage: " << argv[0] << " <URL> [--windowed]\n" << endl;
    return 1;
  }

  bool windowed = QApplication::arguments().contains("--windowed");

  MatrixWebView* windowView = new MatrixWebView(QUrl(args[1]));

  if ( !windowed )
  {
    windowView->showFullScreen();
  }
  else
    windowView->show();

  int ret = setup_unix_signal_handlers();
  if ( ret != 0 )
  {
    std::cerr << "Error setting up signal handler!" << endl;
    return 1;
  }

  return app.exec();
}
