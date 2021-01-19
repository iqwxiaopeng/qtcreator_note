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

#include "pluginmanager.h"
#include "pluginmanager_p.h"
#include "pluginspec.h"
#include "pluginspec_p.h"
#include "optionsparser.h"
#include "iplugin.h"
#include "plugincollection.h"

#include <QEventLoop>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMetaProperty>
#include <QSettings>
#include <QTextStream>
#include <QTime>
#include <QWriteLocker>
#include <QDebug>
#include <QTimer>
#include <QSysInfo>

#ifdef WITH_TESTS
#include <QTest>
#endif

const char C_IGNORED_PLUGINS[] = "Plugins/Ignored";
const char C_FORCEENABLED_PLUGINS[] = "Plugins/ForceEnabled";
const int DELAYED_INITIALIZE_INTERVAL = 20; // ms

typedef QList<ExtensionSystem::PluginSpec *> PluginSpecSet;

enum { debugLeaks = 0 };

/*!
    \namespace ExtensionSystem
    \brief The ExtensionSystem namespace provides classes that belong to the
           core plugin system.

    The basic extension system contains the plugin manager and its supporting classes,
    and the IPlugin interface that must be implemented by plugin providers.
*/

/*!
    \namespace ExtensionSystem::Internal
    \internal
*/

/*!
    \class ExtensionSystem::PluginManager
    \mainclass

    \brief The PluginManager class implements the core plugin system that
    manages the plugins, their life cycle, and their registered objects.

    The plugin manager is used for the following tasks:
    \list
    \li Manage plugins and their state
    \li Manipulate a 'common object pool'
    \endlist

    \section1 Plugins
    Plugins consist of an XML descriptor file, and of a library that contains a Qt plugin
    (declared via Q_EXPORT_PLUGIN) that must derive from the IPlugin class.
    The plugin manager is used to set a list of file system directories to search for
    plugins, retrieve information about the state of these plugins, and to load them.

    Usually, the application creates a PluginManager instance and initiates the
    loading.
    \code
        // 'plugins' and subdirs will be searched for plugins
        ExtensionSystem::PluginManager::setPluginPaths(QStringList() << "plugins");
        ExtensionSystem::PluginManager::loadPlugins(); // try to load all the plugins
    \endcode
    Additionally, it is possible to directly access the plugin specifications
    (the information in the descriptor file), the plugin instances (via PluginSpec),
    and their state.

    \section1 Object Pool
    Plugins (and everybody else) can add objects to a common 'pool' that is located in
    the plugin manager. Objects in the pool must derive from QObject, there are no other
    prerequisites. All objects of a specified type can be retrieved from the object pool
    via the getObjects() and getObject() functions. They are aware of Aggregation::Aggregate, i.e.
    these functions use the Aggregation::query functions instead of a qobject_cast to determine
    the matching objects.

    Whenever the state of the object pool changes a corresponding signal is emitted by the plugin manager.

    A common usecase for the object pool is that a plugin (or the application) provides
    an "extension point" for other plugins, which is a class / interface that can
    be implemented and added to the object pool. The plugin that provides the
    extension point looks for implementations of the class / interface in the object pool.
    \code
        // Plugin A provides a "MimeTypeHandler" extension point
        // in plugin B:
        MyMimeTypeHandler *handler = new MyMimeTypeHandler();
        ExtensionSystem::PluginManager::instance()->addObject(handler);
        // In plugin A:
        QList<MimeTypeHandler *> mimeHandlers =
            ExtensionSystem::PluginManager::getObjects<MimeTypeHandler>();
    \endcode


    The \c{ExtensionSystem::Invoker} class template provides "syntactic sugar"
    for using "soft" extension points that may or may not be provided by an
    object in the pool. This approach does neither require the "user" plugin being
    linked against the "provider" plugin nor a common shared
    header file. The exposed interface is implicitly given by the
    invokable functions of the "provider" object in the object pool.

    The \c{ExtensionSystem::invoke} function template encapsulates
    {ExtensionSystem::Invoker} construction for the common case where
    the success of the call is not checked.

    \code
        // In the "provide" plugin A:
        namespace PluginA {
        class SomeProvider : public QObject
        {
            Q_OBJECT

        public:
            Q_INVOKABLE QString doit(const QString &msg, int n) {
            {
                qDebug() << "I AM DOING IT " << msg;
                return QString::number(n);
            }
        };
        } // namespace PluginA


        // In the "user" plugin B:
        int someFuntionUsingPluginA()
        {
            using namespace ExtensionSystem;

            QObject *target = PluginManager::getObjectByClassName("PluginA::SomeProvider");

            if (target) {
                // Some random argument.
                QString msg = "REALLY.";

                // Plain function call, no return value.
                invoke<void>(target, "doit", msg, 2);

                // Plain function with no return value.
                qDebug() << "Result: " << invoke<QString>(target, "doit", msg, 21);

                // Record success of function call with return value.
                Invoker<QString> in1(target, "doit", msg, 21);
                qDebug() << "Success: (expected)" << in1.wasSuccessful();

                // Try to invoke a non-existing function.
                Invoker<QString> in2(target, "doitWrong", msg, 22);
                qDebug() << "Success (not expected):" << in2.wasSuccessful();

            } else {

                // We have to cope with plugin A's absence.
            }
        };
    \endcode

    \note The type of the parameters passed to the \c{invoke()} calls
    is deduced from the parameters themselves and must match the type of
    the arguments of the called functions \e{exactly}. No conversion or even
    integer promotions are applicable, so to invoke a function with a \c{long}
    parameter explicitly use \c{long(43)} or such.

    \note The object pool manipulating functions are thread-safe.
*/

/*!
    \fn void PluginManager::objectAdded(QObject *obj)
    Signals that \a obj has been added to the object pool.
*/

/*!
    \fn void PluginManager::aboutToRemoveObject(QObject *obj)
    Signals that \a obj will be removed from the object pool.
*/

/*!
    \fn void PluginManager::pluginsChanged()
    Signals that the list of available plugins has changed.

    \sa plugins()
*/

