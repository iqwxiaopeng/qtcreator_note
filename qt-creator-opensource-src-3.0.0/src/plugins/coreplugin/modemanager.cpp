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

#include "modemanager.h"

#include "fancytabwidget.h"
#include "fancyactionbar.h"
#include "icore.h"
#include "mainwindow.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/imode.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/qtcassert.h>

#include <QDebug>
#include <QMap>
#include <QVector>

#include <QSignalMapper>
#include <QShortcut>
#include <QAction>

namespace Core {

/*!
    \class Core::ModeManager

    The mode manager handles everything related to the instances of IMode
    that were added to the plugin manager's object pool as well as their
    buttons and the tool bar with the round buttons in the lower left
    corner of Qt Creator.

	ģʽ��������������ӵ��������������ص�IModeʵ����ص���������,�Լ����ǵİ�ť�͹�����,����Բ�ΰ�ťλ��qt creator�����½�
*/

struct ModeManagerPrivate
{
    Internal::MainWindow *m_mainWindow; //���������
    Internal::FancyTabWidget *m_modeStack; //ģʽ����  �������ϵ�����
    Internal::FancyActionBar *m_actionBar; //ѡ���� ֮ Corner ���½ǵ�ѡ��˵� [���� ���� �����Ǽ���] ������ĵ�һ��Ԫ��
    QMap<QAction*, int> m_actions; //������ �� ���ȼ����ձ�
    QVector<IMode*> m_modes; //ģʽ����
    QVector<Command*> m_modeShortcuts; //ģʽ��ݷ�ʽ�� idxΪģʽ���� ��ӦCommandΪ��ݷ�ʽ
    QSignalMapper *m_signalMapper; //ת����. ����,��ť�������Ӧ��,�󶨵�QSignalMapper��,QSignalMapper�յ���ť�ĵ����,��֪ͨ������Ŀؼ���������
    Context m_addedContexts; //��ǰ����չʾģʽ�� ���
    int m_oldCurrent; //��ǰ֣��չʾģʽ������
    bool m_saveSettingsOnModeChange; //����Ƿ���ģʽ�л�ʱҪ����������Ϣ
    bool m_modeSelectorVisible; //���ģ��ѡ���� ��ർ��  �Ƿ���ʾ
};
//��̬ȫ��ģʽ����������ʵ��
static ModeManagerPrivate *d;

//ģʽ������ȫ��ʵ��
static ModeManager *m_instance = 0;

//����id��ȡģʽ����
static int indexOf(Id id)
{
    for (int i = 0; i < d->m_modes.count(); ++i) {
        if (d->m_modes.at(i)->id() == id)
            return i;
    }
    qDebug() << "Warning, no such mode:" << id.toString();
    return -1;
}

ModeManager::ModeManager(Internal::MainWindow *mainWindow,
                         Internal::FancyTabWidget *modeStack)
{
    m_instance = this; //������ֵ
    d = new ModeManagerPrivate(); //�����൥����ֵ
    d->m_mainWindow = mainWindow; //������
    d->m_modeStack = modeStack; //����������

	//ת����. ����,��ť�������Ӧ��,�󶨵�QSignalMapper��,QSignalMapper�յ���ť�ĵ����,��֪ͨ������Ŀؼ���������
    d->m_signalMapper = new QSignalMapper(this);

    d->m_oldCurrent = -1;

	//�� CornerWidget������ CornerWidgetΪһ��FancyActionBar
    d->m_actionBar = new Internal::FancyActionBar(modeStack);
    d->m_modeStack->addCornerWidget(d->m_actionBar);

    d->m_saveSettingsOnModeChange = false;

	//����ѡ���� ��ʾ
    d->m_modeSelectorVisible = true;
    d->m_modeStack->setSelectionWidgetVisible(d->m_modeSelectorVisible);

	//�¼���
    connect(d->m_modeStack, SIGNAL(currentAboutToShow(int)), SLOT(currentTabAboutToChange(int))); //ģ�ͽ�Ҫ�л���Ϣ����  �ؼ�չʾǰ�ᷢ����Ϣ
    connect(d->m_modeStack, SIGNAL(currentChanged(int)), SLOT(currentTabChanged(int))); //ģʽѡ���� ѡ��ı��¼�����
    connect(d->m_signalMapper, SIGNAL(mapped(int)), this, SLOT(slotActivateMode(int))); //�¼�ת���� slotActivateMode
    connect(ExtensionSystem::PluginManager::instance(), SIGNAL(initializationDone()), this, SLOT(handleStartup()));
    connect(ICore::instance(), SIGNAL(coreAboutToClose()), this, SLOT(handleShutdown()));
}

//��ʼ�� ����� �Ƴ�model�¼�
void ModeManager::init()
{
    QObject::connect(ExtensionSystem::PluginManager::instance(), SIGNAL(objectAdded(QObject*)),  m_instance, SLOT(objectAdded(QObject*))); //������������������Ϣ
    QObject::connect(ExtensionSystem::PluginManager::instance(), SIGNAL(aboutToRemoveObject(QObject*)),  m_instance, SLOT(aboutToRemoveObject(QObject*))); //����������Ƴ���Ϣ
}

ModeManager::~ModeManager()
{
    delete d;
    d = 0;
    m_instance = 0;
}

//��Corner ����һ����
void ModeManager::addWidget(QWidget *widget)
{
    // We want the actionbar to stay on the bottom
    // so d->m_modeStack->cornerWidgetCount() -1 inserts it at the position immediately above
    // the actionbar
    d->m_modeStack->insertCornerWidget(d->m_modeStack->cornerWidgetCount() -1, widget);
}

//��ȡ��ǰ�� ģʽ
IMode *ModeManager::currentMode()
{
    int currentIndex = d->m_modeStack->currentIndex();
    if (currentIndex < 0)
        return 0;
    return d->m_modes.at(currentIndex);
}

//����ģʽid��ȡģʽ
IMode *ModeManager::mode(Id id)
{
    const int index = indexOf(id);
    if (index >= 0)
        return d->m_modes.at(index);
    return 0;
}
//����id ����Tab����ǰ��Ծ�˵���(Ҳ�������õ�ǰģʽ)
void ModeManager::slotActivateMode(int id)
{
    m_instance->activateMode(Id::fromUniqueIdentifier(id)); //���õ�ǰģʽ
    ICore::raiseWindow(d->m_modeStack); //�������������
}
//����id ����Tab����ǰ��Ծ�˵���(Ҳ�������õ�ǰģʽ)
void ModeManager::activateMode(Id id)
{
    const int index = indexOf(id);
    if (index >= 0)
        d->m_modeStack->setCurrentIndex(index);
}

//����������ģʽ
void ModeManager::objectAdded(QObject *obj)
{
    IMode *mode = Aggregation::query<IMode>(obj);
    if (!mode) //û��ע���ģʽ���������
        return;

    d->m_mainWindow->addContextObject(mode);

    // Count the number of modes with a higher priority ���ұȵ�ǰģʽ���ȼ��ߵ�ģʽ����
    int index = 0;
    foreach (const IMode *m, d->m_modes)
        if (m->priority() > mode->priority())
            ++index;

    d->m_modes.insert(index, mode);//���뵽ģʽ��
    d->m_modeStack->insertTab(index, mode->widget(), mode->icon(), mode->displayName()); //ģʽTab�˵���Ŀ�Լ�ģʽ���������
    d->m_modeStack->setTabEnabled(index, mode->isEnabled()); //����ģʽ ״̬ ����or��ֹ

    // Register mode shortcut
    const Id shortcutId = mode->id().withPrefix("QtCreator.Mode.");//����ǰģʽidָ�����ַ������ǰ׺ ����������ģʽ��ݼ���id
    QShortcut *shortcut = new QShortcut(d->m_mainWindow);//������ݼ�
    shortcut->setWhatsThis(tr("Switch to <b>%1</b> mode").arg(mode->displayName())); //���ÿ�ݼ���ʾ��Ϣ
    Command *cmd = ActionManager::registerShortcut(shortcut, shortcutId, Context(Constants::C_GLOBAL)); //ע���ݼ�

    d->m_modeShortcuts.insert(index, cmd); //�����ݷ�ʽ��

    connect(cmd, SIGNAL(keySequenceChanged()), m_instance, SLOT(updateModeToolTip())); //�󶨿�ݷ�ʽ�ı䴦����

    //��û�����ÿ�ݰ����Ŀ�ݷ�ʽ���ÿ�ݰ���
    for (int i = 0; i < d->m_modeShortcuts.size(); ++i) {
        Command *currentCmd = d->m_modeShortcuts.at(i);
        // we need this hack with currentlyHasDefaultSequence
        // because we call setDefaultShortcut multiple times on the same cmd
        // and still expect the current shortcut to change with it
        bool currentlyHasDefaultSequence = (currentCmd->keySequence() == currentCmd->defaultKeySequence()); //�жϵ�ǰ��ݷ�ʽ�Ƿ���Ĭ�ϰ���

        //Ϊ��ݷ�ʽ����Ĭ�Ͽ�ݰ���
        currentCmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? QString::fromLatin1("Meta+%1").arg(i+1) : QString::fromLatin1("Ctrl+%1").arg(i+1))); 
        if (currentlyHasDefaultSequence) //��ԭ�Ѿ����ÿ�ݰ����Ŀ�ݷ�ʽ
            currentCmd->setKeySequence(currentCmd->defaultKeySequence());
    }

	//��ݷ�ʽ�󶨵� maper  Ȼ�� ����maper��ת�� ����ģʽ���ں��� �Ӷ�ʵ�ֿ�ݼ��л�ģʽ
    d->m_signalMapper->setMapping(shortcut, mode->id().uniqueIdentifier());
    connect(shortcut, SIGNAL(activated()), d->m_signalMapper, SLOT(map()));

    connect(mode, SIGNAL(enabledStateChanged(bool)),m_instance, SLOT(enabledStateChanged())); //ģʽ������Ϣ����
}

