/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
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

#include "iosdebugsupport.h"

#include "iosrunner.h"
#include "iosmanager.h"

#include <debugger/debuggerengine.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggerrunner.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerrunconfigurationaspect.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakebuildconfiguration.h>
#include <qmakeprojectmanager/qmakenodes.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qtsupport/qtkitinformation.h>

#include <QDir>
#include <QTcpServer>

#include <stdio.h>
#include <fcntl.h>
#ifdef Q_OS_UNIX
#include <unistd.h>
#else
#include <io.h>
#endif

using namespace Debugger;
using namespace ProjectExplorer;
using namespace QmakeProjectManager;

namespace Ios {
namespace Internal {

RunControl *IosDebugSupport::createDebugRunControl(IosRunConfiguration *runConfig,
                                                   QString *errorMessage)
{
    Target *target = runConfig->target();
    if (!target)
        return 0;
    ProjectExplorer::IDevice::ConstPtr device = DeviceKitInformation::device(target->kit());
    if (device.isNull())
        return 0;
    QmakeProject *project = static_cast<QmakeProject *>(target->project());

    DebuggerStartParameters params;
    if (device->type() == Core::Id(Ios::Constants::IOS_DEVICE_TYPE)) {
        params.startMode = AttachToRemoteProcess;
        params.platform = QLatin1String("remote-ios");
    } else {
        params.startMode = AttachExternal;
        params.platform = QLatin1String("ios-simulator");
    }
    params.displayName = runConfig->appName();
    params.remoteSetupNeeded = true;
    if (!params.breakOnMain)
        params.continueAfterAttach = true;

    Debugger::DebuggerRunConfigurationAspect *aspect
            = runConfig->extraAspect<Debugger::DebuggerRunConfigurationAspect>();
    if (aspect->useCppDebugger()) {
        params.languages |= CppLanguage;
        Kit *kit = target->kit();
        params.sysRoot = SysRootKitInformation::sysRoot(kit).toString();
        params.debuggerCommand = DebuggerKitInformation::debuggerCommand(kit).toString();
        if (ToolChain *tc = ToolChainKitInformation::toolChain(kit))
            params.toolChainAbi = tc->targetAbi();
        params.executable = runConfig->exePath().toString();
        params.remoteChannel = QLatin1String("connect://localhost:0");
    }
    if (aspect->useQmlDebugger()) {
        params.languages |= QmlLanguage;
        QTcpServer server;
        QTC_ASSERT(server.listen(QHostAddress::LocalHost)
                   || server.listen(QHostAddress::LocalHostIPv6), return 0);
        params.qmlServerAddress = server.serverAddress().toString();
        params.remoteSetupNeeded = true;
        //TODO: Not sure if these are the right paths.
        params.projectSourceDirectory = project->projectDirectory();
        params.projectSourceFiles = project->files(QmakeProject::ExcludeGeneratedFiles);
        params.projectBuildDirectory = project->rootQmakeProjectNode()->buildDir();
    }

    DebuggerRunControl * const debuggerRunControl
        = DebuggerPlugin::createDebugger(params, runConfig, errorMessage);
    if (debuggerRunControl)
        new IosDebugSupport(runConfig, debuggerRunControl);
    return debuggerRunControl;
}

IosDebugSupport::IosDebugSupport(IosRunConfiguration *runConfig,
    DebuggerRunControl *runControl)
    : QObject(runControl), m_runControl(runControl),
      m_runner(new IosRunner(this, runConfig, true)),
      m_qmlPort(0)
{

    connect(m_runControl->engine(), SIGNAL(requestRemoteSetup()),
            m_runner, SLOT(start()));
    connect(m_runControl, SIGNAL(finished()),
            m_runner, SLOT(stop()));

    connect(m_runner, SIGNAL(gotGdbserverPort(int)),
        SLOT(handleGdbServerPort(int)));
    connect(m_runner, SIGNAL(gotInferiorPid(Q_PID)),
        SLOT(handleGotInferiorPid(Q_PID)));
    connect(m_runner, SIGNAL(finished(bool)),
        SLOT(handleRemoteProcessFinished(bool)));

    connect(m_runner, SIGNAL(errorMsg(QString)),
        SLOT(handleRemoteErrorOutput(QString)));
    connect(m_runner, SIGNAL(appOutput(QString)),
        SLOT(handleRemoteOutput(QString)));
}

IosDebugSupport::~IosDebugSupport()
{
}

void IosDebugSupport::handleGdbServerPort(int gdbServerPort)
{
    if (gdbServerPort > 0) {
        m_runControl->engine()->notifyEngineRemoteSetupDone(gdbServerPort, m_qmlPort);
    } else {
        m_runControl->engine()->notifyEngineRemoteSetupFailed(
                    tr("Could not get debug server file descriptor."));
    }
}

void IosDebugSupport::handleGotInferiorPid(Q_PID pid)
{
    if (pid > 0) {
        //m_runControl->engine()->notifyInferiorPid(pid);
#ifndef Q_OS_WIN // Q_PID might be 64 bit pointer...
        m_runControl->engine()->notifyEngineRemoteSetupDone(int(pid), m_qmlPort);
#endif
    } else {
        m_runControl->engine()->notifyEngineRemoteSetupFailed(
                    tr("Got an invalid process id."));
    }
}

void IosDebugSupport::handleRemoteProcessFinished(bool cleanEnd)
{
    if (!cleanEnd && m_runControl)
        m_runControl->showMessage(tr("Run failed unexpectedly."), AppStuff);
    //m_runControl->engine()->notifyInferiorIll();
    m_runControl->engine()->abortDebugger();
}

void IosDebugSupport::handleRemoteOutput(const QString &output)
{
    if (m_runControl) {
        if (m_runControl->engine())
            m_runControl->engine()->showMessage(output, AppOutput);
        else
            m_runControl->showMessage(output, AppOutput);
    }
}

void IosDebugSupport::handleRemoteErrorOutput(const QString &output)
{
    if (m_runControl) {
        if (m_runControl->engine())
            m_runControl->engine()->showMessage(output + QLatin1Char('\n'), AppError);
        else
            m_runControl->showMessage(output + QLatin1Char('\n'), AppError);
    }
}

} // namespace Internal
} // namespace Ios