/*!
    \fn T *PluginManager::getObject()

    Retrieves the object of a given type from the object pool.

    This function is aware of Aggregation::Aggregate. That is, it uses
    the \c Aggregation::query functions instead of \c qobject_cast to
    determine the type of an object.
    If there are more than one object of the given type in
    the object pool, this function will choose an arbitrary one of them.

    \sa addObject()
*/

/*!
    \fn QList<T *> PluginManager::getObjects()

    Retrieves all objects of a given type from the object pool.

    This function is aware of Aggregation::Aggregate. That is, it uses
    the \c Aggregation::query functions instead of \c qobject_cast to
    determine the type of an object.

    \sa addObject()
*/

using namespace ExtensionSystem;
using namespace ExtensionSystem::Internal;

static Internal::PluginManagerPrivate *d = 0;
static PluginManager *m_instance = 0;
//为了对插件描述对象进行比较排序用
static bool lessThanByPluginName(const PluginSpec *one, const PluginSpec *two)
{
    return one->name() < two->name();
}

/*!
    Gets the unique plugin manager instance.
*/
PluginManager *PluginManager::instance()
{
    return m_instance;
}

/*!
    Creates a plugin manager. Should be done only once per application.
*/
PluginManager::PluginManager()
{
    m_instance = this;
    d = new PluginManagerPrivate(this);
}

/*!
    \internal
*/
PluginManager::~PluginManager()
{
    delete d;
    d = 0;
}

/*!
    Adds the object \a obj to the object pool, so it can be retrieved
    again from the pool by type.

    The plugin manager does not do any memory management - added objects
    must be removed from the pool and deleted manually by whoever is responsible for the object.

    Emits the objectAdded() signal.

    \sa PluginManager::removeObject()
    \sa PluginManager::getObject()
    \sa PluginManager::getObjects()
*/
void PluginManager::addObject(QObject *obj)
{
    d->addObject(obj);
}

/*!
    Emits aboutToRemoveObject() and removes the object \a obj from the object pool.
    \sa PluginManager::addObject()
*/
void PluginManager::removeObject(QObject *obj)
{
    d->removeObject(obj);
}

/*!
    Retrieves the list of all objects in the pool, unfiltered.

    Usually, clients do not need to call this function.

    \sa PluginManager::getObject()
    \sa PluginManager::getObjects()
*/
QList<QObject *> PluginManager::allObjects()
{
    return d->allObjects;
}

QReadWriteLock *PluginManager::listLock()
{
    return &d->m_lock;
}

/*!
    Tries to load all the plugins that were previously found when
    setting the plugin search paths. The plugin specs of the plugins
    can be used to retrieve error and state information about individual plugins.

    \sa setPluginPaths()
    \sa plugins()
*/
void PluginManager::loadPlugins()
{
    return d->loadPlugins();
}

/*!
    Returns true if any plugin has errors even though it is enabled.
    Most useful to call after loadPlugins().
*/
bool PluginManager::hasError()
{
    foreach (PluginSpec *spec, plugins()) {
        // only show errors on startup if plugin is enabled.
        if (spec->hasError() && spec->isEnabledInSettings() && !spec->isDisabledIndirectly())
            return true;
    }
    return false;
}

/*!
    Shuts down and deletes all plugins.
*/
void PluginManager::shutdown()
{
    d->shutdown();
}

/*!
    The list of paths were the plugin manager searches for plugins.

    \sa setPluginPaths()
*/
QStringList PluginManager::pluginPaths()
{
    return d->pluginPaths;
}

/*!
    Sets the plugin search paths, i.e. the file system paths where the plugin manager
    looks for plugin descriptions. All given \a paths and their sub directory trees
    are searched for plugin xml description files.

    \sa pluginPaths()
    \sa loadPlugins()
*/
void PluginManager::setPluginPaths(const QStringList &paths)
{
    d->setPluginPaths(paths);
}

/*!
    The file extension of plugin description files.
    The default is "xml".

    \sa setFileExtension()
*/
QString PluginManager::fileExtension()
{
    return d->extension;
}

/*!
    Sets the file extension of plugin description files.
    The default is "xml".
    At the moment this must be called before setPluginPaths() is called.
    // ### TODO let this + setPluginPaths read the plugin specs lazyly whenever loadPlugins() or plugins() is called.
*/
void PluginManager::setFileExtension(const QString &extension)
{
    d->extension = extension;
}

/*!
    Defines the user specific settings to use for information about enabled and
    disabled plugins.
    Needs to be set before the plugin search path is set with setPluginPaths().
*/
void PluginManager::setSettings(QSettings *settings)
{
    d->setSettings(settings);
}

/*!
    Defines the global (user-independent) settings to use for information about
    default disabled plugins.
    Needs to be set before the plugin search path is set with setPluginPaths().
*/
void PluginManager::setGlobalSettings(QSettings *settings)
{
    d->setGlobalSettings(settings);
}

/*!
    Returns the user specific settings used for information about enabled and
    disabled plugins.
*/
QSettings *PluginManager::settings()
{
    return d->settings;
}

/*!
    Returns the global (user-independent) settings used for information about default disabled plugins.
*/
QSettings *PluginManager::globalSettings()
{
    return d->globalSettings;
}

void PluginManager::writeSettings()
{
    d->writeSettings();
}

/*!
    The arguments left over after parsing (that were neither startup nor plugin
    arguments). Typically, this will be the list of files to open.
*/
QStringList PluginManager::arguments()
{
    return d->arguments;
}

/*!
    List of all plugin specifications that have been found in the plugin search paths.
    This list is valid directly after the setPluginPaths() call.
    The plugin specifications contain the information from the plugins' xml description files
    and the current state of the plugins. If a plugin's library has been already successfully loaded,
    the plugin specification has a reference to the created plugin instance as well.

    \sa setPluginPaths()
*/
QList<PluginSpec *> PluginManager::plugins()
{
    return d->pluginSpecs;
}

QHash<QString, PluginCollection *> PluginManager::pluginCollections()
{
    return d->pluginCategories;
}

static const char argumentKeywordC[] = ":arguments";

