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

#include "pluginspec.h"

#include "pluginspec_p.h"
#include "iplugin.h"
#include "iplugin_p.h"
#include "pluginmanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QRegExp>
#include <QCoreApplication>
#include <QDebug>

#ifdef Q_OS_LINUX
// Using the patched version breaks on Fedora 10, KDE4.2.2/Qt4.5.
#   define USE_UNPATCHED_QPLUGINLOADER 1
#else
#   define USE_UNPATCHED_QPLUGINLOADER 1
#endif

#if USE_UNPATCHED_QPLUGINLOADER

#   include <QPluginLoader>
    typedef QT_PREPEND_NAMESPACE(QPluginLoader) PluginLoader;

#else

#   include "patchedpluginloader.cpp"
    typedef PatchedPluginLoader PluginLoader;

#endif

/*!
    \class ExtensionSystem::PluginDependency
    \brief The PluginDependency class contains the name and required compatible
    version number of a plugin's dependency.

    This reflects the data of a dependency tag in the plugin's XML description
    file. The name and version are used to resolve the dependency. That is,
    a plugin with the given name and
    plugin \c {compatibility version <= dependency version <= plugin version} is searched for.

    See also ExtensionSystem::IPlugin for more information about plugin dependencies and
    version matching.
*/

/*!
    \variable ExtensionSystem::PluginDependency::name
    String identifier of the plugin.
*/

/*!
    \variable ExtensionSystem::PluginDependency::version
    Version string that a plugin must match to fill this dependency.
*/

/*!
    \variable ExtensionSystem::PluginDependency::type
    Defines whether the dependency is required or optional.
    \sa ExtensionSystem::PluginDependency::Type
*/

/*!
    \enum ExtensionSystem::PluginDependency::Type
    Whether the dependency is required or optional.
    \value Required
           Dependency needs to be there.
    \value Optional
           Dependency is not necessarily needed. You need to make sure that
           the plugin is able to load without this dependency installed, so
           for example you may not link to the dependency's library.
*/

/*!
    \class ExtensionSystem::PluginSpec
    \brief The PluginSpec class contains the information of the plugin's XML
    description file and
    information about the plugin's current state.

    The plugin spec is also filled with more information as the plugin
    goes through its loading process (see PluginSpec::State).
    If an error occurs, the plugin spec is the place to look for the
    error details.
*/

/*!
    \enum ExtensionSystem::PluginSpec::State
    The State enum indicates the states the plugin goes through while
    it is being loaded.

    The state gives a hint on what went wrong in case of an error.

    \value  Invalid
            Starting point: Even the XML description file was not read.
    \value  Read
            The XML description file has been successfully read, and its
            information is available via the PluginSpec.
    \value  Resolved
            The dependencies given in the description file have been
            successfully found, and are available via the dependencySpecs() function.
    \value  Loaded
            The plugin's library is loaded and the plugin instance created
            (available through plugin()).
    \value  Initialized
            The plugin instance's IPlugin::initialize() function has been called
            and returned a success value.
    \value  Running
            The plugin's dependencies are successfully initialized and
            extensionsInitialized has been called. The loading process is
            complete.
    \value Stopped
            The plugin has been shut down, i.e. the plugin's IPlugin::aboutToShutdown() function has been called.
    \value Deleted
            The plugin instance has been deleted.
*/

using namespace ExtensionSystem;
using namespace ExtensionSystem::Internal;

/*!
    \internal
*/
uint ExtensionSystem::qHash(const ExtensionSystem::PluginDependency &value)
{
    return qHash(value.name);
}

/*!
    \internal
*/
bool PluginDependency::operator==(const PluginDependency &other) const
{
    return name == other.name && version == other.version && type == other.type;
}

/*!
    \internal
*/
PluginSpec::PluginSpec()
    : d(new PluginSpecPrivate(this))
{
}

/*!
    \internal
*/
PluginSpec::~PluginSpec()
{
    delete d;
    d = 0;
}

/*!
    The plugin name. This is valid after the PluginSpec::Read state is reached.
*/
QString PluginSpec::name() const
{
    return d->name;
}

