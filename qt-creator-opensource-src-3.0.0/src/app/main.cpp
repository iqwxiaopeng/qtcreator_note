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

#include "../tools/qtcreatorcrashhandler/crashhandlersetup.h"

#include <app/app_version.h>
#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginerroroverview.h>
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>
#include <qtsingleapplication.h>
#include <utils/hostosinfo.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QSettings>
#include <QTextStream>
#include <QThreadPool>
#include <QTimer>
#include <QTranslator>
#include <QUrl>
#include <QVariant>

#include <QNetworkProxyFactory>

#include <QApplication>
#include <QDesktopServices>
#include <QMessageBox>

#ifdef ENABLE_QT_BREAKPAD
#include <qtsystemexceptionhandler.h>
#endif

using namespace ExtensionSystem;

enum { OptionIndent = 4, DescriptionIndent = 34 };

const char appNameC[] = "Qt Creator";
const char corePluginNameC[] = "Core";
const char fixedOptionsC[] =
" [OPTION]... [FILE]...\n"
"Options:\n"
"    -help                         Display this help\n"
"    -version                      Display program version\n"
"    -client                       Attempt to connect to already running first instance\n"
"    -settingspath <path>          Override the default path where user settings are stored\n"
"    -pid <pid>                    Attempt to connect to instance given by pid\n"
"    -block                        Block until editor is closed\n"
"    -pluginpath <path>            Add a custom search path for plugins\n";

const char HELP_OPTION1[] = "-h";
const char HELP_OPTION2[] = "-help";
const char HELP_OPTION3[] = "/h";
const char HELP_OPTION4[] = "--help";
const char VERSION_OPTION[] = "-version";
const char CLIENT_OPTION[] = "-client";
const char SETTINGS_OPTION[] = "-settingspath";
const char TEST_OPTION[] = "-test";
const char PID_OPTION[] = "-pid";
const char BLOCK_OPTION[] = "-block";
const char PLUGINPATH_OPTION[] = "-pluginpath";

typedef QList<PluginSpec *> PluginSpecSet;

// Helpers for displaying messages. Note that there is no console on Windows.
#ifdef Q_OS_WIN
// Format as <pre> HTML
static inline void toHtml(QString &t)
{
    t.replace(QLatin1Char('&'), QLatin1String("&amp;"));
    t.replace(QLatin1Char('<'), QLatin1String("&lt;"));
    t.replace(QLatin1Char('>'), QLatin1String("&gt;"));
    t.insert(0, QLatin1String("<html><pre>"));
    t.append(QLatin1String("</pre></html>"));
}

static void displayHelpText(QString t) // No console on Windows.
{
    toHtml(t);
    QMessageBox::information(0, QLatin1String(appNameC), t);
}

static void displayError(const QString &t) // No console on Windows.
{
    QMessageBox::critical(0, QLatin1String(appNameC), t);
}

#else

static void displayHelpText(const QString &t)
{
    qWarning("%s", qPrintable(t));
}

static void displayError(const QString &t)
{
    qCritical("%s", qPrintable(t));
}

#endif

static void printVersion(const PluginSpec *coreplugin)
{
    QString version;
    QTextStream str(&version);
    str << '\n' << appNameC << ' ' << coreplugin->version()<< " based on Qt " << qVersion() << "\n\n";
    PluginManager::formatPluginVersions(str);
    str << '\n' << coreplugin->copyright() << '\n';
    displayHelpText(version);
}

static void printHelp(const QString &a0)
{
    QString help;
    QTextStream str(&help);
    str << "Usage: " << a0 << fixedOptionsC;
    PluginManager::formatOptions(str, OptionIndent, DescriptionIndent);
    PluginManager::formatPluginOptions(str, OptionIndent, DescriptionIndent);
    displayHelpText(help);
}

static inline QString msgCoreLoadFailure(const QString &why)
{
    return QCoreApplication::translate("Application", "Failed to load core: %1").arg(why);
}