/*!
    Serializes plugin options and arguments for sending in a single string
    via QtSingleApplication:
    ":myplugin|-option1|-option2|:arguments|argument1|argument2",
    as a list of lists started by a keyword with a colon. Arguments are last.

    \sa setPluginPaths()
*/
QString PluginManager::serializedArguments()
{
    const QChar separator = QLatin1Char('|');
    QString rc;
    foreach (const PluginSpec *ps, plugins()) {
        if (!ps->arguments().isEmpty()) {
            if (!rc.isEmpty())
                rc += separator;
            rc += QLatin1Char(':');
            rc += ps->name();
            rc += separator;
            rc +=  ps->arguments().join(QString(separator));
        }
    }
    if (!d->arguments.isEmpty()) {
        if (!rc.isEmpty())
            rc += separator;
        rc += QLatin1String(argumentKeywordC);
        // If the argument appears to be a file, make it absolute
        // when sending to another instance.
        foreach (const QString &argument, d->arguments) {
            rc += separator;
            const QFileInfo fi(argument);
            if (fi.exists() && fi.isRelative())
                rc += fi.absoluteFilePath();
            else
                rc += argument;
        }
    }
    return rc;
}

/* Extract a sublist from the serialized arguments
 * indicated by a keyword starting with a colon indicator:
 * ":a,i1,i2,:b:i3,i4" with ":a" -> "i1,i2"
 */
static QStringList subList(const QStringList &in, const QString &key)
{
    QStringList rc;
    // Find keyword and copy arguments until end or next keyword
    const QStringList::const_iterator inEnd = in.constEnd();
    QStringList::const_iterator it = qFind(in.constBegin(), inEnd, key);
    if (it != inEnd) {
        const QChar nextIndicator = QLatin1Char(':');
        for (++it; it != inEnd && !it->startsWith(nextIndicator); ++it)
            rc.append(*it);
    }
    return rc;
}

/*!
    Parses the options encoded by serializedArguments() const
    and passes them on to the respective plugins along with the arguments.

    \a socket is passed for disconnecting the peer when the operation is done (for example,
    document is closed) for supporting the -block flag.
*/

void PluginManager::remoteArguments(const QString &serializedArgument, QObject *socket)
{
    if (serializedArgument.isEmpty())
        return;
    QStringList serializedArguments = serializedArgument.split(QLatin1Char('|'));
    const QStringList arguments = subList(serializedArguments, QLatin1String(argumentKeywordC));
    foreach (const PluginSpec *ps, plugins()) {
        if (ps->state() == PluginSpec::Running) {
            const QStringList pluginOptions = subList(serializedArguments, QLatin1Char(':') + ps->name());
            QObject *socketParent = ps->plugin()->remoteCommand(pluginOptions, arguments);
            if (socketParent && socket) {
                socket->setParent(socketParent);
                socket = 0;
            }
        }
    }
    if (socket)
        delete socket;
}

/*!
    Takes the list of command line options in \a args and parses them.
    The plugin manager itself might process some options itself directly (-noload <plugin>), and
    adds options that are registered by plugins to their plugin specs.
    The caller (the application) may register itself for options via the \a appOptions list, containing pairs
    of "option string" and a bool that indicates if the option requires an argument.
    Application options always override any plugin's options.

    \a foundAppOptions is set to pairs of ("option string", "argument") for any application options that were found.
    The command line options that were not processed can be retrieved via the arguments() function.
    If an error occurred (like missing argument for an option that requires one), \a errorString contains
    a descriptive message of the error.

    Returns if there was an error.
 */
bool PluginManager::parseOptions(const QStringList &args,
    const QMap<QString, bool> &appOptions,
    QMap<QString, QString> *foundAppOptions,
    QString *errorString)
{
    OptionsParser options(args, appOptions, foundAppOptions, errorString, d);
    return options.parse();
}



static inline void indent(QTextStream &str, int indent)
{
    const QChar blank = QLatin1Char(' ');
    for (int i = 0 ; i < indent; i++)
        str << blank;
}

static inline void formatOption(QTextStream &str,
                                const QString &opt, const QString &parm, const QString &description,
                                int optionIndentation, int descriptionIndentation)
{
    int remainingIndent = descriptionIndentation - optionIndentation - opt.size();
    indent(str, optionIndentation);
    str << opt;
    if (!parm.isEmpty()) {
        str << " <" << parm << '>';
        remainingIndent -= 3 + parm.size();
    }
    indent(str, qMax(1, remainingIndent));
    str << description << '\n';
}

/*!
    Formats the startup options of the plugin manager for command line help.
*/

void PluginManager::formatOptions(QTextStream &str, int optionIndentation, int descriptionIndentation)
{
    formatOption(str, QLatin1String(OptionsParser::LOAD_OPTION),
                 QLatin1String("plugin"), QLatin1String("Load <plugin>"),
                 optionIndentation, descriptionIndentation);
    formatOption(str, QLatin1String(OptionsParser::NO_LOAD_OPTION),
                 QLatin1String("plugin"), QLatin1String("Do not load <plugin>"),
                 optionIndentation, descriptionIndentation);
    formatOption(str, QLatin1String(OptionsParser::PROFILE_OPTION),
                 QString(), QLatin1String("Profile plugin loading"),
                 optionIndentation, descriptionIndentation);
#ifdef WITH_TESTS
    formatOption(str, QString::fromLatin1(OptionsParser::TEST_OPTION)
                 + QLatin1String(" <plugin>[,testfunction[:testdata]]..."), QString(),
                 QLatin1String("Run plugin's tests (by default a separate settings path is used)"),
                 optionIndentation, descriptionIndentation);
    formatOption(str, QString::fromLatin1(OptionsParser::TEST_OPTION) + QLatin1String(" all"),
                 QString(), QLatin1String("Run tests from all plugins"),
                 optionIndentation, descriptionIndentation);
#endif
}

/*!
    Formats the plugin options of the plugin specs for command line help.
*/

void PluginManager::formatPluginOptions(QTextStream &str, int optionIndentation, int descriptionIndentation)
{
    typedef PluginSpec::PluginArgumentDescriptions PluginArgumentDescriptions;
    // Check plugins for options
    const PluginSpecSet::const_iterator pcend = d->pluginSpecs.constEnd();
    for (PluginSpecSet::const_iterator pit = d->pluginSpecs.constBegin(); pit != pcend; ++pit) {
        const PluginArgumentDescriptions pargs = (*pit)->argumentDescriptions();
        if (!pargs.empty()) {
            str << "\nPlugin: " <<  (*pit)->name() << '\n';
            const PluginArgumentDescriptions::const_iterator acend = pargs.constEnd();
            for (PluginArgumentDescriptions::const_iterator ait =pargs.constBegin(); ait != acend; ++ait)
                formatOption(str, ait->name, ait->parameter, ait->description, optionIndentation, descriptionIndentation);
        }
    }
}