//ģʽ��ݷ�ʽ�ı���Ϣ����
void ModeManager::updateModeToolTip()
{
    Command *cmd = qobject_cast<Command *>(sender());
    if (cmd) {
        int index = d->m_modeShortcuts.indexOf(cmd);
        if (index != -1) //��ȡģʽ����
            d->m_modeStack->setTabToolTip(index, cmd->stringWithAppendedShortcut(cmd->shortcut()->whatsThis())); //����ģʽ��Ӧ ���˵�����ť����Ϣ��ʾΪ�����õĿ�ݼ���ʾ
    }
}

//����ģʽ������Ϣ
void ModeManager::enabledStateChanged()
{
    IMode *mode = qobject_cast<IMode *>(sender()); //��ȡ���õ�ģʽ
    QTC_ASSERT(mode, return);
    int index = d->m_modes.indexOf(mode);
    QTC_ASSERT(index >= 0, return);
    d->m_modeStack->setTabEnabled(index, mode->isEnabled()); //����Tab������

    // Make sure we leave any disabled mode to prevent possible crashes:
    if (mode == currentMode() && !mode->isEnabled()) { // ���ģʽ�ǵ�ǰģʽ ����û�н�����ֻ��Ҫ����
        // This assumes that there is always at least one enabled mode.
        for (int i = 0; i < d->m_modes.count(); ++i) {
            if (d->m_modes.at(i) != mode && d->m_modes.at(i)->isEnabled()) { //���˵�ǰģʽ ��������ģʽû�н���
                //���ǰģʽ ���õ�ǰģʽ
                activateMode(d->m_modes.at(i)->id());
                break;
            }
        }
    }
}