/*!
    The plugin version. This is valid after the PluginSpec::Read state is reached.
*/
QString PluginSpec::version() const
{
    return d->version;
}

/*!
    The plugin compatibility version. This is valid after the PluginSpec::Read state is reached.
*/
QString PluginSpec::compatVersion() const
{
    return d->compatVersion;
}

/*!
    The plugin vendor. This is valid after the PluginSpec::Read state is reached.
*/
QString PluginSpec::vendor() const
{
    return d->vendor;
}

/*!
    The plugin copyright. This is valid after the PluginSpec::Read state is reached.
*/
QString PluginSpec::copyright() const
{
    return d->copyright;
}

/*!
    The plugin license. This is valid after the PluginSpec::Read state is reached.
*/
QString PluginSpec::license() const
{
    return d->license;
}

/*!
    The plugin description. This is valid after the PluginSpec::Read state is reached.
*/
QString PluginSpec::description() const
{
    return d->description;
}

/*!
    The plugin URL where you can find more information about the plugin.
    This is valid after the PluginSpec::Read state is reached.
*/
QString PluginSpec::url() const
{
    return d->url;
}

/*!
    The category that the plugin belongs to. Categories are groups of plugins which allow for keeping them together in the UI.
    Returns an empty string if the plugin does not belong to a category.
*/
QString PluginSpec::category() const
{
    return d->category;
}

/*!
    A QRegExp matching the platforms this plugin works on. An empty pattern implies all platforms.
    \since 3.0
*/

QRegExp PluginSpec::platformSpecification() const
{
    return d->platformSpecification;
}

/*!
    Returns whether the plugin has its experimental flag set.
*/
bool PluginSpec::isExperimental() const
{
    return d->experimental;
}

/*!
    Returns whether the plugin is disabled by default.
    This might be because the plugin is experimental, or because
    the plugin manager's settings define it as disabled by default.
*/
bool PluginSpec::isDisabledByDefault() const
{
    return d->disabledByDefault;
}

/*!
    Returns whether the plugin should be loaded at startup. True by default.

    The user can change it from the Plugin settings.

    \note This function returns true even if a plugin is disabled because its
    dependencies were not loaded, or an error occurred during loading it.
*/
bool PluginSpec::isEnabledInSettings() const
{
    return d->enabledInSettings;
}

/*!
    Returns whether the plugin is loaded at startup.
    \see PluginSpec::isEnabled
    //插件是否要在启动时就加载
*/
bool PluginSpec::isEffectivelyEnabled() const
{
    if (d->disabledIndirectly
        || (!d->enabledInSettings && !d->forceEnabled)
        || d->forceDisabled) {
        return false;
    }
    return d->platformSpecification.isEmpty() || d->platformSpecification.exactMatch(PluginManager::platformName());
}

/*!
    Returns true if loading was not done due to user unselecting this plugin or its dependencies.
*/
bool PluginSpec::isDisabledIndirectly() const
{
    return d->disabledIndirectly;
}

/*!
    Returns whether the plugin is enabled via the -load option on the command line.
*/
bool PluginSpec::isForceEnabled() const
{
    return d->forceEnabled;
}

/*!
    Returns whether the plugin is disabled via the -noload option on the command line.
*/
bool PluginSpec::isForceDisabled() const
{
    return d->forceDisabled;
}

/*!
    The plugin dependencies. This is valid after the PluginSpec::Read state is reached.
*/
QList<PluginDependency> PluginSpec::dependencies() const
{
    return d->dependencies;
}

/*!
    Returns a list of descriptions of command line arguments the plugin processes.
*/

PluginSpec::PluginArgumentDescriptions PluginSpec::argumentDescriptions() const
{
    return d->argumentDescriptions;
}

/*!
    The absolute path to the directory containing the plugin XML description file
    this PluginSpec corresponds to.
*/
QString PluginSpec::location() const
{
    return d->location;
}

/*!
    The absolute path to the plugin XML description file (including the file name)
    this PluginSpec corresponds to.
*/
QString PluginSpec::filePath() const
{
    return d->filePath;
}

/*!
    Command line arguments specific to the plugin. Set at startup.
*/