/*!
    Formats the version of the plugin specs for command line help.
*/
void PluginManager::formatPluginVersions(QTextStream &str)
{
    const PluginSpecSet::const_iterator cend = d->pluginSpecs.constEnd();
    for (PluginSpecSet::const_iterator it = d->pluginSpecs.constBegin(); it != cend; ++it) {
        const PluginSpec *ps = *it;
        str << "  " << ps->name() << ' ' << ps->version() << ' ' << ps->description() <<  '\n';
    }
}

void PluginManager::startTests()
{
#ifdef WITH_TESTS
    foreach (const PluginManagerPrivate::TestSpec &testSpec, d->testSpecs) {
        const PluginSpec * const pluginSpec = testSpec.pluginSpec;
        if (!pluginSpec->plugin())
            continue;

        // Collect all test functions of the plugin.
        QStringList allTestFunctions;
        const QMetaObject *metaObject = pluginSpec->plugin()->metaObject();

        for (int i = metaObject->methodOffset(); i < metaObject->methodCount(); ++i) {
#if QT_VERSION >= 0x050000
            const QByteArray signature = metaObject->method(i).methodSignature();
#else
            const QByteArray signature = metaObject->method(i).signature();
#endif
            if (signature.startsWith("test") && !signature.endsWith("_data()")) {
                const QString method = QString::fromLatin1(signature);
                const QString methodName = method.left(method.size() - 2);
                allTestFunctions.append(methodName);
            }
        }

        QStringList testFunctionsToExecute;

        // User did not specify any test functions, so add every test function.
        if (testSpec.testFunctions.isEmpty()) {
            testFunctionsToExecute = allTestFunctions;

        // User specified test functions. Add them if they are valid.
        } else {
            foreach (const QString &userTestFunction, testSpec.testFunctions) {
                // There might be a test data suffix like in "testfunction:testdata1".
                QString testFunctionName = userTestFunction;
                QString testDataSuffix;
                const int index = testFunctionName.indexOf(QLatin1Char(':'));
                if (index != -1) {
                    testDataSuffix = testFunctionName.mid(index);
                    testFunctionName = testFunctionName.left(index);
                }

                const QRegExp regExp(testFunctionName, Qt::CaseSensitive, QRegExp::Wildcard);
                QStringList matchingFunctions;
                foreach (const QString &testFunction, allTestFunctions) {
                    if (regExp.exactMatch(testFunction))
                        matchingFunctions.append(testFunction);
                }
                if (!matchingFunctions.isEmpty()) {
                    // If the specified test data is invalid, the QTest framework will
                    // print a reasonable error message for us.
                    foreach (const QString &matchingFunction, matchingFunctions)
                        testFunctionsToExecute.append(matchingFunction + testDataSuffix);
                } else {
                    QTextStream out(stdout);
                    out << "No test function matches \"" << testFunctionName
                        << "\" for plugin \"" << pluginSpec->name() << "\"." << endl
                        << "  Available test functions for plugin \"" << pluginSpec->name()
                        << "\" are:" << endl;
                    foreach (const QString &testFunction, allTestFunctions)
                        out << "    " << testFunction << endl;
                }
            }
        }

        // Don't run QTest::qExec without any test functions, that'd run
        // *all* slots as tests.
        if (!testFunctionsToExecute.isEmpty()) {
            // QTest::qExec() expects basically QCoreApplication::arguments(),
            QStringList qExecArguments = QStringList()
                << QLatin1String("arg0") // fake application name
                << QLatin1String("-maxwarnings") << QLatin1String("0"); // unlimit output
            qExecArguments << testFunctionsToExecute;
            QTest::qExec(pluginSpec->plugin(), qExecArguments);
        }
    }
    if (!d->testSpecs.isEmpty())
        QTimer::singleShot(1, QCoreApplication::instance(), SLOT(quit()));
#endif
}

/*!
 * \internal
 */
bool PluginManager::testRunRequested()
{
    return !d->testSpecs.isEmpty();
}

/*!
 * \internal
 */
QString PluginManager::testDataDirectory()
{
    QByteArray ba = qgetenv("QTCREATOR_TEST_DIR");
    QString s = QString::fromLocal8Bit(ba.constData(), ba.size());
    if (s.isEmpty()) {
        s = QLatin1String(IDE_TEST_DIR);
        s.append(QLatin1String("/tests"));
    }
    s = QDir::cleanPath(s);
    return s;
}

/*!
    Creates a profiling entry showing the elapsed time if profiling is
    activated.
*/

void PluginManager::profilingReport(const char *what, const PluginSpec *spec)
{
    d->profilingReport(what, spec);
}


/*!
    Returns a list of plugins in load order.
*/
QList<PluginSpec *> PluginManager::loadQueue()
{
    return d->loadQueue();
}

//============PluginManagerPrivate===========

/*!
    \internal
*/
PluginSpec *PluginManagerPrivate::createSpec()
{
    return new PluginSpec();
}

/*!
    \internal
*/
void PluginManagerPrivate::setSettings(QSettings *s)
{
    if (settings)
        delete settings;
    settings = s;
    if (settings)
        settings->setParent(this);
}

/*!
    \internal
*/
void PluginManagerPrivate::setGlobalSettings(QSettings *s)
{
    if (globalSettings)
        delete globalSettings;
    globalSettings = s;
    if (globalSettings)
        globalSettings->setParent(this);
}

/*!
    \internal
*/
PluginSpecPrivate *PluginManagerPrivate::privateSpec(PluginSpec *spec)
{
    return spec->d;
}

