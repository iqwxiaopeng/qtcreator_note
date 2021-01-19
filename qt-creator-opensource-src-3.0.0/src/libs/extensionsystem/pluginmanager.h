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

#ifndef EXTENSIONSYSTEM_PLUGINMANAGER_H
#define EXTENSIONSYSTEM_PLUGINMANAGER_H

#include "extensionsystem_global.h"
#include <aggregation/aggregate.h>

#include <QObject>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QTextStream;
class QSettings;
QT_END_NAMESPACE

namespace ExtensionSystem {
class PluginCollection;
class IPlugin;
class PluginSpec;

namespace Internal {
class PluginManagerPrivate; //具体的管理器实现类
}

class EXTENSIONSYSTEM_EXPORT PluginManager : public QObject
{
    Q_OBJECT

public:
	//管理器单例
    static PluginManager *instance();

	//赋值给单例子,并构造具体的管理器实现类给d成员
    PluginManager();
    ~PluginManager();

    //====================================== Object pool operations 对象池操作方法==========================
	//将对象添加到对象池中，以便可以按类型从池中再次检索该对象。
    //插件管理器不做任何内存管理-添加的对象必须从池中删除，并由负责该对象的人手动删除。发出objectadded（）信号

    static void addObject(QObject *obj);
    static void removeObject(QObject *obj);
	static QList<QObject *> allObjects(); //检索池中所有对象的列表（不筛选）。通常，客户端不需要调用此函数
    static QReadWriteLock *listLock();
    template <typename T> static QList<T *> getObjects() //获取所有指定类型的对象
    {
        QReadLocker lock(listLock());
        QList<T *> results;
        QList<QObject *> all = allObjects();
        QList<T *> result;
        foreach (QObject *obj, all) {
            result = Aggregation::query_all<T>(obj); //如果对象是聚合对象，则把其聚合体的其他对象看成整体，(只要发现了胳膊 就认为脑袋，腿等也在里面)
            if (!result.isEmpty())
                results += result;
        }
        return results;
    }
    template <typename T> static T *getObject() //获取一个指定类型的对象
    {
        QReadLocker lock(listLock());
        QList<QObject *> all = allObjects();
        T *result = 0;
        foreach (QObject *obj, all) {
            if ((result = Aggregation::query<T>(obj)) != 0)
                break;
        }
        return result;
    }

	//根据对象名 获取管理器所管理对象中的一个
    static QObject *getObjectByName(const QString &name);
	//根据类名 获取管理器中继承于此类或者此类对象中的一个
    static QObject *getObjectByClassName(const QString &className);
	//====================================== Object pool operations 对象池操作方法Ending==========================



    //+++++++++++++++++ Plugin operations 插件相关方法+++++++++++++++++++++++++++++++++++
	static QList<PluginSpec *> loadQueue(); //按加载顺序返回插件列表。 （因为有依赖关系所以插件加载需要自底向上，因此需要求这个顺序）

	//加载总入口: 尝试加载以前在设置插件搜索路径时找到的所有插件。插件的插件描述对象可用于检索其对应插件的错误信息和状态信息。
    static void loadPlugins();

	//获取插件管理器对插件的搜索路径列表(插件不止存在于一个路径)
    static QStringList pluginPaths();

	//设置插件搜索路径列表，所有表中的路径会被管理器递归的检索插件的xml配置文件
    static void setPluginPaths(const QStringList &paths); //设置插件目录 会去read目录加载插件忽略项,强制加载项

	//在插件搜索路径中找到的所有插件的描述信息表,如果插件以及被创建，表中的描述对象会指向那个创建插件的实例，并且显示当前插件的最新状态
    static QList<PluginSpec *> plugins();

	//获取插件归类表 (因为插件非常多，所以进行了归类 eg: 肉类插件有 牛肉 羊肉； 蛋类插件有 鸡蛋 鸭蛋等；)每种插件有个大类别
    static QHash<QString, PluginCollection *> pluginCollections();

	//设置插件描述文件的后缀名  默认是 .xml
    static void setFileExtension(const QString &extension);

	//获取插件描述文件的后缀名 默认 .xml
    static QString fileExtension();

	//获取插件加载过程是否出错（在loadPlugins调用后调用,如果有任意插件加载过程出现错误,都会反悔错误）
    static bool hasError();
	//+++++++++++++++++ Plugin operations 插件相关方法Ending+++++++++++++++++++++++++++++++++++


