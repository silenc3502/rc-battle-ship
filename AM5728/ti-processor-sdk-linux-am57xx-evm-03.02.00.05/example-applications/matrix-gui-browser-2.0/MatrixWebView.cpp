/*
 * MatrixWebView.cpp
 *
 * MatrixWebView Class inherted from QWebView to support additional
 * functionalities such as signal handler
 *
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
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
#include <QtNetwork>
#include <QtWebKitWidgets>
#include "MatrixWebView.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <QApplication>

int MatrixWebView::sigtermFd[2];

MatrixWebView::MatrixWebView(const QUrl& url)
{
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigtermFd))
       qFatal("Couldn't create TERM socketpair");

    view = new QWebView(this);
    view->load(url);
    setCentralWidget(view);

    snTerm = new QSocketNotifier(sigtermFd[1], QSocketNotifier::Read, this);
    connect(snTerm, SIGNAL(activated(int)), this, SLOT(handleSigTerm()));
}

void MatrixWebView::termSignalHandler(int)
{
    char a = 1;
    if(::write(sigtermFd[0], &a, sizeof(a))){}
}

void MatrixWebView::handleSigTerm()
{
    snTerm->setEnabled(false);
    char tmp;
    if(::read(sigtermFd[1], &tmp, sizeof(tmp))){}

    // do Qt stuff
    qApp->quit();

    snTerm->setEnabled(true);
}