//延时初始化函数
void PluginManagerPrivate::nextDelayedInitialize()
{
    //从延迟表取插件初始化
    while (!delayedInitializeQueue.isEmpty()) {
        PluginSpec *spec = delayedInitializeQueue.takeFirst();
        profilingReport(">delayedInitialize", spec);
        bool delay = spec->d->delayedInitialize(); //调用插件的延迟初始化
        profilingReport("<delayedInitialize", spec);
        if (delay) //如果插件加载完后要延迟则等会在加载下一个
            break; // do next delayedInitialize after a delay
    }
    //该加载的已经加载完毕 则清除延时加载表 清楚定时器 发消息初始化完成 如果要求试运行则开始试运行
    if (delayedInitializeQueue.isEmpty()) {
        delete delayedInitializeTimer;
        delayedInitializeTimer = 0;
        profilingSummary();
        emit q->initializationDone();
#ifdef WITH_TESTS
        if (q->testRunRequested())
            q->startTests();
#endif
    } else { //延时队列仍就有插件要加载 则开启延时定时器等待下次再执行这个函数
        delayedInitializeTimer->start();
    }
}

/*!
    \internal
*/
PluginManagerPrivate::PluginManagerPrivate(PluginManager *pluginManager) :
    extension(QLatin1String("xml")),
    delayedInitializeTimer(0),
    shutdownEventLoop(0),
    m_profileElapsedMS(0),
    m_profilingVerbosity(0),
    settings(0),
    globalSettings(0),
    q(pluginManager)
{
}


/*!
    \internal
*/
PluginManagerPrivate::~PluginManagerPrivate()
{
    qDeleteAll(pluginSpecs);
    qDeleteAll(pluginCategories);
}

/*!
    \internal
*/
void PluginManagerPrivate::writeSettings()
{
    if (!settings)
        return;
    QStringList tempDisabledPlugins;
    QStringList tempForceEnabledPlugins;
    foreach (PluginSpec *spec, pluginSpecs) {
        if (!spec->isDisabledByDefault() && !spec->isEnabledInSettings())
            tempDisabledPlugins.append(spec->name());
        if (spec->isDisabledByDefault() && spec->isEnabledInSettings())
            tempForceEnabledPlugins.append(spec->name());
    }

    settings->setValue(QLatin1String(C_IGNORED_PLUGINS), tempDisabledPlugins);
    settings->setValue(QLatin1String(C_FORCEENABLED_PLUGINS), tempForceEnabledPlugins);
}

/*!
    \internal 读取插件相关的忽略加载项和强制加载项
*/
void PluginManagerPrivate::readSettings()
{
    //分别读取全局配置和用户配置的强制加载项和忽略项
    if (globalSettings) {
        defaultDisabledPlugins = globalSettings->value(QLatin1String(C_IGNORED_PLUGINS)).toStringList();
        defaultEnabledPlugins = globalSettings->value(QLatin1String(C_FORCEENABLED_PLUGINS)).toStringList();
    }
    if (settings) {
        disabledPlugins = settings->value(QLatin1String(C_IGNORED_PLUGINS)).toStringList();
        forceEnabledPlugins = settings->value(QLatin1String(C_FORCEENABLED_PLUGINS)).toStringList();
    }
}

/*!
    \internal
*/
void PluginManagerPrivate::stopAll()
{
    if (delayedInitializeTimer && delayedInitializeTimer->isActive()) {
        delayedInitializeTimer->stop();
        delete delayedInitializeTimer;
        delayedInitializeTimer = 0;
    }
    QList<PluginSpec *> queue = loadQueue();
    foreach (PluginSpec *spec, queue) {
        loadPlugin(spec, PluginSpec::Stopped);
    }
}

/*!
    \internal
*/
void PluginManagerPrivate::deleteAll()
{
    QList<PluginSpec *> queue = loadQueue();
    QListIterator<PluginSpec *> it(queue);
    it.toBack();
    while (it.hasPrevious()) {
        loadPlugin(it.previous(), PluginSpec::Deleted);
    }
}

/*!
    \internal
*/
void PluginManagerPrivate::addObject(QObject *obj)
{
    {
        QWriteLocker lock(&m_lock);
        if (obj == 0) {
            qWarning() << "PluginManagerPrivate::addObject(): trying to add null object";
            return;
        }
        if (allObjects.contains(obj)) {
            qWarning() << "PluginManagerPrivate::addObject(): trying to add duplicate object";
            return;
        }

        if (debugLeaks)
            qDebug() << "PluginManagerPrivate::addObject" << obj << obj->objectName();

        if (m_profilingVerbosity && !m_profileTimer.isNull()) {
            // Report a timestamp when adding an object. Useful for profiling
            // its initialization time.
            const int absoluteElapsedMS = m_profileTimer->elapsed();
            qDebug("  %-43s %8dms", obj->metaObject()->className(), absoluteElapsedMS);
        }

        allObjects.append(obj);
    }
    emit q->objectAdded(obj);
}

/*!
    \internal
*/
void PluginManagerPrivate::removeObject(QObject *obj)
{
    if (obj == 0) {
        qWarning() << "PluginManagerPrivate::removeObject(): trying to remove null object";
        return;
    }

    if (!allObjects.contains(obj)) {
        qWarning() << "PluginManagerPrivate::removeObject(): object not in list:"
            << obj << obj->objectName();
        return;
    }
    if (debugLeaks)
        qDebug() << "PluginManagerPrivate::removeObject" << obj << obj->objectName();

    emit q->aboutToRemoveObject(obj);
    QWriteLocker lock(&m_lock);
    allObjects.removeAll(obj);
}

/*!
    \internal
    加载插件
*/
void PluginManagerPrivate::loadPlugins()
{
    //Returns a list of plugins in load order.
    QList<PluginSpec *> queue = loadQueue();

    //顺序加载每个插件
    foreach (PluginSpec *spec, queue) {
        loadPlugin(spec, PluginSpec::Loaded);
    }

    //顺序初始化每个插件
    foreach (PluginSpec *spec, queue) {
        loadPlugin(spec, PluginSpec::Initialized);
    }

    //根据依赖关系自下而上依次运行插件
    QListIterator<PluginSpec *> it(queue);
    it.toBack();
    while (it.hasPrevious()) {
        PluginSpec *spec = it.previous();
        loadPlugin(spec, PluginSpec::Running); //倒序对插件扩展项进行初始化
        if (spec->state() == PluginSpec::Running)
            delayedInitializeQueue.append(spec); //扩展项初始化好的放入延迟表
    }
    emit q->pluginsChanged();

    //启动一个延迟初始化定时器 只会出发一次 20ms后触发
    delayedInitializeTimer = new QTimer;
    delayedInitializeTimer->setInterval(DELAYED_INITIALIZE_INTERVAL);
    delayedInitializeTimer->setSingleShot(true);
    connect(delayedInitializeTimer, SIGNAL(timeout()),
            this, SLOT(nextDelayedInitialize()));
    delayedInitializeTimer->start(); //开定时器
}

