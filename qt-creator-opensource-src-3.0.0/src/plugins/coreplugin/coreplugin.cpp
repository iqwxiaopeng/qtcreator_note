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
	//����źŲܻ��ƵĲ�����Core::Id���;ͱ�����ϵͳע�����ʹ��
    qRegisterMetaType<Core::Id>(); //Ϊ���� Core::Idע����������
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
//���������в��� ��������ɫ �Լ���ʾģʽ
void CorePlugin::parseArguments(const QStringList &arguments)
{
    for (int i = 0; i < arguments.size(); ++i) {
        if (arguments.at(i) == QLatin1String("-color")) { //����Ĭ��ɫ
            const QString colorcode(arguments.at(i + 1));
            m_mainWindow->setOverrideColor(QColor(colorcode)); //����Ĭ��ɫ
            i++; // skip the argument
        }
        if (arguments.at(i) == QLatin1String("-presentationMode")) //���ָ����ʾģʽ
            ActionManager::setPresentationModeEnabled(true); //������ʾģʽ
    }
}

bool CorePlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    qsrand(QDateTime::currentDateTime().toTime_t()); //�����������
    parseArguments(arguments); //���������в��� ��������ɫ �Լ���ʾģʽ
    const bool success = m_mainWindow->init(errorMessage);//����������
    if (success) {
		//�������ڲ������������ӱ༭ģ�Ͷ���
        m_editMode = new EditMode;
        addObject(m_editMode);

        ModeManager::activateMode(m_editMode->id()); //���õ�ǰģʽΪ�༭ģʽ

		//�������ģʽ����
        m_designMode = new DesignMode;

        InfoBar::initializeGloballySuppressed();
    }

    // Make sure we respect the process's umask when creating new files
	//�ڴ������ļ�ʱ����ȷ���������ؽ��̵�umask
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
//���Ĳ���ӿ�����ڲ����Ľ���
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