//PluginManger initializationDone �źŴ���
void ModeManager::handleStartup()
{ d->m_saveSettingsOnModeChange = true; }
 
//ICore��coreAboutToClose �źŴ���
void ModeManager::handleShutdown()
{ d->m_saveSettingsOnModeChange = false; }

//�Ƴ�ģʽ 1.ģʽ���Ƴ� 2.��ݰ������Ƴ� 3.Tab�˵����Ƴ� 4.�������Ƴ�ģʽ���
void ModeManager::aboutToRemoveObject(QObject *obj)
{
    IMode *mode = Aggregation::query<IMode>(obj); //δע��ֱ�ӷ���
    if (!mode)
        return;

    const int index = d->m_modes.indexOf(mode);
    d->m_modes.remove(index); //ģʽ���Ƴ�
    d->m_modeShortcuts.remove(index); //��ݰ������Ƴ�
    d->m_modeStack->removeTab(index); //Tab�˵����Ƴ�

    d->m_mainWindow->removeContextObject(mode); //�������Ƴ�ģʽ���
}

//�������ȼ���Corner����������
void ModeManager::addAction(QAction *action, int priority)
{
    d->m_actions.insert(action, priority); //���������ӳ���

    // Count the number of commands with a higher priority ������ڵ�ǰ���ȼ��Ĳ�������
    int index = 0;
    foreach (int p, d->m_actions) {
        if (p > priority)
            ++index;
    }

    //���������������ȼ�����Corner�е�actionBar��
    d->m_actionBar->insertAction(index, action);
}