static inline int askMsgSendFailed()
{
    return QMessageBox::question(0, QApplication::translate("Application","Could not send message"),
                                 QCoreApplication::translate("Application", "Unable to send command line arguments to the already running instance. "
                                                             "It appears to be not responding. Do you want to start a new instance of Creator?"),
                                 QMessageBox::Yes | QMessageBox::No | QMessageBox::Retry,
                                 QMessageBox::Retry);
}

// taken from utils/fileutils.cpp. We can not use utils here since that depends app_version.h.
static bool copyRecursively(const QString &srcFilePath,
                            const QString &tgtFilePath)
{
    QFileInfo srcFileInfo(srcFilePath);
    if (srcFileInfo.isDir()) {
        QDir targetDir(tgtFilePath);
        targetDir.cdUp();
        if (!targetDir.mkdir(QFileInfo(tgtFilePath).fileName()))
            return false;
        QDir sourceDir(srcFilePath);
        QStringList fileNames = sourceDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        foreach (const QString &fileName, fileNames) {
            const QString newSrcFilePath
                    = srcFilePath + QLatin1Char('/') + fileName;
            const QString newTgtFilePath
                    = tgtFilePath + QLatin1Char('/') + fileName;
            if (!copyRecursively(newSrcFilePath, newTgtFilePath))
                return false;
        }
    } else {
        if (!QFile::copy(srcFilePath, tgtFilePath))
            return false;
    }
    return true;
}
//这个函数返回插件默认存放的路径有2个默认路径 1. 工程下的插件目录 2.系统用户数据区下的本软件的插件目录，并打上版本
static inline QStringList getPluginPaths()
{
    QStringList rc; //保存插件存放路径 1. 程序目录下的 /qtcreator/plugins 2. 用户存储区/QtProject/qtcreator/plugins/软件版本/
    // Figure out root:  Up one from 'bin' 拿到运行程序所在dir 此处调试就是sln文件所在目录
    QDir rootDir = QApplication::applicationDirPath();
    rootDir.cdUp();
    const QString rootDirPath = rootDir.canonicalPath();//获得与系统相关的路径也就是win,linux不一样的那种路径
#if !defined(Q_OS_MAC)
    // 1) "plugins" (Win/Linux) 根据程序的目录获得插件子目录
    QString pluginPath = rootDirPath;
    pluginPath += QLatin1Char('/');
    pluginPath += QLatin1String(IDE_LIBRARY_BASENAME);
    pluginPath += QLatin1String("/qtcreator/plugins");
    rc.push_back(pluginPath);
#else
    // 2) "PlugIns" (OS X)
    QString pluginPath = rootDirPath;
    pluginPath += QLatin1String("/PlugIns");
    rc.push_back(pluginPath);
#endif
    // 3) <localappdata>/plugins/<ideversion>
    //    where <localappdata> is e.g.
    //    "%LOCALAPPDATA%\QtProject\qtcreator" on Windows Vista and later
    //    "$XDG_DATA_HOME/data/QtProject/qtcreator" or "~/.local/share/data/QtProject/qtcreator" on Linux
    //    "~/Library/Application Support/QtProject/Qt Creator" on Mac 获得用户存储区路径 C:/Users/XP/AppData/Local/
    pluginPath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    pluginPath += QLatin1Char('/')
            + QLatin1String(Core::Constants::IDE_SETTINGSVARIANT_STR)
            + QLatin1Char('/');
#if !defined(Q_OS_MAC)
    pluginPath += QLatin1String("qtcreator");
#else
    pluginPath += QLatin1String("Qt Creator");
#endif
    pluginPath += QLatin1String("/plugins/");
    pluginPath += QLatin1String(Core::Constants::IDE_VERSION_LONG);
    rc.push_back(pluginPath);
    return rc;
}

//创建用户配置文件在用户配置区
static QSettings *createUserSettings()
{
	// C:/Users/usr/AppData/Roaming/QtProject/QtCreator.ini
    return new QSettings(QSettings::IniFormat, QSettings::UserScope,
                         QLatin1String(Core::Constants::IDE_SETTINGSVARIANT_STR),
                         QLatin1String("QtCreator"));
}

