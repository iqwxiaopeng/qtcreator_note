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

#ifndef IPLUGIN_H
#define IPLUGIN_H

#include "extensionsystem_global.h"

#include <QObject>
#if QT_VERSION >= 0x050000
#    include <QtPlugin>
#endif

namespace ExtensionSystem {

namespace Internal {
    class IPluginPrivate;
    class PluginSpecPrivate;
}

class PluginManager;
class PluginSpec;
//一个插件由2部分组成 : 描述文件 和 一个实现了IPlugin接口的实例 本借口抽象了插件的生命流程
/*插件接口类*/
class EXTENSIONSYSTEM_EXPORT IPlugin : public QObject
{
    Q_OBJECT

public:
    enum ShutdownFlag {
        SynchronousShutdown, //需要等待其他依赖插件关闭才能关闭自己 暂时这么认为
        AsynchronousShutdown //无需等待依赖关闭
    };

    IPlugin();
	//析构函数会释放掉 本插件注册的自动释放对象
    virtual ~IPlugin();

    //初始化函数,在插件被加载时会调用(根据依赖关系从底向上初始化构建基本框架)
    virtual bool initialize(const QStringList &arguments, QString *errorString) = 0;

    //在所有插件的initialize函数被调用后,调用该函数(这个时候插件已经初始化完成了可以工作，此时去创建一些共享对象注册给其他插件用)
    virtual void extensionsInitialized() = 0;

    //在所有插件的extensionsInitialized函数调用完成以后进行调用(此时可以让插件做一些程序启动前的准备工作)
    virtual bool delayedInitialize() { return false; }

    //关闭方式 同步关闭 还是异步关闭
    virtual ShutdownFlag aboutToShutdown() { return SynchronousShutdown; }

	//当已有qtcreator运行时候，新启动一个实例时使用 - client参数执行时，将在正在运行的实例中调用插件的此函数。
	//插件特定的参数在一个选项中传递，而其余的参数在一个参数中传递。
	//如果使用 - block，则返回阻止命令直到命令被销毁的qobject。
    virtual QObject *remoteCommand(const QStringList & /* options */, const QStringList & /* arguments */) { return 0; }

	//返回与此插件对应的PlugInspect 插件描述文件
    PluginSpec *pluginSpec() const;

	//在插件管理器中注册obj的便利功能
    void addObject(QObject *obj);

	//在插件管理器中注册自释放obj的便利功能 当IPlugin实例被销毁时，通过addautoreleasedObject添加到池中的对象将按注册的相反顺序被自动删除。
    void addAutoReleasedObject(QObject *obj);

	//从插件管理器注销对象的便利函数
    void removeObject(QObject *obj);

signals:
    void asynchronousShutdownFinished(); //插件异步关闭结束回调

private:
    Internal::IPluginPrivate *d; //插件数据类,放到一个数据指针对象是为了兼容二进制，就是修改后lib不用更新只需要更新dll，因为数据对象中添加数据不会改变头文件

    friend class Internal::PluginSpecPrivate;
};

} // namespace ExtensionSystem

// The macros Q_EXPORT_PLUGIN, Q_EXPORT_PLUGIN2 become obsolete in Qt 5.
#if QT_VERSION >= 0x050000
#    if defined(Q_EXPORT_PLUGIN)
#        undef Q_EXPORT_PLUGIN
#        undef Q_EXPORT_PLUGIN2
#    endif
#    define Q_EXPORT_PLUGIN(plugin)
#    define Q_EXPORT_PLUGIN2(function, plugin)
#else
#    define Q_PLUGIN_METADATA(x)
#endif

#endif // IPLUGIN_H
