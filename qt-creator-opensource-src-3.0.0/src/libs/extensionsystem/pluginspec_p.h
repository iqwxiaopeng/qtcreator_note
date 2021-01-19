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

#ifndef PLUGINSPEC_P_H
#define PLUGINSPEC_P_H

#include "pluginspec.h"
#include "iplugin.h"

#include <QObject>
#include <QStringList>
#include <QXmlStreamReader>
#include <QRegExp>

namespace ExtensionSystem {

class IPlugin;
class PluginManager;

namespace Internal {

    /*插件描述类 内部实现体
    */
class EXTENSIONSYSTEM_EXPORT PluginSpecPrivate : public QObject
{
    Q_OBJECT

public:
    PluginSpecPrivate(PluginSpec *spec);

    //1. 读取插件描述文件中的信息用来实例化本插件描述类
    //2. 更新插件描述类状态为已经读
    bool read(const QString &fileName);//读取一个插件描述文件
    bool provides(const QString &pluginName, const QString &version) const; //判断本插件是否可替代名为pluginName版本为pluginVersion的插件

    //函数的参数specs是在设置目录中所搜到的所有插件描述对象
    bool resolveDependencies(const QList<PluginSpec *> &specs);//检查依赖插件是否在插件集合specs并根据加载依赖强弱关系 生成插件映射表dependencySpecs
    bool loadLibrary(); //加载插件所指代的动态库 并创建插件实例
    bool initializePlugin(); //初始化插件 调用插件的initialize方法
    bool initializeExtensions();//对其他子项目进行初始化
    bool delayedInitialize(); //调用插件的延迟初始化 delayedInitialize
    IPlugin::ShutdownFlag stop();
    void kill();

    QString name; //插件名
    QString version;//插件版本
    QString compatVersion;//插件兼容版本
    bool experimental;//是否测试版本
    bool disabledByDefault;//插件默认不启用标记
    QString vendor;//插件提供方
    QString copyright;//插件的版权
    QString license;//插件的授权协议
    QString description;//插件的具体描述
    QString url;//插件的url地址
    QString category;//插件所属的类别
    QRegExp platformSpecification; //存储插件平台描述正则
    QList<PluginDependency> dependencies; //插件的依赖表 (一个插件可能依赖多个其他插件)
    bool enabledInSettings;//启用设置标记 与 不可用标记相反
    bool disabledIndirectly; //间接启用标记 为真则不会在程序启动时加载
    bool forceEnabled; //强制启动加载标记
    bool forceDisabled; //强制启动不加载

    QString location; //插件描述文件所在的绝对目录
    QString filePath; //插件描述文件的绝对位置
    QStringList arguments; //插件的命令行参数 从命令传入的

    QHash<PluginDependency, PluginSpec *> dependencySpecs; //依赖插件与具体插件对应关系表 PluginDependency插件粗略信息 PluginSpec插件具体信息
    PluginSpec::PluginArgumentDescriptions argumentDescriptions;//插件描述文件中的参数集
    IPlugin *plugin; //指向加载的插件对象 **这个是具体的插件对象

    PluginSpec::State state; //插件描述文件当前的状态 EG:已被读取状态 PluginSpec::Read;
    bool hasError; //插件解析过程错误记录 出错标志
    QString errorString; //错误信息记录

    //判断是否支持该版本 内部调用 versionRegExp函数判断
    static bool isValidVersion(const QString &version);
    static int versionCompare(const QString &version1, const QString &version2); // 版本比较 

    //设置依赖插件的间接启用标记 为真 则意味着插件在启动时不会被加载
    void disableIndirectlyIfDependencyDisabled();


private:
    PluginSpec *q; //对象所指代的对外接口的指针(接口通过d指针指向本类，本类通过q指针指向对外接口对象)

    bool reportError(const QString &err);
    void readPluginSpec(QXmlStreamReader &reader);//解析xml格式的插件描述数据
    void readDependencies(QXmlStreamReader &reader); //从xml节点读取插件依赖
    void readDependencyEntry(QXmlStreamReader &reader);//读取一条依赖信息并存入依赖表
    void readArgumentDescriptions(QXmlStreamReader &reader); // 解析插件参数
    void readArgumentDescription(QXmlStreamReader &reader); //读取一个参数信息并存入参数集
    bool readBooleanValue(QXmlStreamReader &reader, const char *key);//读取key 条目bool值

    // 当前支持插件版本的正则匹配
    static QRegExp &versionRegExp();
};

} // namespace Internal
} // namespace ExtensionSystem

#endif // PLUGINSPEC_P_H