static inline QSettings *userSettings()
{
	//创建配置对象
    QSettings *settings = createUserSettings();
	//取出Nokia
    const QString fromVariant = QLatin1String(Core::Constants::IDE_COPY_SETTINGS_FROM_VARIANT_STR);
    if (fromVariant.isEmpty())
        return settings;
	
    // Copy old settings to new ones: 判断文件C:/Users/usr/AppData/Roaming/QtProject/QtCreator.ini是否已经存在
    QFileInfo pathFi = QFileInfo(settings->fileName()); //如果配置以及存在 则返回
    if (pathFi.exists()) // already copied.
        return settings;
	//创建路径目录
    QDir destDir = pathFi.absolutePath();
    if (!destDir.exists())
        destDir.mkpath(pathFi.absolutePath());
	//如果C:/Users/usr/AppData/Roaming/Nokia不存在 返回settings
    QDir srcDir = destDir;
    srcDir.cdUp();
    if (!srcDir.cd(fromVariant))
        return settings;

    if (srcDir == destDir) // Nothing to copy and no settings yet
        return settings;

    QStringList entries = srcDir.entryList();
    foreach (const QString &file, entries) {
        const QString lowerFile = file.toLower();
        if (lowerFile.startsWith(QLatin1String("profiles.xml"))
                || lowerFile.startsWith(QLatin1String("toolchains.xml"))
                || lowerFile.startsWith(QLatin1String("qtversion.xml"))
                || lowerFile.startsWith(QLatin1String("devices.xml"))
                || lowerFile.startsWith(QLatin1String("debuggers.xml"))
                || lowerFile.startsWith(QLatin1String("qtcreator.")))
            QFile::copy(srcDir.absoluteFilePath(file), destDir.absoluteFilePath(file));
        if (file == QLatin1String("qtcreator"))
            copyRecursively(srcDir.absoluteFilePath(file), destDir.absoluteFilePath(file));
    }

    // Make sure to use the copied settings:
    delete settings;
    return createUserSettings();
}

#ifdef Q_OS_MAC
#  define SHARE_PATH "/../Resources"
#else
#  define SHARE_PATH "/../share/qtcreator"
#endif