QStringList PluginSpec::arguments() const
{
    return d->arguments;
}

/*!
    Sets the command line arguments specific to the plugin to \a arguments.
*/

void PluginSpec::setArguments(const QStringList &arguments)
{
    d->arguments = arguments;
}

/*!
    Adds \a argument to the command line arguments specific to the plugin.
*/

void PluginSpec::addArgument(const QString &argument)
{
    d->arguments.push_back(argument);
}


/*!
    The state in which the plugin currently is.
    See the description of the PluginSpec::State enum for details.
*/
PluginSpec::State PluginSpec::state() const
{
    return d->state;
}

/*!
    Returns whether an error occurred while reading/starting the plugin.
*/
bool PluginSpec::hasError() const
{
    return d->hasError;
}

/*!
    Detailed, possibly multi-line, error description in case of an error.
*/
QString PluginSpec::errorString() const
{
    return d->errorString;
}

/*!
    Returns whether this plugin can be used to fill in a dependency of the given
    \a pluginName and \a version.

        \sa PluginSpec::dependencies()
*/
bool PluginSpec::provides(const QString &pluginName, const QString &version) const
{
    return d->provides(pluginName, version);
}

/*!
    The corresponding IPlugin instance, if the plugin library has already been successfully loaded,
    i.e. the PluginSpec::Loaded state is reached.
*/
IPlugin *PluginSpec::plugin() const
{
    return d->plugin;
}

/*!
    Returns the list of dependencies, already resolved to existing plugin specs.
    Valid if PluginSpec::Resolved state is reached.

    \sa PluginSpec::dependencies()
*/
QHash<PluginDependency, PluginSpec *> PluginSpec::dependencySpecs() const
{
    return d->dependencySpecs;
}

//==========PluginSpecPrivate==================

namespace {
    const char PLUGIN[] = "plugin";
    const char PLUGIN_NAME[] = "name";
    const char PLUGIN_VERSION[] = "version";
    const char PLUGIN_COMPATVERSION[] = "compatVersion";
    const char PLUGIN_EXPERIMENTAL[] = "experimental";
    const char PLUGIN_DISABLED_BY_DEFAULT[] = "disabledByDefault";
    const char VENDOR[] = "vendor";
    const char COPYRIGHT[] = "copyright";
    const char LICENSE[] = "license";
    const char DESCRIPTION[] = "description";
    const char URL[] = "url";
    const char CATEGORY[] = "category";
    const char PLATFORM[] = "platform";
    const char DEPENDENCYLIST[] = "dependencyList";
    const char DEPENDENCY[] = "dependency";
    const char DEPENDENCY_NAME[] = "name";
    const char DEPENDENCY_VERSION[] = "version";
    const char DEPENDENCY_TYPE[] = "type";
    const char DEPENDENCY_TYPE_SOFT[] = "optional";
    const char DEPENDENCY_TYPE_HARD[] = "required";
    const char ARGUMENTLIST[] = "argumentList";
    const char ARGUMENT[] = "argument";
    const char ARGUMENT_NAME[] = "name";
    const char ARGUMENT_PARAMETER[] = "parameter";
}
/*!
    \internal
*/
PluginSpecPrivate::PluginSpecPrivate(PluginSpec *spec)
    :
    experimental(false),
    disabledByDefault(false),
    enabledInSettings(true),
    disabledIndirectly(false),
    forceEnabled(false),
    forceDisabled(false),
    plugin(0),
    state(PluginSpec::Invalid),
    hasError(false),
    q(spec)
{
}

