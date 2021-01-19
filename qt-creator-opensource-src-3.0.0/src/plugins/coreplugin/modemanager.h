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

#ifndef MODEMANAGER_H
#define MODEMANAGER_H

#include <coreplugin/core_global.h>
#include <coreplugin/id.h>
#include <QObject>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Core {

class IMode;

namespace Internal {
    class MainWindow;
    class FancyTabWidget;
}
//ModeManager提供了FancyTabWidget和Mode之间的通信桥梁
class CORE_EXPORT ModeManager : public QObject
{
    Q_OBJECT

public:
    //获取管理器实例
    static QObject *instance();

	//获取当前的 模式 tab栏当前选择{欢迎界面 还是 编辑界面 还是 ...}
    static IMode *currentMode();

	//根据模式id获取模式 {本类的静态数据类内部持有一个模式表 List<IMode>}
    static IMode *mode(Id id);
    //根据优先级像Corner首项插入操作
    static void addAction(QAction *action, int priority);

    //插入最大优先级的操作项目 （项目选择器）
    static void addProjectSelector(QAction *action);

	//给Corner 增加一个项
    static void addWidget(QWidget *widget);

	//根据id 设置Tab栏当前活跃菜单项(也就是设置当前模式)
    static void activateMode(Id id);

    //设置焦点到当前模式窗口 会让当前窗口在最顶层显示
    static void setFocusToCurrentMode();

    //获取 模型选择栏 左侧导航  是否显示
    static bool isModeSelectorVisible();

public slots:
//设置 模型选择栏 左侧导航  是否显示
    static void setModeSelectorVisible(bool visible);

signals:
    void currentModeAboutToChange(Core::IMode *mode);

    // the default argument '=0' is important for connects without the oldMode argument.
    void currentModeChanged(Core::IMode *mode, Core::IMode *oldMode = 0);

private slots:
//根据id 设置Tab栏当前活跃菜单项(也就是设置当前模式) 会raise窗口
    void slotActivateMode(int id);

    //向管理器添加模式
    void objectAdded(QObject *obj);

    //移除模式 1.模式表移除 2.快捷按键表移除 3.Tab菜单栏移除 4.主窗体移除模式句柄
    void aboutToRemoveObject(QObject *obj);


    void currentTabAboutToChange(int index);
    void currentTabChanged(int index);

    //模式快捷方式改变消息处理
    void updateModeToolTip();

    //处理模式启用消息
    void enabledStateChanged();

    //PluginManger initializationDone 信号处理
    void handleStartup();

    //ICore的coreAboutToClose 信号处理
    void handleShutdown();

private:
	//构造 需要主窗体 以及 主窗体中的内容  (FancyTabWidget包含了左侧导航以及主窗体内容)
    explicit ModeManager(Internal::MainWindow *mainWindow, Internal::FancyTabWidget *modeStack);
    ~ModeManager();

	//初始化 绑定添加 移除model事件
    static void init();

    friend class Core::Internal::MainWindow;
};

} // namespace Core

#endif // MODEMANAGER_H
