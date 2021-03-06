/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Build Suite.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "configcommandlineparser.h"
#include "configcommandexecutor.h"
#include "../shared/logging/consolelogger.h"
#include "../shared/qbssettings.h"

#include <logging/translator.h>
#include <tools/error.h>

#include <QCoreApplication>

#include <cstdlib>

using qbs::Internal::Tr;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    SettingsPtr settings = qbsSettings();
    ConfigCommandLineParser parser;
    ConsoleLogger::instance(settings.data());
    try {
        parser.parse(app.arguments().mid(1));
        if (parser.helpRequested()) {
            qbsInfo() << Tr::tr("This tool manages qbs settings.");
            parser.printUsage();
            return EXIT_SUCCESS;
        }
        ConfigCommandExecutor(settings.data()).execute(parser.command());
    } catch (const qbs::ErrorInfo &e) {
        qbsError() << e.toString();
        parser.printUsage();
        return EXIT_FAILURE;
    }
}