/*!
    \internal
    1. 读取插件描述文件中的信息用来实例化本插件描述类
    2. 更新插件描述类状态为已经读
*/
bool PluginSpecPrivate::read(const QString &fileName)
{
    name
        = version
        = compatVersion
        = vendor
        = copyright
        = license
        = description
        = url
        = category
        = location
        = QString();
    state = PluginSpec::Invalid;
    hasError = false;
    errorString.clear();
    dependencies.clear();
    QFile file(fileName); //只读方式打开插件描述文件
    if (!file.open(QIODevice::ReadOnly))
        return reportError(tr("Cannot open file %1 for reading: %2")
                           .arg(QDir::toNativeSeparators(file.fileName()), file.errorString()));
    QFileInfo fileInfo(file);
    location = fileInfo.absolutePath(); //插件描述文件所在的绝对目录
    filePath = fileInfo.absoluteFilePath(); //插件描述文件的绝对位置
    QXmlStreamReader reader(&file); //解析xml文本
    while (!reader.atEnd()) {
        reader.readNext();
        switch (reader.tokenType()) {
        case QXmlStreamReader::StartElement:
            readPluginSpec(reader); //具体解析插件描述文件函数
            break;
        default:
            break;
        }
    }
    if (reader.hasError())
        return reportError(tr("Error parsing file %1: %2, at line %3, column %4")
                .arg(QDir::toNativeSeparators(file.fileName()))
                .arg(reader.errorString())
                .arg(reader.lineNumber())
                .arg(reader.columnNumber()));
    state = PluginSpec::Read;
    return true;
}

void PluginSpec::setEnabled(bool value)
{
    d->enabledInSettings = value;
}

void PluginSpec::setDisabledByDefault(bool value)
{
    d->disabledByDefault = value;
}

void PluginSpec::setDisabledIndirectly(bool value)
{
    d->disabledIndirectly = value;
}

void PluginSpec::setForceEnabled(bool value)
{
    d->forceEnabled = value;
    if (value)
        d->forceDisabled = false;
}

void PluginSpec::setForceDisabled(bool value)
{
    if (value)
        d->forceEnabled = false;
    d->forceDisabled = value;
}

/*!
    \internal
*/
bool PluginSpecPrivate::reportError(const QString &err)
{
    errorString = err;
    hasError = true;
    return false;
}

static inline QString msgAttributeMissing(const char *elt, const char *attribute)
{
    return QCoreApplication::translate("PluginSpec", "'%1' misses attribute '%2'").arg(QLatin1String(elt), QLatin1String(attribute));
}

static inline QString msgInvalidFormat(const char *content)
{
    return QCoreApplication::translate("PluginSpec", "'%1' has invalid format").arg(QLatin1String(content));
}

static inline QString msgInvalidElement(const QString &name)
{
    return QCoreApplication::translate("PluginSpec", "Invalid element '%1'").arg(name);
}

static inline QString msgUnexpectedClosing(const QString &name)
{
    return QCoreApplication::translate("PluginSpec", "Unexpected closing element '%1'").arg(name);
}

static inline QString msgUnexpectedToken()
{
    return QCoreApplication::translate("PluginSpec", "Unexpected token");
}

