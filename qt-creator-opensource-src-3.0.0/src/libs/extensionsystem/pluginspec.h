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

#ifndef PLUGINSPEC_H
#define PLUGINSPEC_H

#include "extensionsystem_global.h"

#include <QString>
#include <QList>
#include <QHash>

QT_BEGIN_NAMESPACE
class QStringList;
class QRegExp;
QT_END_NAMESPACE

namespace ExtensionSystem {

namespace Internal { //内部使用的,接口看不到这个实现,其他插件也最好不使用这个命名空间里面的类
    class PluginSpecPrivate;
    class PluginManagerPrivate;
}

class IPlugin;

//插件描述文件中 依赖条目 结构定义
struct EXTENSIONSYSTEM_EXPORT PluginDependency
{
    //依赖类型定义 必须 或者 可选
    enum Type {
        Required,
        Optional
    };

    PluginDependency() : type(Required) {}

    QString name; //所依赖的插件名
    QString version; //所依赖的插件版本
    Type type; //依赖关系是必须还是可选
    bool operator==(const PluginDependency &other) const;
};

//根据插件的名称生成一个哈希值
uint qHash(const ExtensionSystem::PluginDependency &value);

//插件描述文件中 插件参数类定义(参数名 参数值 参数描述)
struct EXTENSIONSYSTEM_EXPORT PluginArgumentDescription
{
    QString name;
    QString parameter;
    QString description;
};

/*
插件描述类
*/
class EXTENSIONSYSTEM_EXPORT PluginSpec
{
public:
	//不可用  正在读中 依赖关系OK  已经加载  以及初始化 以及运行  被关闭  被删除
    enum State { Invalid, Read, Resolved, Loaded, Initialized, Running, Stopped, Deleted}; //插件的状态枚举

    ~PluginSpec();

    // information from the xml file, valid after 'Read' state is reached
    QString name() const; //插件名
    QString version() const; //插件版本
    QString compatVersion() const; //兼容版本
    QString vendor() const; //
    QString copyright() const; //版权
    QString license() const; //授权协议
    QString description() const; //插件描述信息
    QString url() const; //插件url
    QString category() const; //插件类别
    QRegExp platformSpecification() const; //插件平台描述信息
    bool isExperimental() const; //是否测试版
    bool isDisabledByDefault() const; //默认开启与否
    bool isEnabledInSettings() const; //配置是否设置启用
    bool isEffectivelyEnabled() const;//插件是否要在启动时就加载
    bool isDisabledIndirectly() const;
    bool isForceEnabled() const; //是否强制启用
    bool isForceDisabled() const;
    QList<PluginDependency> dependencies() const; //插件依赖列表

    typedef QList<PluginArgumentDescription> PluginArgumentDescriptions; //插件参数表
    PluginArgumentDescriptions argumentDescriptions() const; //参数列表

    // other information, valid after 'Read' state is reached
    QString location() const; //位置信息
    QString filePath() const; //文件目录信息

    void setEnabled(bool value);
    void setDisabledByDefault(bool value);
    void setDisabledIndirectly(bool value);
    void setForceEnabled(bool value);
    void setForceDisabled(bool value);

	//获取插件的命令行参数
    QStringList arguments() const;
	//设置特定于插件的命令行参数
    void setArguments(const QStringList &arguments);
    void addArgument(const QString &argument);

    bool provides(const QString &pluginName, const QString &version) const;

    // dependency specs, valid after 'Resolved' state is reached
    QHash<PluginDependency, PluginSpec *> dependencySpecs() const;

    // linked plugin instance, valid after 'Loaded' state is reached
    IPlugin *plugin() const; //描述对应的插件

    // state 状态相关方法
    State state() const; //当前状态
    bool hasError() const; //出错与否
    QString errorString() const; //错误信息

private:
    PluginSpec();

    Internal::PluginSpecPrivate *d; //具体的插件数据以及内部方法,d指针是为了二进制兼容
    friend class Internal::PluginManagerPrivate;
    friend class Internal::PluginSpecPrivate; //插件描述信息构造私有，意味着不能凭空产生，只能是由管理器类生成
};

} // namespace ExtensionSystem

#endif // PLUGINSPEC_H