    // -----------------------------Settings 插件管理器相关配置项目----------------------------
	//注意软件产品通常会有默认设置，但是如果用户做了设置，那么就按用户设置走 因此，有了用户设置与全局设置这2方面的设置
	//

	//定义用于有关已启用和已禁用插件的信息的用户特定设置。
    static void setSettings(QSettings *settings); //设置普通配置对象 就是用户的配置文件

	//返回用于有关已启用和已禁用插件的信息的用户特定设置。
    static QSettings *settings();

	//定义用于有关默认禁用插件的信息的全局（与用户无关）设置。
    static void setGlobalSettings(QSettings *settings);//设置全局配置对象 就是全局有效的配置文件

	//返回用于有关默认禁用插件的信息的全局（与用户无关）设置。
    static QSettings *globalSettings();

	//将用户对于默认配置的修改写入用户特定设置中。(比如用户在qtcreator界面选择了启用某插件，不启用这些就会写入，其实就是同步插件描述对象的相关状态到配置)
    static void writeSettings();
	// -----------------------------Settings 插件管理器相关配置项目Ending----------------------------


    //+++++++++++++++++++++++++++++++++ command line arguments+++++++++++++++++++++++++++++++++++

	//解析后留下的参数（既不是启动参数也不是插件参数）。通常，这是要打开的文件列表。调试发现就是命令行参数，比如我通过命令行改变qtcreator颜色设置为红色
    static QStringList arguments();

	/*
	获取参数args中的命令行选项列表并对其进行分析。
    插件管理器本身可以直接处理一些选项本身（- noload<plugin>），并将插件注册的选项添加到插件描述中。
	调用者（应用程序）可以通过参数appoptions列表为选项注册自己，该列表包含成对的“选项字符串”和一个bool，指示选项是否需要参数。
    应用程序选项总是覆盖任何插件的选项。
    对于找到的任何应用程序选项，参数foundappoptions都设置为对（“option string”，“argument”）。
    无法处理的命令行选项可以通过arguments（）函数检索。
    如果发生错误（例如缺少需要一个选项的参数），则参数errorstring包含该错误的描述性消息

	注： 以上大意就是提供插件注册自身命令行指令的功能，例如 -color red 这种命令给我这个插件aaa处理等，aaa注册了-color命令要求带参数颜色
	*/
    static bool parseOptions(const QStringList &args,
        const QMap<QString, bool> &appOptions,
        QMap<QString, QString> *foundAppOptions,
        QString *errorString);

	//为命令行帮助格式化插件管理器的启动选项。 规范化命令行 -help输出
    static void formatOptions(QTextStream &str, int optionIndentation, int descriptionIndentation);

	//为命令行帮助格式化插件描述的插件选项。规范化cmd 插件描述的输出
    static void formatPluginOptions(QTextStream &str, int optionIndentation, int descriptionIndentation);

	//为命令行帮助格式化插件规范的版本。 规范化 cmd 插件描述文件版本的输出
    static void formatPluginVersions(QTextStream &str);

	//序列化插件选项和参数，以便通过qtsingleapplication发送单个字符串：“：myplugin - option1 - option2：参数参数1参数2”，
	//以带冒号的关键字开头的列表。参数在最后。
    static QString serializedArguments();

	//测试运行 应该是用例吧 暂时这样写
    static bool testRunRequested();
	//测试运行时 数据存放的目录 避免打扰软件正常运行数据
    static QString testDataDirectory();

	//创建一个分析条目，显示激活分析时经过的时间。 暂时这样写
    static void profilingReport(const char *what, const PluginSpec *spec = 0);

	//返回程序所运行的系统 win？mac？Linux之类
    static QString platformName();

signals:
    void objectAdded(QObject *obj);
    void aboutToRemoveObject(QObject *obj);

    void pluginsChanged(); //函数内部就一个 QMetaObject::activate(this, &staticMetaObject, 2, nullptr); 1发射信号对象 2发射信号对象的元对象 3信号相对索引 4入参和返回值
    
    void initializationDone();

public slots:
    void remoteArguments(const QString &serializedArguments, QObject *socket); //socket收到数据调用
    void shutdown(); //关闭信号调用

private slots:
    void startTests();
    friend class Internal::PluginManagerPrivate; //内部实现体给有缘了，让他可访问本类私有方法
};

} // namespace ExtensionSystem

#endif // EXTENSIONSYSTEM_PLUGINMANAGER_H