/*!
    \internal
    解析xml格式的插件描述数据 这个主要是为了数据来源多元化数据可以从任意地方来
*/
void PluginSpecPrivate::readPluginSpec(QXmlStreamReader &reader)
{
    if (reader.name() != QLatin1String(PLUGIN)) {
        reader.raiseError(QCoreApplication::translate("PluginSpec", "Expected element '%1' as top level element")
                          .arg(QLatin1String(PLUGIN)));
        return;
    }
    //获取插件名
    name = reader.attributes().value(QLatin1String(PLUGIN_NAME)).toString();
    if (name.isEmpty()) {
        reader.raiseError(msgAttributeMissing(PLUGIN, PLUGIN_NAME));
        return;
    }
    //插件版本
    version = reader.attributes().value(QLatin1String(PLUGIN_VERSION)).toString();
    if (version.isEmpty()) {
        reader.raiseError(msgAttributeMissing(PLUGIN, PLUGIN_VERSION));
        return;
    }
    //判断是否支持该版本
    if (!isValidVersion(version)) {
        reader.raiseError(msgInvalidFormat(PLUGIN_VERSION));
        return;
    }
    //插件兼容版本
    compatVersion = reader.attributes().value(QLatin1String(PLUGIN_COMPATVERSION)).toString();
    if (!compatVersion.isEmpty() && !isValidVersion(compatVersion)) { //兼容版本配置空 或者已经不支持该兼容版本 则程序返回错误
        reader.raiseError(msgInvalidFormat(PLUGIN_COMPATVERSION));
        return;
    } else if (compatVersion.isEmpty()) { //如果兼容版本配置空 则兼容版本赋值为版本
        compatVersion = version;
    }
    //插件默认不启用标记
    disabledByDefault = readBooleanValue(reader, PLUGIN_DISABLED_BY_DEFAULT);
    //测试版本
    experimental = readBooleanValue(reader, PLUGIN_EXPERIMENTAL);
    if (reader.hasError())
        return;

    //对于测试版的插件默认不启用
    if (experimental)
        disabledByDefault = true;

    //启用设置标记 与 不可用标记相反
    enabledInSettings = !disabledByDefault;

    while (!reader.atEnd()) {
        reader.readNext();
        switch (reader.tokenType()) {
        case QXmlStreamReader::StartElement: {
            const QStringRef element = reader.name();
            if (element == QLatin1String(VENDOR))
                vendor = reader.readElementText().trimmed(); //插件提供方
            else if (element == QLatin1String(COPYRIGHT))
                copyright = reader.readElementText().trimmed();//插件版权
            else if (element == QLatin1String(LICENSE))
                license = reader.readElementText().trimmed();//插件的授权协议
            else if (element == QLatin1String(DESCRIPTION))
                description = reader.readElementText().trimmed();//插件的具体描述
            else if (element == QLatin1String(URL))
                url = reader.readElementText().trimmed();//插件的url地址
            else if (element == QLatin1String(CATEGORY))
                category = reader.readElementText().trimmed();//插件所属的类别
            else if (element == QLatin1String(PLATFORM)) {
                const QString platformSpec = reader.readElementText().trimmed(); //插件的平台描述
                if (!platformSpec.isEmpty()) {
                    platformSpecification.setPattern(platformSpec);
                    if (!platformSpecification.isValid())
                        reader.raiseError(QLatin1String("Invalid platform specification \"")
                                          + platformSpec + QLatin1String("\": ")
                                          + platformSpecification.errorString());
                }
            } else if (element == QLatin1String(DEPENDENCYLIST)) //插件依赖
                readDependencies(reader);
            else if (element == QLatin1String(ARGUMENTLIST)) //插件参数
                readArgumentDescriptions(reader);//读取插件参数
            else
                reader.raiseError(msgInvalidElement(element.toString()));
        }
            break;
        case QXmlStreamReader::EndDocument:
        case QXmlStreamReader::Comment:
        case QXmlStreamReader::EndElement:
        case QXmlStreamReader::Characters:
            break;
        default:
            reader.raiseError(msgUnexpectedToken());
            break;
        }
    }
}

/*!
    \internal
    解析插件参数
*/
void PluginSpecPrivate::readArgumentDescriptions(QXmlStreamReader &reader)
{
    while (!reader.atEnd()) {
        reader.readNext();
        switch (reader.tokenType()) {
        case QXmlStreamReader::StartElement:
            if (reader.name() == QLatin1String(ARGUMENT))
                readArgumentDescription(reader);
            else
                reader.raiseError(msgInvalidElement(reader.name().toString()));
            break;
        case QXmlStreamReader::Comment:
        case QXmlStreamReader::Characters:
            break;
        case QXmlStreamReader::EndElement:
            if (reader.name() == QLatin1String(ARGUMENTLIST))
                return;
            reader.raiseError(msgUnexpectedClosing(reader.name().toString()));
            break;
        default:
            reader.raiseError(msgUnexpectedToken());
            break;
        }
    }
}

/*!
    \internal
    读取一个参数信息并存入参数集
*/
void PluginSpecPrivate::readArgumentDescription(QXmlStreamReader &reader)
{
    PluginArgumentDescription arg;
    arg.name = reader.attributes().value(QLatin1String(ARGUMENT_NAME)).toString();
    if (arg.name.isEmpty()) {
        reader.raiseError(msgAttributeMissing(ARGUMENT, ARGUMENT_NAME));
        return;
    }
    arg.parameter = reader.attributes().value(QLatin1String(ARGUMENT_PARAMETER)).toString();
    arg.description = reader.readElementText();
    if (reader.tokenType() != QXmlStreamReader::EndElement)
        reader.raiseError(msgUnexpectedToken());
    argumentDescriptions.push_back(arg);
}

