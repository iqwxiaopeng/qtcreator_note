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

	模式管理器处理与添加到插件管理器对象池的IMode实例相关的所有内容,以及它们的按钮和工具栏,其中圆形按钮位于qt creator的左下角
*/

struct ModeManagerPrivate
{
    Internal::MainWindow *m_mainWindow; //主窗体对象
    Internal::FancyTabWidget *m_modeStack; //模式对象  主窗体上的内容
    Internal::FancyActionBar *m_actionBar; //选择栏 之 Corner 左下角的选择菜单 [构建 调试 运行那几个] 所插入的第一个元素
    QMap<QAction*, int> m_actions; //操作项 与 优先级对照表
    QVector<IMode*> m_modes; //模式数组
    QVector<Command*> m_modeShortcuts; //模式快捷方式表 idx为模式索引 对应Command为快捷方式
    QSignalMapper *m_signalMapper; //转发器. 比如,按钮点击的响应槽,绑定到QSignalMapper上,QSignalMapper收到按钮的点击后,又通知到另外的控件上做处理
    Context m_addedContexts; //当前正在展示模式的 句柄
    int m_oldCurrent; //当前郑州展示模式的索引
    bool m_saveSettingsOnModeChange; //标记是否在模式切换时要保存设置信息
    bool m_modeSelectorVisible; //标记模型选择栏 左侧导航  是否显示
};
//静态全局模式管理器数据实例
static ModeManagerPrivate *d;

//模式管理器全局实例
static ModeManager *m_instance = 0;

//根据id获取模式索引
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
    m_instance = this; //单例赋值
    d = new ModeManagerPrivate(); //数据类单例赋值
    d->m_mainWindow = mainWindow; //主窗体
    d->m_modeStack = modeStack; //主窗体内容

	//转发器. 比如,按钮点击的响应槽,绑定到QSignalMapper上,QSignalMapper收到按钮的点击后,又通知到另外的控件上做处理
    d->m_signalMapper = new QSignalMapper(this);

    d->m_oldCurrent = -1;

	//给 CornerWidget增加项 CornerWidget为一个FancyActionBar
    d->m_actionBar = new Internal::FancyActionBar(modeStack);
    d->m_modeStack->addCornerWidget(d->m_actionBar);

    d->m_saveSettingsOnModeChange = false;

	//设置选择栏 显示
    d->m_modeSelectorVisible = true;
    d->m_modeStack->setSelectionWidgetVisible(d->m_modeSelectorVisible);

	//事件绑定
    connect(d->m_modeStack, SIGNAL(currentAboutToShow(int)), SLOT(currentTabAboutToChange(int))); //模型将要切换消息处理  控件展示前会发此消息
    connect(d->m_modeStack, SIGNAL(currentChanged(int)), SLOT(currentTabChanged(int))); //模式选择栏 选择改变事件处理
    connect(d->m_signalMapper, SIGNAL(mapped(int)), this, SLOT(slotActivateMode(int))); //事件转发到 slotActivateMode
    connect(ExtensionSystem::PluginManager::instance(), SIGNAL(initializationDone()), this, SLOT(handleStartup()));
    connect(ICore::instance(), SIGNAL(coreAboutToClose()), this, SLOT(handleShutdown()));
}

//初始化 绑定添加 移除model事件
void ModeManager::init()
{
    QObject::connect(ExtensionSystem::PluginManager::instance(), SIGNAL(objectAdded(QObject*)),  m_instance, SLOT(objectAdded(QObject*))); //处理插件管理器增加消息
    QObject::connect(ExtensionSystem::PluginManager::instance(), SIGNAL(aboutToRemoveObject(QObject*)),  m_instance, SLOT(aboutToRemoveObject(QObject*))); //插件管理器移除消息
}

ModeManager::~ModeManager()
{
    delete d;
    d = 0;
    m_instance = 0;
}

//给Corner 增加一个项
void ModeManager::addWidget(QWidget *widget)
{
    // We want the actionbar to stay on the bottom
    // so d->m_modeStack->cornerWidgetCount() -1 inserts it at the position immediately above
    // the actionbar
    d->m_modeStack->insertCornerWidget(d->m_modeStack->cornerWidgetCount() -1, widget);
}

//获取当前的 模式
IMode *ModeManager::currentMode()
{
    int currentIndex = d->m_modeStack->currentIndex();
    if (currentIndex < 0)
        return 0;
    return d->m_modes.at(currentIndex);
}