int main(int argc, char **argv)
{
#ifdef Q_OS_MAC
    // increase the number of file that can be opened in Qt Creator.
    struct rlimit rl;
    getrlimit(RLIMIT_NOFILE, &rl);

    rl.rlim_cur = qMin((rlim_t)OPEN_MAX, rl.rlim_max);
    setrlimit(RLIMIT_NOFILE, &rl);
#endif

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    // QML is unusable with the xlib backend
    QApplication::setGraphicsSystem(QLatin1String("raster"));
#endif

	//初始化一个app实例
    SharedTools::QtSingleApplication app((QLatin1String(appNameC)), argc, argv);

	//获得计算机CPU线程数 比如4核心8线程
    const int threadCount = QThreadPool::globalInstance()->maxThreadCount();

	//设置线程池最大开启线程数
    QThreadPool::globalInstance()->setMaxThreadCount(qMax(4, 2 * threadCount));

	//设置程序崩溃回调
    setupCrashHandler(); // Display a backtrace once a serious signal is delivered.

#ifdef ENABLE_QT_BREAKPAD
    QtSystemExceptionHandler systemExceptionHandler;
#endif

	//5.1版本以上 设置支持高分辨率 更好的支持Retina显示,否则会显示模糊
#if QT_VERSION >= 0x050100
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    // Manually determine -settingspath command line option
    // We can't use the regular way of the plugin manager, because that needs to parse pluginspecs
    // but the settings path can influence which plugins are enabled
	//以下代码用来解析程序输入参数
	//为了方便对比,把命令参数截取到如下注释
	//const char HELP_OPTION1[] = "-h";
	//const char HELP_OPTION2[] = "-help";
	//const char HELP_OPTION3[] = "/h";
	//const char HELP_OPTION4[] = "--help";
	//const char VERSION_OPTION[] = "-version";
	//const char CLIENT_OPTION[] = "-client";
	//const char SETTINGS_OPTION[] = "-settingspath";
	//const char TEST_OPTION[] = "-test";
	//const char PID_OPTION[] = "-pid";
	//const char BLOCK_OPTION[] = "-block";
	//const char PLUGINPATH_OPTION[] = "-pluginpath";

    QString settingsPath; //保存配置目录
    QStringList customPluginPaths; //保存插件

	//得到参数列表
    QStringList arguments = app.arguments(); // adapted arguments list is passed to plugin manager later
    QMutableStringListIterator it(arguments);
    bool testOptionProvided = false; //是否测试标记
    while (it.hasNext()) {//挨个遍历输入参数
        const QString &arg = it.next();
        if (arg == QLatin1String(SETTINGS_OPTION)) { //解析配置路径命令
            it.remove();
            if (it.hasNext()) { //解析具体的命令
                settingsPath = QDir::fromNativeSeparators(it.next()); //得到命令 路径中的分隔符转换为具体系统分隔符
                it.remove();
            }
        } else if (arg == QLatin1String(PLUGINPATH_OPTION)) { //解析插件命令
            it.remove();
            if (it.hasNext()) {
                customPluginPaths << QDir::fromNativeSeparators(it.next());
                it.remove();
            }
        } else if (arg == QLatin1String(TEST_OPTION)) { //测试命令
            testOptionProvided = true;
        }
    }
	//如果用户没有设置配置路径并且是测试模式,则在系统临时目录新建一个目录作为配置路径
    if (settingsPath.isEmpty() && testOptionProvided) {
        settingsPath = QDir::tempPath() + QString::fromLatin1("/qtc-%1-test-settings")
                .arg(QLatin1String(Core::Constants::IDE_VERSION_LONG));
        settingsPath = QDir::cleanPath(settingsPath);
    }

    //QSettings::SystemScope ,QSettings::UserScope该枚举指定设置是否用户特定或同一系统的所有用户共享;
	//UserScope              %APPDATA%\*.ini
	//SystemScope         %COMMON_APPDATA%\*.ini

	//设置目录非空 则设置INI配置文件用户主目录 为配置目录
    if (!settingsPath.isEmpty())
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsPath);

    // Must be done before any QSettings class is created
	//设置 INI 配置文件全局目录为 qt-creator-opensource-src-3.0.0/src/app/../../bin/../share/qtcreator
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope,
                       QCoreApplication::applicationDirPath() + QLatin1String(SHARE_PATH));

	//设置配置默认格式为ini
    QSettings::setDefaultFormat(QSettings::IniFormat);

	// C:/Users/usr/AppData/Roaming/QtProject 创建了这个文件夹
    // plugin manager takes control of this settings object
    QSettings *settings = userSettings();

    QSettings *globalSettings = new QSettings(QSettings::IniFormat, QSettings::SystemScope,
                                              QLatin1String(Core::Constants::IDE_SETTINGSVARIANT_STR),
                                              QLatin1String("QtCreator"));
    PluginManager pluginManager;

	//设置文件扩展名 默认调用子类的
    PluginManager::setFileExtension(QLatin1String("pluginspec"));

	//设置插件管理器的全局和普通配置对象,被设置的配置对象会作为管理器的孩子节点
    PluginManager::setGlobalSettings(globalSettings);
    PluginManager::setSettings(settings);

    QTranslator translator;
    QTranslator qtTranslator;
    QStringList uiLanguages;

	//获取系统支持的语言
// uiLanguages crashes on Windows with 4.8.0 release builds
#if (QT_VERSION >= 0x040801) || (QT_VERSION >= 0x040800 && !defined(Q_OS_WIN))
    uiLanguages = QLocale::system().uiLanguages();
#else
    uiLanguages << QLocale::system().name();