/*!
    \internal
*/
void PluginManagerPrivate::shutdown()
{
    stopAll();
    if (!asynchronousPlugins.isEmpty()) {
        shutdownEventLoop = new QEventLoop;
        shutdownEventLoop->exec();
    }
    deleteAll();
    if (!allObjects.isEmpty())
        qDebug() << "There are" << allObjects.size() << "objects left in the plugin manager pool: " << allObjects;
}

/*!
    \internal
*/
void PluginManagerPrivate::asyncShutdownFinished()
{
    IPlugin *plugin = qobject_cast<IPlugin *>(sender());
    Q_ASSERT(plugin);
    asynchronousPlugins.removeAll(plugin->pluginSpec());
    if (asynchronousPlugins.isEmpty())
        shutdownEventLoop->exit();
}

/*!
    \internal
    获得当前插件的非循环依赖 循环依赖项会被移除
*/
QList<PluginSpec *> PluginManagerPrivate::loadQueue()
{
    QList<PluginSpec *> queue;
    //依次检测插件的依赖关系可能会依赖多个插件 但是在检测过程中会顺带检测子插件 因此实际上检测一个插件就是检测了插件和其依赖插件因而返回一个队列
    foreach (PluginSpec *spec, pluginSpecs) {
        QList<PluginSpec *> circularityCheckQueue; //每次会被清空从而保证检测的是当前spec的依赖关系
        loadQueue(spec, queue, circularityCheckQueue);
    }
    return queue;
}

/*!
    \internal
    queue 记录依赖关系检测OK的插件(检测OK只是说明没发现循环依赖而已,而返回值是说依赖有没有问题,除了循环依赖还可能依赖没找到或者版本不匹配)
    circularityCheckQueue 记录处于检测中的插件和已发现的依赖OK插件因为不OK的会清除掉(一个插件处于检测中说明其要么被依赖要么是正在排查他的依赖问题,如果这个时候有插件依赖这些插件，说明出现循环依赖)
*/
bool PluginManagerPrivate::loadQueue(PluginSpec *spec, QList<PluginSpec *> &queue,
        QList<PluginSpec *> &circularityCheckQueue)
{
    //已经检测成功了 直接返回true
    if (queue.contains(spec))
        return true;
    // check for circular dependencies 正在处理本插件的依赖关系,现在本插件又被依赖,说明出现了循环依赖
    if (circularityCheckQueue.contains(spec)) {
        spec->d->hasError = true;
        spec->d->errorString = PluginManager::tr("Circular dependency detected:");
        spec->d->errorString += QLatin1Char('\n');
        int index = circularityCheckQueue.indexOf(spec); //拿到插件索引
        for (int i = index; i < circularityCheckQueue.size(); ++i) { //从索引开始到后面的插件依次提醒错误(因为是顺序检测的,所以后面的都可能有问题)
            spec->d->errorString.append(PluginManager::tr("%1(%2) depends on")
                .arg(circularityCheckQueue.at(i)->name()).arg(circularityCheckQueue.at(i)->version()));
            spec->d->errorString += QLatin1Char('\n');
        }
        spec->d->errorString.append(PluginManager::tr("%1(%2)").arg(spec->name()).arg(spec->version()));
        return false;
    }
    circularityCheckQueue.append(spec);
    // check if we have the dependencies
    if (spec->state() == PluginSpec::Invalid || spec->state() == PluginSpec::Read) { //非Resolved表示其过程中存在依赖问题,压根不会读取其依赖,所以自然谈不上循环依赖所以控件直接检测OK但是结果返回false证明是依赖有问题
        queue.append(spec);
        return false;
    }

    // add dependencies 依次检测本插件依赖插件的依赖关系是否OK 只有本插件和其依赖插件的依赖关系都OK才是真的OK
    foreach (PluginSpec *depSpec, spec->dependencySpecs()) { 
        if (!loadQueue(depSpec, queue, circularityCheckQueue)) { //
            spec->d->hasError = true;
            spec->d->errorString =
                PluginManager::tr("Cannot load plugin because dependency failed to load: %1(%2)\nReason: %3")
                    .arg(depSpec->name()).arg(depSpec->version()).arg(depSpec->errorString());
            return false;
        }
    }
    // add self 一切OK则增加到处理队列
    queue.append(spec);
    return true;
}

/*!
    \internal
    加载插件 和初始化插件 通过destState来指定
*/
void PluginManagerPrivate::loadPlugin(PluginSpec *spec, PluginSpec::State destState)
{
    if (spec->hasError() || spec->state() != destState-1) //如果已经出错或者状态部队则直接返回
        return;

    // don't load disabled plugins.
    if (!spec->isEffectivelyEnabled() && destState == PluginSpec::Loaded) //如果不要求启动时加载或者已经加载则返回
        return;

    switch (destState) {
    case PluginSpec::Running:
        profilingReport(">initializeExtensions", spec);
        spec->d->initializeExtensions(); //对插件的其他子项目进行初始化
        profilingReport("<initializeExtensions", spec);
        return;
    case PluginSpec::Deleted:
        profilingReport(">delete", spec);
        spec->d->kill();
        profilingReport("<delete", spec);
        return;
    default:
        break;
    }

    //检查插件的依赖项是否有错误,如果有错误则返回也就不会得到加载
    // check if dependencies have loaded without error
    QHashIterator<PluginDependency, PluginSpec *> it(spec->dependencySpecs());
    while (it.hasNext()) {
        it.next();
        if (it.key().type == PluginDependency::Optional)
            continue;
        PluginSpec *depSpec = it.value();
        if (depSpec->state() != destState) {
            spec->d->hasError = true;
            spec->d->errorString =
                PluginManager::tr("Cannot load plugin because dependency failed to load: %1(%2)\nReason: %3")
                    .arg(depSpec->name()).arg(depSpec->version()).arg(depSpec->errorString());
            return;
        }
    }

    //根据状态来 加载 初始化 或者停止插件
    switch (destState) {
    case PluginSpec::Loaded:
        profilingReport(">loadLibrary", spec);
        spec->d->loadLibrary();
        profilingReport("<loadLibrary", spec);
        break;
    case PluginSpec::Initialized:
        profilingReport(">initializePlugin", spec);
        spec->d->initializePlugin();
        profilingReport("<initializePlugin", spec);
        break;
    case PluginSpec::Stopped:
        profilingReport(">stop", spec);
        if (spec->d->stop() == IPlugin::AsynchronousShutdown) {
            asynchronousPlugins << spec;
            connect(spec->plugin(), SIGNAL(asynchronousShutdownFinished()),
                    this, SLOT(asyncShutdownFinished()));
        }
        profilingReport("<stop", spec);
        break;
    default:
        break;
    }
}