//根据模式id获取模式
IMode *ModeManager::mode(Id id)
{
    const int index = indexOf(id);
    if (index >= 0)
        return d->m_modes.at(index);
    return 0;
}
//根据id 设置Tab栏当前活跃菜单项(也就是设置当前模式)
void ModeManager::slotActivateMode(int id)
{
    m_instance->activateMode(Id::fromUniqueIdentifier(id)); //设置当前模式
    ICore::raiseWindow(d->m_modeStack); //窗口上升到最顶层
}
//根据id 设置Tab栏当前活跃菜单项(也就是设置当前模式)
void ModeManager::activateMode(Id id)
{
    const int index = indexOf(id);
    if (index >= 0)
        d->m_modeStack->setCurrentIndex(index);
}

//向管理器添加模式
void ModeManager::objectAdded(QObject *obj)
{
    IMode *mode = Aggregation::query<IMode>(obj);
    if (!mode) //没有注册的模式不允许添加
        return;

    d->m_mainWindow->addContextObject(mode);

    // Count the number of modes with a higher priority 查找比当前模式优先级高的模式个数
    int index = 0;
    foreach (const IMode *m, d->m_modes)
        if (m->priority() > mode->priority())
            ++index;

    d->m_modes.insert(index, mode);//插入到模式表
    d->m_modeStack->insertTab(index, mode->widget(), mode->icon(), mode->displayName()); //模式Tab菜单项目以及模式主窗体插入
    d->m_modeStack->setTabEnabled(index, mode->isEnabled()); //设置模式 状态 启用or禁止

    // Register mode shortcut
    const Id shortcutId = mode->id().withPrefix("QtCreator.Mode.");//将当前模式id指代的字符串添加前缀 后重新生成模式快捷键的id
    QShortcut *shortcut = new QShortcut(d->m_mainWindow);//创建快捷键
    shortcut->setWhatsThis(tr("Switch to <b>%1</b> mode").arg(mode->displayName())); //设置快捷键提示信息
    Command *cmd = ActionManager::registerShortcut(shortcut, shortcutId, Context(Constants::C_GLOBAL)); //注册快捷键

    d->m_modeShortcuts.insert(index, cmd); //插入快捷方式表

    connect(cmd, SIGNAL(keySequenceChanged()), m_instance, SLOT(updateModeToolTip())); //绑定快捷方式改变处理函数

    //给没有设置快捷按键的快捷方式设置快捷按键
    for (int i = 0; i < d->m_modeShortcuts.size(); ++i) {
        Command *currentCmd = d->m_modeShortcuts.at(i);
        // we need this hack with currentlyHasDefaultSequence
        // because we call setDefaultShortcut multiple times on the same cmd
        // and still expect the current shortcut to change with it
        bool currentlyHasDefaultSequence = (currentCmd->keySequence() == currentCmd->defaultKeySequence()); //判断当前快捷方式是否有默认按键

        //为快捷方式设置默认快捷按键
        currentCmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? QString::fromLatin1("Meta+%1").arg(i+1) : QString::fromLatin1("Ctrl+%1").arg(i+1))); 
        if (currentlyHasDefaultSequence) //还原已经设置快捷按键的快捷方式
            currentCmd->setKeySequence(currentCmd->defaultKeySequence());
    }

	//快捷方式绑定到 maper  然后 再由maper中转到 激活模式窗口函数 从而实现快捷键切换模式
    d->m_signalMapper->setMapping(shortcut, mode->id().uniqueIdentifier());
    connect(shortcut, SIGNAL(activated()), d->m_signalMapper, SLOT(map()));

    connect(mode, SIGNAL(enabledStateChanged(bool)),m_instance, SLOT(enabledStateChanged())); //模式启用消息处理
}

//模式快捷方式改变消息处理
void ModeManager::updateModeToolTip()
{
    Command *cmd = qobject_cast<Command *>(sender());
    if (cmd) {
        int index = d->m_modeShortcuts.indexOf(cmd);
        if (index != -1) //获取模式索引
            d->m_modeStack->setTabToolTip(index, cmd->stringWithAppendedShortcut(cmd->shortcut()->whatsThis())); //设置模式对应 左侧菜单栏按钮的消息提示为新设置的快捷键提示
    }
}

//处理模式启用消息
void ModeManager::enabledStateChanged()
{
    IMode *mode = qobject_cast<IMode *>(sender()); //获取启用的模式
    QTC_ASSERT(mode, return);
    int index = d->m_modes.indexOf(mode);
    QTC_ASSERT(index >= 0, return);
    d->m_modeStack->setTabEnabled(index, mode->isEnabled()); //设置Tab栏启用

    // Make sure we leave any disabled mode to prevent possible crashes:
    if (mode == currentMode() && !mode->isEnabled()) { // 如果模式是当前模式 并且没有禁用则只需要激活
        // This assumes that there is always at least one enabled mode.
        for (int i = 0; i < d->m_modes.count(); ++i) {
            if (d->m_modes.at(i) != mode && d->m_modes.at(i)->isEnabled()) { //除了当前模式 还有其他模式没有禁用
                //激活当前模式 设置当前模式
                activateMode(d->m_modes.at(i)->id());
                break;
            }
        }
    }
}