#endif
    //C:/Users/usr/AppData/Roaming/QtProject  如果用户已经有了之前保留的语言设置 则添加用户的到语言列表
    QString overrideLanguage = settings->value(QLatin1String("General/OverrideLanguage")).toString();
    if (!overrideLanguage.isEmpty())
        uiLanguages.prepend(overrideLanguage);

	//翻译目录为 C:/qt-creator-opensource-src-3.0.0/src/app/../../bin/../share/qtcreator/translations
    const QString &creatorTrPath = QCoreApplication::applicationDirPath()
            + QLatin1String(SHARE_PATH "/translations");

    foreach (QString locale, uiLanguages) {
#if (QT_VERSION >= 0x050000)
		//locale 为语言名 zh_CN 之类
        locale = QLocale(locale).name();
#else
        locale.replace(QLatin1Char('-'), QLatin1Char('_')); // work around QTBUG-25973
#endif
		//加载C:/qt-creator-opensource-src-3.0.0/src/app/../../bin/../share/qtcreator/translations/qtcreator_zh_CN
        if (translator.load(QLatin1String("qtcreator_") + locale, creatorTrPath)) {
            const QString &qtTrPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
            const QString &qtTrFile = QLatin1String("qt_") + locale;
            // Binary installer puts Qt tr files into creatorTrPath
            if (qtTranslator.load(qtTrFile, qtTrPath) || qtTranslator.load(qtTrFile, creatorTrPath)) {
                app.installTranslator(&translator);
                app.installTranslator(&qtTranslator);
                app.setProperty("qtc_locale", locale);
                break;
            }
            translator.load(QString()); // unload()
        } else if (locale == QLatin1String("C") /* overrideLanguage == "English" */) {
            // use built-in
            break;
        } else if (locale.startsWith(QLatin1String("en")) /* "English" is built-in */) {
            // use built-in
            break;
        }
    }

    // Make sure we honor the system's proxy settings
#if defined(Q_OS_UNIX)
    QUrl proxyUrl(QString::fromLatin1(qgetenv("http_proxy")));
    if (proxyUrl.isValid()) {
        QNetworkProxy proxy(QNetworkProxy::HttpProxy, proxyUrl.host(),
                            proxyUrl.port(), proxyUrl.userName(), proxyUrl.password());
        QNetworkProxy::setApplicationProxy(proxy);
# if defined(Q_OS_MAC) // unix and mac
    } else {
        QNetworkProxyFactory::setUseSystemConfiguration(true);
# endif
    }
#else // windows 启动特定平台此处是win的代理设置
    QNetworkProxyFactory::setUseSystemConfiguration(true);