/*!
    \internal
    //设置插件描述文件的目录 会去读取插件描述信息,创建描述文件表
*/
void PluginManagerPrivate::setPluginPaths(const QStringList &paths)
{
    pluginPaths = paths;
    readSettings(); //读取强制忽略和启用的插件
    readPluginPaths();
}

/*!
    \internal
    1. 在插件目录搜索插件描述文件
    2. 填充与插件相关的集合，获取依赖
    3. 更新插件的依赖关系
*/
void PluginManagerPrivate::readPluginPaths()
{
    qDeleteAll(pluginCategories);
    qDeleteAll(pluginSpecs);
    pluginSpecs.clear();
    pluginCategories.clear();

    //每个插件都需要提供一个插件描述文件pluginspec，用于提供关于插件的元数据，例如版本、依赖项等
    //qt-creator-opensource-src-3.0.0/lib/qtcreator/plugins/QtProject/Core.pluginspec
    QStringList specFiles; //存储插件目录中 *.pluginspec 类文件 
    QStringList searchPaths = pluginPaths;
    //递归搜索插件
    while (!searchPaths.isEmpty()) {
        const QDir dir(searchPaths.takeFirst()); //清除搜索过的目录
        const QString pattern = QLatin1String("*.") + extension; //匹配 *.pluginspec 的文件 
        const QFileInfoList files = dir.entryInfoList(QStringList(pattern), QDir::Files);
        foreach (const QFileInfo &file, files) //存插件描述文件
            specFiles << file.absoluteFilePath();

        //将子目录获取
        const QFileInfoList dirs = dir.entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot);

        //将子目录作也加入插件搜索目录 此处转了子目录的绝对路径
        foreach (const QFileInfo &subdir, dirs)
            searchPaths << subdir.absoluteFilePath();
    }

    //创建默认的插件描述集
    defaultCollection = new PluginCollection(QString());
    //将默认描述集插入插件种类表
    pluginCategories.insert(QString(), defaultCollection);

    //读取解析插件描述
    foreach (const QString &specFile, specFiles) {
        //实例化插件描述代理类
        PluginSpec *spec = new PluginSpec;
        //调用具体类读取插件描述文件填充插件描述类的数据
        spec->d->read(specFile);
        //插件描述类集 指向pluginCategories 插件表的具体某类插件集合
        PluginCollection *collection = 0;
        // find correct plugin collection or create a new one
        if (pluginCategories.contains(spec->category())) { //已经存在此类型的插件描述集合 则取出这个集合不存在，则创建这类集合插入插件表
            collection = pluginCategories.value(spec->category());
        } else {
            collection = new PluginCollection(spec->category());
            pluginCategories.insert(spec->category(), collection);
        }

        // defaultDisabledPlugins and defaultEnabledPlugins from install settings
        // is used to override the defaults read from the plugin spec
        if (!spec->isDisabledByDefault() && defaultDisabledPlugins.contains(spec->name())) { //若插件自身设置启用但被默认忽略则设置默认忽略标记为true 并设置不启用
            spec->setDisabledByDefault(true);
            spec->setEnabled(false);
        } else if (spec->isDisabledByDefault() && defaultEnabledPlugins.contains(spec->name())) { //若插件自己设置不启用 则设置默认忽略为false因为默认不启用是插件要求的
            spec->setDisabledByDefault(false);
            spec->setEnabled(true);
        }
        if (spec->isDisabledByDefault() && forceEnabledPlugins.contains(spec->name()))//若插件自身设置不启用但是强制启用表要求其必须启用，则设置启用
            spec->setEnabled(true);
        if (!spec->isDisabledByDefault() && disabledPlugins.contains(spec->name()))//若插件自身设置启用，但是忽略表要求其不启用 则设置不启用
            spec->setEnabled(false);

        collection->addPlugin(spec); //将解析到的插件按类型 插入插件表 
        pluginSpecs.append(spec); //将解析到的插件描述对象放入list
    }
    resolveDependencies(); //处理依赖的插件 生成信息映射表 更新间接依赖标记
    // ensure deterministic plugin load order by sorting
    qSort(pluginSpecs.begin(), pluginSpecs.end(), lessThanByPluginName);

    emit q->pluginsChanged();//发射了 SIGNAL 2
}

//插件解决依赖 生成插件映射表 更新插件的坚决启用标记
void PluginManagerPrivate::resolveDependencies()
{
    //依次逐个插件解决依赖
    foreach (PluginSpec *spec, pluginSpecs) {
        spec->d->resolveDependencies(pluginSpecs);
    }

    // Reset disabledIndirectly flag loadQueue()返回的插件,依赖关系都没有问题,对这些插件的间接不使用标记设定默认值false
    foreach (PluginSpec *spec, loadQueue())
        spec->d->disabledIndirectly = false;

    //依次处理插件的间接启用标记
    foreach (PluginSpec *spec, loadQueue()) {
        spec->d->disableIndirectlyIfDependencyDisabled();
    }
}

 // Look in argument descriptions of the specs for the option.