//����������ȼ��Ĳ�����Ŀ ����Ŀѡ������
void ModeManager::addProjectSelector(QAction *action)
{
    d->m_actionBar->addProjectSelector(action);
    d->m_actions.insert(0, INT_MAX);
}

//ģ�ͽ�Ҫ�л���Ϣ����
void ModeManager::currentTabAboutToChange(int index)
{
    if (index >= 0) {
        IMode *mode = d->m_modes.at(index); //ȡ��ģʽ
        if (mode) {
            if (d->m_saveSettingsOnModeChange) //���ģ���л���Ҫ����������Ϣ ��֪ͨ���Ĳ����������
                ICore::saveSettings();
            emit currentModeAboutToChange(mode);
        }
    }
}

//ģʽѡ�����ı�ģʽѡ����Ϣ����
void ModeManager::currentTabChanged(int index)
{
    // Tab index changes to -1 when there is no tab left.
    if (index >= 0) {
        IMode *mode = d->m_modes.at(index); //ȡ���¸ı��ģʽ Ҳ���ǵ�ǰչʾģʽ

        // FIXME: This hardcoded context update is required for the Debug and Edit modes, since
        // they use the editor widget, which is already a context widget so the main window won't
        // go further up the parent tree to find the mode context.
        ICore::updateAdditionalContexts(d->m_addedContexts, mode->context()); //�л���
        d->m_addedContexts = mode->context(); //���µ�ǰ����չʾģʽ���

        IMode *oldMode = 0;
        if (d->m_oldCurrent >= 0)
            oldMode = d->m_modes.at(d->m_oldCurrent); //��ȡ��ģʽ
        d->m_oldCurrent = index; //���µ�ǰչʾģʽ��������Ϣ
        emit currentModeChanged(mode, oldMode); //����ģ��ת����Ϣ ����Ϊ  ���л�ģʽ �� ��ģʽ
    }
}

//���ý��㵽��ǰģʽ���� ���õ�ǰ�����������ʾ
void ModeManager::setFocusToCurrentMode()
{
    IMode *mode = currentMode(); //��ȡ��ǰ�� ģʽ
    QTC_ASSERT(mode, return);
    QWidget *widget = mode->widget(); //�õ�ģʽ��Ӧ���� ������Ϊ���㴰��
    if (widget) {
        QWidget *focusWidget = widget->focusWidget();
        if (!focusWidget)
            focusWidget = widget;
        focusWidget->setFocus();
        ICore::raiseWindow(focusWidget); //�ô�����ϵͳ������ʾ
    } 
}

//���� ģ��ѡ���� ��ർ��  �Ƿ���ʾ
void ModeManager::setModeSelectorVisible(bool visible)
{
    d->m_modeSelectorVisible = visible;
    d->m_modeStack->setSelectionWidgetVisible(visible);
}

//��ȡ ģ��ѡ���� ��ർ��  �Ƿ���ʾ
bool ModeManager::isModeSelectorVisible()
{
    return d->m_modeSelectorVisible;
}

//��ȡ������ʵ��
QObject *ModeManager::instance()
{
    return m_instance;
}

} // namespace Core