//PluginManger initializationDone 信号处理
void ModeManager::handleStartup()
{ d->m_saveSettingsOnModeChange = true; }
 
//ICore的coreAboutToClose 信号处理
void ModeManager::handleShutdown()
{ d->m_saveSettingsOnModeChange = false; }

//移除模式 1.模式表移除 2.快捷按键表移除 3.Tab菜单栏移除 4.主窗体移除模式句柄
void ModeManager::aboutToRemoveObject(QObject *obj)
{
    IMode *mode = Aggregation::query<IMode>(obj); //未注册直接返回
    if (!mode)
        return;

    const int index = d->m_modes.indexOf(mode);
    d->m_modes.remove(index); //模式表移除
    d->m_modeShortcuts.remove(index); //快捷按键表移除
    d->m_modeStack->removeTab(index); //Tab菜单栏移除

    d->m_mainWindow->removeContextObject(mode); //主窗体移除模式句柄
}

//根据优先级像Corner首项插入操作
void ModeManager::addAction(QAction *action, int priority)
{
    d->m_actions.insert(action, priority); //插入操作项映射表

    // Count the number of commands with a higher priority 计算大于当前优先级的操作项数
    int index = 0;
    foreach (int p, d->m_actions) {
        if (p > priority)
            ++index;
    }

    //将本操作按照优先级插入Corner中的actionBar中
    d->m_actionBar->insertAction(index, action);
}

//插入最大优先级的操作项目 （项目选择器）
void ModeManager::addProjectSelector(QAction *action)
{
    d->m_actionBar->addProjectSelector(action);
    d->m_actions.insert(0, INT_MAX);
}

//模型将要切换消息处理
void ModeManager::currentTabAboutToChange(int index)
{
    if (index >= 0) {
        IMode *mode = d->m_modes.at(index); //取得模式
        if (mode) {
            if (d->m_saveSettingsOnModeChange) //如果模型切换需要保存设置信息 则通知核心插件保存设置
                ICore::saveSettings();
            emit currentModeAboutToChange(mode);
        }
    }
}

//模式选择栏改变模式选择消息处理
void ModeManager::currentTabChanged(int index)
{
    // Tab index changes to -1 when there is no tab left.
    if (index >= 0) {
        IMode *mode = d->m_modes.at(index); //取得新改变的模式 也就是当前展示模式

        // FIXME: This hardcoded context update is required for the Debug and Edit modes, since
        // they use the editor widget, which is already a context widget so the main window won't
        // go further up the parent tree to find the mode context.
        ICore::updateAdditionalContexts(d->m_addedContexts, mode->context()); //切换链
        d->m_addedContexts = mode->context(); //更新当前正在展示模式句柄

        IMode *oldMode = 0;
        if (d->m_oldCurrent >= 0)
            oldMode = d->m_modes.at(d->m_oldCurrent); //获取旧模式
        d->m_oldCurrent = index; //更新当前展示模式的索引信息
        emit currentModeChanged(mode, oldMode); //发送模型转换消息 参数为  新切换模式 和 旧模式
    }
}

//设置焦点到当前模式窗口 会让当前窗口在最顶层显示
void ModeManager::setFocusToCurrentMode()
{
    IMode *mode = currentMode(); //获取当前的 模式
    QTC_ASSERT(mode, return);
    QWidget *widget = mode->widget(); //拿到模式对应窗体 并设置为交点窗口
    if (widget) {
        QWidget *focusWidget = widget->focusWidget();
        if (!focusWidget)
            focusWidget = widget;
        focusWidget->setFocus();
        ICore::raiseWindow(focusWidget); //让窗口在系统顶层显示
    } 
}

//设置 模型选择栏 左侧导航  是否显示
void ModeManager::setModeSelectorVisible(bool visible)
{
    d->m_modeSelectorVisible = visible;
    d->m_modeStack->setSelectionWidgetVisible(visible);
}

//获取 模型选择栏 左侧导航  是否显示
bool ModeManager::isModeSelectorVisible()
{
    return d->m_modeSelectorVisible;
}

//获取管理器实例
QObject *ModeManager::instance()
{
    return m_instance;
}

} // namespace Core