PluginSpec *PluginManagerPrivate::pluginForOption(const QString &option, bool *requiresArgument) const
{
    // Look in the plugins for an option
    typedef PluginSpec::PluginArgumentDescriptions PluginArgumentDescriptions;

    *requiresArgument = false;
    const PluginSpecSet::const_iterator pcend = pluginSpecs.constEnd();
    for (PluginSpecSet::const_iterator pit = pluginSpecs.constBegin(); pit != pcend; ++pit) {
        PluginSpec *ps = *pit;
        const PluginArgumentDescriptions pargs = ps->argumentDescriptions();
        if (!pargs.empty()) {
            const PluginArgumentDescriptions::const_iterator acend = pargs.constEnd();
            for (PluginArgumentDescriptions::const_iterator ait = pargs.constBegin(); ait != acend; ++ait) {
                if (ait->name == option) {
                    *requiresArgument = !ait->parameter.isEmpty();
                    return ps;
                }
            }
        }
    }
    return 0;
}

void PluginManagerPrivate::disablePluginIndirectly(PluginSpec *spec)
{
    spec->d->disabledIndirectly = true;
}

PluginSpec *PluginManagerPrivate::pluginByName(const QString &name) const
{
    foreach (PluginSpec *spec, pluginSpecs)
        if (spec->name() == name)
            return spec;
    return 0;
}

void PluginManagerPrivate::initProfiling()
{
    if (m_profileTimer.isNull()) {
        m_profileTimer.reset(new QTime);
        m_profileTimer->start();
        m_profileElapsedMS = 0;
        qDebug("Profiling started");
    } else {
        m_profilingVerbosity++;
    }
}

void PluginManagerPrivate::profilingReport(const char *what, const PluginSpec *spec /* = 0 */)
{
    if (!m_profileTimer.isNull()) {
        const int absoluteElapsedMS = m_profileTimer->elapsed();
        const int elapsedMS = absoluteElapsedMS - m_profileElapsedMS;
        m_profileElapsedMS = absoluteElapsedMS;
        if (spec)
            m_profileTotal[spec] += elapsedMS;
        if (spec)
            qDebug("%-22s %-22s %8dms (%8dms)", what, qPrintable(spec->name()), absoluteElapsedMS, elapsedMS);
        else
            qDebug("%-45s %8dms (%8dms)", what, absoluteElapsedMS, elapsedMS);
    }
}

void PluginManagerPrivate::profilingSummary() const
{
    if (!m_profileTimer.isNull()) {
        typedef QMultiMap<int, const PluginSpec *> Sorter;
        Sorter sorter;
        int total = 0;

        QHash<const PluginSpec *, int>::ConstIterator it1 = m_profileTotal.constBegin();
        QHash<const PluginSpec *, int>::ConstIterator et1 = m_profileTotal.constEnd();
        for (; it1 != et1; ++it1) {
            sorter.insert(it1.value(), it1.key());
            total += it1.value();
        }

        Sorter::ConstIterator it2 = sorter.begin();
        Sorter::ConstIterator et2 = sorter.end();
        for (; it2 != et2; ++it2)
            qDebug("%-22s %8dms   ( %5.2f%% )", qPrintable(it2.value()->name()),
                it2.key(), 100.0 * it2.key() / total);
         qDebug("Total: %8dms", total);
    }
}

static inline QString getPlatformName()
{
#if defined(Q_OS_MAC)
    QString result = QLatin1String("Mac OS");
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_0)
        result += QLatin1String(" 10.") + QString::number(QSysInfo::MacintoshVersion - QSysInfo::MV_10_0);
    return result;
#elif defined(Q_OS_UNIX)
    QFile osReleaseFile(QLatin1String("/etc/os-release")); // Newer Linuxes
    if (osReleaseFile.open(QIODevice::ReadOnly)) {
        QString name;
        QString version;
        forever {
            const QByteArray line = osReleaseFile.readLine();
            if (line.isEmpty())
                break;
            if (line.startsWith("NAME=\""))
                name = QString::fromLatin1(line.mid(6, line.size() - 8)).trimmed();
            if (line.startsWith("VERSION_ID=\""))
                version = QString::fromLatin1(line.mid(12, line.size() - 14)).trimmed();
        }
        if (!name.isEmpty()) {
            if (!version.isEmpty())
                name += QLatin1Char(' ') + version;
            return name;
        }
    }
    QFile issueFile(QLatin1String("/etc/issue")); // Older Linuxes
    if (issueFile.open(QIODevice::ReadOnly)) {
        QByteArray issue = issueFile.readAll();
        const int end = issue.lastIndexOf(" \\n");
        if (end >= 0)
            issue.truncate(end);
        return QString::fromLatin1(issue).trimmed();
    }
#  ifdef Q_OS_LINUX
    return QLatin1String("Linux");
#  else
    return QLatin1String("Unix");
#  endif // Q_OS_LINUX
#elif defined(Q_OS_WIN)
    QString result = QLatin1String("Windows");
    switch (QSysInfo::WindowsVersion) {
    case QSysInfo::WV_XP:
        result += QLatin1String(" XP");
        break;
    case QSysInfo::WV_2003:
        result += QLatin1String(" 2003");
        break;
    case QSysInfo::WV_VISTA:
        result += QLatin1String(" Vista");
        break;
    case QSysInfo::WV_WINDOWS7:
        result += QLatin1String(" 7");
        break;
    default:
        break;
    }
    if (QSysInfo::WindowsVersion >= QSysInfo::WV_WINDOWS8)
        result += QLatin1String(" 8");
    return result;
#endif // Q_OS_WIN
    return QLatin1String("Unknown");
}

QString PluginManager::platformName()
{
    static const QString result = getPlatformName();
    return result;
}

/*!
    Retrieves one object with \a name from the object pool.
    \sa addObject()
*/

QObject *PluginManager::getObjectByName(const QString &name)
{
    QReadLocker lock(&d->m_lock);
    QList<QObject *> all = allObjects();
    foreach (QObject *obj, all) {
        if (obj->objectName() == name)
            return obj;
    }
    return 0;
}

/*!
    Retrieves one object inheriting a class with \a className from the object
    pool.
    \sa addObject()
*/

QObject *PluginManager::getObjectByClassName(const QString &className)
{
    const QByteArray ba = className.toUtf8();
    QReadLocker lock(&d->m_lock);
    QList<QObject *> all = allObjects();
    foreach (QObject *obj, all) {
        if (obj->inherits(ba.constData()))
            return obj;
    }
    return 0;
}