//读取bool值
bool PluginSpecPrivate::readBooleanValue(QXmlStreamReader &reader, const char *key)
{
    const QStringRef valueString = reader.attributes().value(QLatin1String(key));
    const bool isOn = valueString.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0;
    if (!valueString.isEmpty() && !isOn
            && valueString.compare(QLatin1String("false"), Qt::CaseInsensitive) != 0) {
        reader.raiseError(msgInvalidFormat(key));
    }
    return isOn;
}

/*!
    \internal
    从xml节点读取插件依赖
*/
void PluginSpecPrivate::readDependencies(QXmlStreamReader &reader)
{
    while (!reader.atEnd()) {
        reader.readNext();
        switch (reader.tokenType()) {
        case QXmlStreamReader::StartElement:
            if (reader.name() == QLatin1String(DEPENDENCY)) //一个条目起始标记
                readDependencyEntry(reader); //读取具体的一条依赖
            else
                reader.raiseError(msgInvalidElement(reader.name().toString()));
            break;
        case QXmlStreamReader::Comment:
        case QXmlStreamReader::Characters:
            break;
        case QXmlStreamReader::EndElement:
            if (reader.name() == QLatin1String(DEPENDENCYLIST))
                return;
            reader.raiseError(msgUnexpectedClosing(reader.name().toString()));
            break;
        default:
            reader.raiseError(msgUnexpectedToken());
            break;
        }
    }
}

/*!
    \internal
    读取一条插件依赖
*/
void PluginSpecPrivate::readDependencyEntry(QXmlStreamReader &reader)
{
    PluginDependency dep; //依赖条目结构
    //所依赖的插件名
    dep.name = reader.attributes().value(QLatin1String(DEPENDENCY_NAME)).toString();
    if (dep.name.isEmpty()) {
        reader.raiseError(msgAttributeMissing(DEPENDENCY, DEPENDENCY_NAME));
        return;
    }
    //所依赖的插件版本
    dep.version = reader.attributes().value(QLatin1String(DEPENDENCY_VERSION)).toString();
    if (!dep.version.isEmpty() && !isValidVersion(dep.version)) { //判断是否支持此版本 不支持则报错退出
        reader.raiseError(msgInvalidFormat(DEPENDENCY_VERSION));
        return;
    }
    //所依赖的类型
    dep.type = PluginDependency::Required;
    if (reader.attributes().hasAttribute(QLatin1String(DEPENDENCY_TYPE))) {
        const QStringRef typeValue = reader.attributes().value(QLatin1String(DEPENDENCY_TYPE));
        if (typeValue == QLatin1String(DEPENDENCY_TYPE_HARD)) {
            dep.type = PluginDependency::Required;
        } else if (typeValue == QLatin1String(DEPENDENCY_TYPE_SOFT)) {
            dep.type = PluginDependency::Optional;
        } else {
            reader.raiseError(msgInvalidFormat(DEPENDENCY_TYPE));
            return;
        }
    }
    //插入依赖表中
    dependencies.append(dep);
    reader.readNext();
    if (reader.tokenType() != QXmlStreamReader::EndElement)
        reader.raiseError(msgUnexpectedToken());
}

/*!
    \internal
    判断本插件是否可替代名为pluginName版本为pluginVersion的插件
*/
bool PluginSpecPrivate::provides(const QString &pluginName, const QString &pluginVersion) const
{
    //和本插件名称不同返回错误
    if (QString::compare(pluginName, name, Qt::CaseInsensitive) != 0)
        return false;
    //检查版是否可替代
    return (versionCompare(version, pluginVersion) >= 0) && (versionCompare(compatVersion, pluginVersion) <= 0);
}

/*!
    \internal
    当前支持插件版本的正则匹配
*/
QRegExp &PluginSpecPrivate::versionRegExp()
{
    static QRegExp reg(QLatin1String("([0-9]+)(?:[.]([0-9]+))?(?:[.]([0-9]+))?(?:_([0-9]+))?"));
    return reg;
}
/*!
    \internal
    判断是否支持该版本
*/
bool PluginSpecPrivate::isValidVersion(const QString &version)
{
    return versionRegExp().exactMatch(version);
}