#endif
    // Load 插件存放路径为默认路径+用户指定的路径
    const QStringList pluginPaths = getPluginPaths() + customPluginPaths;

    //设置插件搜索目录 会去read目录加载插件忽略项,强制加载项
    PluginManager::setPluginPaths(pluginPaths);

    QMap<QString, QString> foundAppOptions;
    if (arguments.size() > 1) {
        QMap<QString, bool> appOptions;
        appOptions.insert(QLatin1String(HELP_OPTION1), false);
        appOptions.insert(QLatin1String(HELP_OPTION2), false);
        appOptions.insert(QLatin1String(HELP_OPTION3), false);
        appOptions.insert(QLatin1String(HELP_OPTION4), false);
        appOptions.insert(QLatin1String(VERSION_OPTION), false);
        appOptions.insert(QLatin1String(CLIENT_OPTION), false);
        appOptions.insert(QLatin1String(PID_OPTION), true);
        appOptions.insert(QLatin1String(BLOCK_OPTION), false);
        QString errorMessage;
        if (!PluginManager::parseOptions(arguments, appOptions, &foundAppOptions, &errorMessage)) {
            displayError(errorMessage);
            printHelp(QFileInfo(app.applicationFilePath()).baseName());
            return -1;
        }
    }

    const PluginSpecSet plugins = PluginManager::plugins(); //获取搜索到的所有插件集
    PluginSpec *coreplugin = 0;
    foreach (PluginSpec *spec, plugins) { //检索Core插件的描述对象
        if (spec->name() == QLatin1String(corePluginNameC)) {
            coreplugin = spec;
            break;
        }
    }
    //没有找到core插件描述类 则报错核心插件必须加载
    if (!coreplugin) {
        QString nativePaths = QDir::toNativeSeparators(pluginPaths.join(QLatin1String(",")));
        const QString reason = QCoreApplication::translate("Application", "Could not find 'Core.pluginspec' in %1").arg(nativePaths);
        displayError(msgCoreLoadFailure(reason));
        return 1;
    }
    //core插件有错误不适合 报错
    if (coreplugin->hasError()) {
        displayError(msgCoreLoadFailure(coreplugin->errorString()));
        return 1;
    }

    /**********************************************************************************/
    //与foundAppOptions相关的暂时不管 因为我们没有从args传参数
    if (foundAppOptions.contains(QLatin1String(VERSION_OPTION))) {
        printVersion(coreplugin);
        return 0;
    }
    if (foundAppOptions.contains(QLatin1String(HELP_OPTION1))
            || foundAppOptions.contains(QLatin1String(HELP_OPTION2))
            || foundAppOptions.contains(QLatin1String(HELP_OPTION3))
            || foundAppOptions.contains(QLatin1String(HELP_OPTION4))) {
        printHelp(QFileInfo(app.applicationFilePath()).baseName());
        return 0;
    }

    qint64 pid = -1;
    if (foundAppOptions.contains(QLatin1String(PID_OPTION))) {
        QString pidString = foundAppOptions.value(QLatin1String(PID_OPTION));
        bool pidOk;
        qint64 tmpPid = pidString.toInt(&pidOk);
        if (pidOk)
            pid = tmpPid;
    }

   
    bool isBlock = foundAppOptions.contains(QLatin1String(BLOCK_OPTION));
    if (app.isRunning() && (pid != -1 || isBlock
                            || foundAppOptions.contains(QLatin1String(CLIENT_OPTION)))) {
        app.setBlock(isBlock);
        if (app.sendMessage(PluginManager::serializedArguments(), 5000 /*timeout*/, pid))
            return 0;

        // Message could not be send, maybe it was in the process of quitting
        if (app.isRunning(pid)) {
            // Nah app is still running, ask the user
            int button = askMsgSendFailed();
            while (button == QMessageBox::Retry) {
                if (app.sendMessage(PluginManager::serializedArguments(), 5000 /*timeout*/, pid))
                    return 0;
                if (!app.isRunning(pid)) // App quit while we were trying so start a new creator
                    button = QMessageBox::Yes;
                else
                    button = askMsgSendFailed();
            }
            if (button == QMessageBox::No)
                return -1;
        }
    }
   //以上代码内容基本只有在设置了程序启动参数才会执行 因此第一步不需要阅读
   /**********************************************************************************/


    PluginManager::loadPlugins();
    if (coreplugin->hasError()) {
        displayError(msgCoreLoadFailure(coreplugin->errorString()));
        return 1;
    }
    if (PluginManager::hasError()) {
        PluginErrorOverview *errorOverview = new PluginErrorOverview(QApplication::activeWindow());
        errorOverview->setAttribute(Qt::WA_DeleteOnClose);
        errorOverview->setModal(true);
        errorOverview->show();
    }

	//将主程序运行过程中得到的消息传递给插件管理器最终给核心插件
    // Set up remote arguments. 有数据到来 交给 插件管理器的remoteArguments函数处理
    QObject::connect(&app, SIGNAL(messageReceived(QString,QObject*)),
                     &pluginManager, SLOT(remoteArguments(QString,QObject*)));

    //app的fileOpenRequest交给coreplugin的 fileOpenRequest处理
    QObject::connect(&app, SIGNAL(fileOpenRequest(QString)), coreplugin->plugin(),
                     SLOT(fileOpenRequest(QString)));

    // quit when last window (relevant window, see WA_QuitOnClose) is closed
    // this should actually be the default, but doesn't work in Qt 5
    // QTBUG-31569
    QObject::connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));
    // shutdown plugin manager on the exit
    QObject::connect(&app, SIGNAL(aboutToQuit()), &pluginManager, SLOT(shutdown()));

    const int r = app.exec();//进入消息循环

    //移除崩溃处理句柄
    cleanupCrashHandler();
    return r;
}
