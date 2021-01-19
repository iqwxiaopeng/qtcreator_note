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

#ifndef PLUGINMANAGER_P_H
#define PLUGINMANAGER_P_H

#include "pluginspec.h"

#include <QSet>
#include <QStringList>
#include <QObject>
#include <QScopedPointer>
#include <QReadWriteLock>

QT_BEGIN_NAMESPACE
class QTime;
class QTimer;
class QSettings;
class QEventLoop;
QT_END_NAMESPACE

namespace ExtensionSystem {

class PluginManager;
class PluginCollection;

namespace Internal {

class PluginSpecPrivate;

class EXTENSIONSYSTEM_EXPORT PluginManagerPrivate : QObject
{
    Q_OBJECT
public:
    PluginManagerPrivate(PluginManager *pluginManager);
    virtual ~PluginManagerPrivate();

    // Object pool operations
    void addObject(QObject *obj);
    void removeObject(QObject *obj);

    // Plugin operations
    void loadPlugins();
    void shutdown();
    void setPluginPaths(const QStringList &paths); //设置插件描述文件的目录 会去读取插件描述信息,创建描述文件表
    QList<PluginSpec *> loadQueue(); //获得当前插件的顺序非循环依赖 循环依赖项会被移除
    void loadPlugin(PluginSpec *spec, PluginSpec::State destState);//加载插件 和初始化插件 通过destState来指定
    void resolveDependencies();//处理依赖的插件 生成信息映射表 更新间接依赖标记
    void initProfiling();
    void profilingSummary() const;
    void profilingReport(const char *what, const PluginSpec *spec = 0);
    void setSettings(QSettings *settings);
    void setGlobalSettings(QSettings *settings);
    void readSettings(); //读取插件相关的忽略加载项和强制加载项
    void writeSettings();
    void disablePluginIndirectly(PluginSpec *spec);

    class TestSpec {
    public:
        TestSpec(PluginSpec *pluginSpec, const QStringList &testFunctions = QStringList())
            : pluginSpec(pluginSpec), testFunctions(testFunctions) {}
        PluginSpec *pluginSpec;
        QStringList testFunctions;
    };

    bool containsTestSpec(PluginSpec *pluginSpec) const
    {
        foreach (const TestSpec &testSpec, testSpecs) {
            if (testSpec.pluginSpec == pluginSpec)
                return true;
        }
        return false;
    }

    QHash<QString, PluginCollection *> pluginCategories; //插件种类表 每一类插件描述又是集合
    QList<PluginSpec *> pluginSpecs; //路径下所有的插件描述文件对应的描述类表
    QList<TestSpec> testSpecs;
    QStringList pluginPaths;//插件所在目录
    QString extension;//插件扩展名
    QList<QObject *> allObjects; // 对象池子，插件们暴露给其他插件使用的对象扔在这个里面
    QStringList defaultDisabledPlugins; // Plugins/Ignored from install settings  全局配置默认的插件忽略项
    QStringList defaultEnabledPlugins; // Plugins/ForceEnabled from install settings 全局配置默认的插件强制启用项
    QStringList disabledPlugins; //用户配置忽略插件项
    QStringList forceEnabledPlugins; //用户配置强制抵用的插件项目
    // delayed initialization
    QTimer *delayedInitializeTimer; //延时初始化定时器 如果延迟队列有插件需要初始化延时则会设置这个定时器
    QList<PluginSpec *> delayedInitializeQueue; //延迟初始化队列 实际字下而上运行插件并放入
    // ansynchronous shutdown
    QList<PluginSpec *> asynchronousPlugins; // plugins that have requested async shutdown
    QEventLoop *shutdownEventLoop; // used for async shutdown

    QStringList arguments; //命令行参数
    QScopedPointer<QTime> m_profileTimer;
    QHash<const PluginSpec *, int> m_profileTotal;
    int m_profileElapsedMS;
    unsigned m_profilingVerbosity;
    QSettings *settings; //用户配置文件
    QSettings *globalSettings; //全局配置文件

    // Look in argument descriptions of the specs for the option.
    PluginSpec *pluginForOption(const QString &option, bool *requiresArgument) const;
    PluginSpec *pluginByName(const QString &name) const;

    // used by tests
    static PluginSpec *createSpec();
    static PluginSpecPrivate *privateSpec(PluginSpec *spec);

    mutable QReadWriteLock m_lock;

private slots:
    void nextDelayedInitialize();
    void asyncShutdownFinished();

private:
    PluginCollection *defaultCollection; //默认的插件描述文件集合
    PluginManager *q; //指向他所指代的管理器对象。这两个是相互指向

    //1. 在插件目录搜索插件描述文件
    //2. 填充与插件相关的集合，获取依赖
    //3. 更新插件的依赖关系
    void readPluginPaths();

    //函数判断插件是否出现循环依赖
    //spec 当前检测的插件
    // queue 记录依赖关系检测OK的插件(检测OK只是说明没发现循环依赖而已, 而返回值是说依赖有没有问题, 除了循环依赖还可能依赖没找到或者版本不匹配)
    // circularityCheckQueue 记录处于检测中的插件和已发现的依赖OK插件因为不OK的会清除掉(一个插件处于检测中说明其要么被依赖要么是正在排查他的依赖问题, 如果这个时候有插件依赖这些插件，说明出现循环依赖)
    bool loadQueue(PluginSpec *spec,
            QList<PluginSpec *> &queue,
            QList<PluginSpec *> &circularityCheckQueue);
    void stopAll();
    void deleteAll();
};

} // namespace Internal
} // namespace ExtensionSystem

#endif // PLUGINMANAGER_P_H