/*!
    \internal
    版本比较 
    如果有一个版本不支持则退出返回错
*/
int PluginSpecPrivate::versionCompare(const QString &version1, const QString &version2)
{
    QRegExp reg1 = versionRegExp();
    QRegExp reg2 = versionRegExp();
    if (!reg1.exactMatch(version1))
        return 0;
    if (!reg2.exactMatch(version2))
        return 0;
    int number1;
    int number2;
    for (int i = 0; i < 4; ++i) {
        number1 = reg1.cap(i+1).toInt();
        number2 = reg2.cap(i+1).toInt();
        if (number1 < number2)
            return -1;
        if (number1 > number2)
            return 1;
    }
    return 0;
}

/*!
    \internal
    检查依赖插件是否在插件集合并根据加载依赖强弱关系 重新更新了read中得到的插件表
*/
bool PluginSpecPrivate::resolveDependencies(const QList<PluginSpec *> &specs)
{
    if (hasError)
        return false;
    if (state == PluginSpec::Resolved)
        state = PluginSpec::Read; // Go back, so we just re-resolve the dependencies.
    if (state != PluginSpec::Read) {
        errorString = QCoreApplication::translate("PluginSpec", "Resolving dependencies failed because state != Read");
        hasError = true;
        return false;
    }
    QHash<PluginDependency, PluginSpec *> resolvedDependencies;
    foreach (const PluginDependency &dependency, dependencies) { //遍历依赖表
        PluginSpec *found = 0;

        foreach (PluginSpec *spec, specs) { //在当前插件集合中找依赖插件
            if (spec->provides(dependency.name, dependency.version)) { //存在则标记找到
                found = spec;
                break;
            }
        }
        //如果不符合要求并且是必须依赖 则打出错标记记录出错信息
        if (!found) {
            if (dependency.type == PluginDependency::Required) {
                hasError = true;
                if (!errorString.isEmpty())
                    errorString.append(QLatin1Char('\n'));
                errorString.append(QCoreApplication::translate("PluginSpec", "Could not resolve dependency '%1(%2)'")
                    .arg(dependency.name).arg(dependency.version));
            }
            continue;
        }
        //对于满足规范的依赖插件插入临时表
        resolvedDependencies.insert(dependency, found);
    }
    //解析过程出现错误返回false
    if (hasError)
        return false;

    //插件映射哈希 key=本插件的依赖插件 value=依赖插件在插件集合的指针 (一个是少量依赖信息 一个是插件的全部信息)
    dependencySpecs = resolvedDependencies;

    state = PluginSpec::Resolved;

    return true;
}

//设置依赖插件的间接启用标记 为真 则意味着插件在启动时不会被加载
void PluginSpecPrivate::disableIndirectlyIfDependencyDisabled()
{
    if (!enabledInSettings)//明确配置不启用直接返回(非启动时加载)
        return;

    if (disabledIndirectly)//为真说明已经处理过了
        return;

    //依次遍历插件
    QHashIterator<PluginDependency, PluginSpec *> it(dependencySpecs);
    while (it.hasNext()) {
        it.next();
        if (it.key().type == PluginDependency::Optional) //忽略可选的插件
            continue;
        PluginSpec *dependencySpec = it.value();
        if (!dependencySpec->isEffectivelyEnabled()) { //如果插件不是在启动时就要加载的则设置间接启用标志为真 因为他不是配置要求启用
            disabledIndirectly = true;
            break;
        }
    }
}

