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

#include "coreplugin.h"
#include "designmode.h"
#include "editmode.h"
#include "helpmanager.h"
#include "mainwindow.h"
#include "mimedatabase.h"
#include "modemanager.h"
#include "infobar.h"
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>

#include <utils/savefile.h>

#include <QtPlugin>
#include <QDebug>
#include <QDateTime>

using namespace Core;
using namespace Core::Internal;

CorePlugin::CorePlugin() : m_editMode(0), m_designMode(0)
{
	//如果信号曹机制的参数有Core::Id类型就必须向系统注册才能使用
    qRegisterMetaType<Core::Id>(); //为类型 Core::Id注册类型名称
    m_mainWindow = new MainWindow;
}

CorePlugin::~CorePlugin()
{
    if (m_editMode) {
        removeObject(m_editMode);
        delete m_editMode;
    }

    if (m_designMode) {
        if (m_designMode->designModeIsRequired())
            removeObject(m_designMode);
        delete m_designMode;
    }

    delete m_mainWindow;
}
//解析命令行参数 设置主题色 以及演示模式
void CorePlugin::parseArguments(const QStringList &arguments)
{
    for (int i = 0; i < arguments.size(); ++i) {
        if (arguments.at(i) == QLatin1String("-color")) { //解析默认色
            const QString colorcode(arguments.at(i + 1));
            m_mainWindow->setOverrideColor(QColor(colorcode)); //设置默认色
            i++; // skip the argument
        }
        if (arguments.at(i) == QLatin1String("-presentationMode")) //如果指定演示模式
            ActionManager::setPresentationModeEnabled(true); //设置演示模式
    }
}

bool CorePlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    qsrand(QDateTime::currentDateTime().toTime_t()); //设置随机种子
    parseArguments(arguments); //解析命令行参数 设置主题色 以及演示模式
    const bool success = m_mainWindow->init(errorMessage);//启动主窗口
    if (success) {
		//创建并在插件管理器中添加编辑模型对象
        m_editMode = new EditMode;
        addObject(m_editMode);

        ModeManager::activateMode(m_editMode->id()); //设置当前模式为编辑模式

		//创建设计模式对象
        m_designMode = new DesignMode;

        InfoBar::initializeGloballySuppressed();
    }

    // Make sure we respect the process's umask when creating new files
	//在创建新文件时，请确保我们尊重进程的umask
    Utils::SaveFile::initializeUmask();

    return success;
}

void CorePlugin::extensionsInitialized()
{
    MimeDatabase::syncUserModifiedMimeTypes();
    if (m_designMode->designModeIsRequired())
        addObject(m_designMode);
    m_mainWindow->extensionsInitialized();
}

bool CorePlugin::delayedInitialize()
{
    HelpManager::setupHelpManager();
    return true;
}
//核心插件接口类对于参数的解析
QObject *CorePlugin::remoteCommand(const QStringList & /* options */, const QStringList &args)
{
    IDocument *res = m_mainWindow->openFiles(
                args, ICore::OpenFilesFlags(ICore::SwitchMode | ICore::CanContainLineNumbers));
    m_mainWindow->raiseWindow();
    return res;
}

void CorePlugin::fileOpenRequest(const QString &f)
{
    remoteCommand(QStringList(), QStringList(f));
}

ExtensionSystem::IPlugin::ShutdownFlag CorePlugin::aboutToShutdown()
{
    m_mainWindow->aboutToShutdown();
    return SynchronousShutdown;
}

Q_EXPORT_PLUGIN(CorePlugin)