/*!
    \internal
    加载插件所指代的动态库 并创建插件实例
*/
bool PluginSpecPrivate::loadLibrary()
{
    //有错 或者依赖有问题的不加载 返回错误
    if (hasError)
        return false;
    if (state != PluginSpec::Resolved) {
        if (state == PluginSpec::Loaded)
            return true;
        errorString = QCoreApplication::translate("PluginSpec", "Loading the library failed because state != Resolved");
        hasError = true;
        return false;
    }
#ifdef QT_NO_DEBUG

#ifdef Q_OS_WIN
    QString libName = QString::fromLatin1("%1/%2.dll").arg(location).arg(name);
#elif defined(Q_OS_MAC)
    QString libName = QString::fromLatin1("%1/lib%2.dylib").arg(location).arg(name);
#else
    QString libName = QString::fromLatin1("%1/lib%2.so").arg(location).arg(name);
#endif

#else //Q_NO_DEBUG

#ifdef Q_OS_WIN
    //拿到插件对应库的绝对路径
    QString libName = QString::fromLatin1("%1/%2d.dll").arg(location).arg(name);
#elif defined(Q_OS_MAC)
    QString libName = QString::fromLatin1("%1/lib%2_debug.dylib").arg(location).arg(name);
#else
    QString libName = QString::fromLatin1("%1/lib%2.so").arg(location).arg(name);
#endif

#endif

    //加载插件
    PluginLoader loader(libName);
    if (!loader.load()) {
        hasError = true;
        errorString = QDir::toNativeSeparators(libName)
            + QString::fromLatin1(": ") + loader.errorString();
        return false;
    }
    //拿到插件实例 比如会调用CorePlugin的构造方法
    IPlugin *pluginObject = qobject_cast<IPlugin*>(loader.instance());
    if (!pluginObject) {
        hasError = true;
        errorString = QCoreApplication::translate("PluginSpec", "Plugin is not valid (does not derive from IPlugin)");
        loader.unload();
        return false;
    }
    state = PluginSpec::Loaded;
    plugin = pluginObject;
    plugin->d->pluginSpec = q;
    return true;
}

/*!
    \internal
    初始化插件 调用插件的initialize方法
*/
bool PluginSpecPrivate::initializePlugin()
{
    if (hasError)
        return false;
    if (state != PluginSpec::Loaded) {
        if (state == PluginSpec::Initialized)
            return true;
        errorString = QCoreApplication::translate("PluginSpec", "Initializing the plugin failed because state != Loaded");
        hasError = true;
        return false;
    }

    //加载失败
    if (!plugin) {
        errorString = QCoreApplication::translate("PluginSpec", "Internal error: have no plugin instance to initialize");
        hasError = true;
        return false;
    }
    QString err;
    if (!plugin->initialize(arguments, &err)) {
        errorString = QCoreApplication::translate("PluginSpec", "Plugin initialization failed: %1").arg(err);
        hasError = true;
        return false;
    }
    state = PluginSpec::Initialized;
    return true;
}

/*!
    \internal
    //对插件其他子项目进行初始化
*/
bool PluginSpecPrivate::initializeExtensions()
{
    if (hasError)
        return false;
    if (state != PluginSpec::Initialized) {
        if (state == PluginSpec::Running)
            return true;
        errorString = QCoreApplication::translate("PluginSpec", "Cannot perform extensionsInitialized because state != Initialized");
        hasError = true;
        return false;
    }
    if (!plugin) {
        errorString = QCoreApplication::translate("PluginSpec", "Internal error: have no plugin instance to perform extensionsInitialized");
        hasError = true;
        return false;
    }
    //对其他子项目进行初始化
    plugin->extensionsInitialized();
    state = PluginSpec::Running;
    return true;
}

/*!
    \internal
   调用插件的延迟初始化 delayedInitialize
*/
bool PluginSpecPrivate::delayedInitialize()
{
    if (hasError)
        return false;
    if (state != PluginSpec::Running)
        return false;
    if (!plugin) {
        errorString = QCoreApplication::translate("PluginSpec", "Internal error: have no plugin instance to perform delayedInitialize");
        hasError = true;
        return false;
    }
    return plugin->delayedInitialize();
}

/*!
    \internal
*/
IPlugin::ShutdownFlag PluginSpecPrivate::stop()
{
    if (!plugin)
        return IPlugin::SynchronousShutdown;
    state = PluginSpec::Stopped;
    return plugin->aboutToShutdown();
}

/*!
    \internal
*/
void PluginSpecPrivate::kill()
{
    if (!plugin)
        return;
    delete plugin;
    plugin = 0;
    state = PluginSpec::Deleted;
}
