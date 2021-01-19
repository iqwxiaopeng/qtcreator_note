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
//����������ز��Ĭ�ϴ�ŵ�·����2��Ĭ��·�� 1. �����µĲ��Ŀ¼ 2.ϵͳ�û��������µı������Ĳ��Ŀ¼�������ϰ汾
static inline QStringList getPluginPaths()
{
    QStringList rc; //���������·�� 1. ����Ŀ¼�µ� /qtcreator/plugins 2. �û��洢��/QtProject/qtcreator/plugins/�����汾/
    // Figure out root:  Up one from 'bin' �õ����г�������dir �˴����Ծ���sln�ļ�����Ŀ¼
    QDir rootDir = QApplication::applicationDirPath();
    rootDir.cdUp();
    const QString rootDirPath = rootDir.canonicalPath();//�����ϵͳ��ص�·��Ҳ����win,linux��һ��������·��
#if !defined(Q_OS_MAC)
    // 1) "plugins" (Win/Linux) ���ݳ����Ŀ¼��ò����Ŀ¼
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
    //    "~/Library/Application Support/QtProject/Qt Creator" on Mac ����û��洢��·�� C:/Users/XP/AppData/Local/
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

//�����û������ļ����û�������
static QSettings *createUserSettings()
{
	// C:/Users/usr/AppData/Roaming/QtProject/QtCreator.ini
    return new QSettings(QSettings::IniFormat, QSettings::UserScope,
                         QLatin1String(Core::Constants::IDE_SETTINGSVARIANT_STR),
                         QLatin1String("QtCreator"));
}

static inline QSettings *userSettings()
{
	//�������ö���
    QSettings *settings = createUserSettings();
	//ȡ��Nokia
    const QString fromVariant = QLatin1String(Core::Constants::IDE_COPY_SETTINGS_FROM_VARIANT_STR);
    if (fromVariant.isEmpty())
        return settings;
	
    // Copy old settings to new ones: �ж��ļ�C:/Users/usr/AppData/Roaming/QtProject/QtCreator.ini�Ƿ��Ѿ�����
    QFileInfo pathFi = QFileInfo(settings->fileName()); //��������Լ����� �򷵻�
    if (pathFi.exists()) // already copied.
        return settings;
	//����·��Ŀ¼
    QDir destDir = pathFi.absolutePath();
    if (!destDir.exists())
        destDir.mkpath(pathFi.absolutePath());
	//���C:/Users/usr/AppData/Roaming/Nokia������ ����settings
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

	//��ʼ��һ��appʵ��
    SharedTools::QtSingleApplication app((QLatin1String(appNameC)), argc, argv);

	//��ü����CPU�߳��� ����4����8�߳�
    const int threadCount = QThreadPool::globalInstance()->maxThreadCount();

	//�����̳߳�������߳���
    QThreadPool::globalInstance()->setMaxThreadCount(qMax(4, 2 * threadCount));

	//���ó�������ص�
    setupCrashHandler(); // Display a backtrace once a serious signal is delivered.

#ifdef ENABLE_QT_BREAKPAD
    QtSystemExceptionHandler systemExceptionHandler;
#endif

	//5.1�汾���� ����֧�ָ߷ֱ��� ���õ�֧��Retina��ʾ,�������ʾģ��
#if QT_VERSION >= 0x050100
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    // Manually determine -settingspath command line option
    // We can't use the regular way of the plugin manager, because that needs to parse pluginspecs
    // but the settings path can influence which plugins are enabled
	//���´����������������������
	//Ϊ�˷���Ա�,�����������ȡ������ע��
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

    QString settingsPath; //��������Ŀ¼
    QStringList customPluginPaths; //������

	//�õ������б�
    QStringList arguments = app.arguments(); // adapted arguments list is passed to plugin manager later
    QMutableStringListIterator it(arguments);
    bool testOptionProvided = false; //�Ƿ���Ա��
    while (it.hasNext()) {//���������������
        const QString &arg = it.next();
        if (arg == QLatin1String(SETTINGS_OPTION)) { //��������·������
            it.remove();
            if (it.hasNext()) { //�������������
                settingsPath = QDir::fromNativeSeparators(it.next()); //�õ����� ·���еķָ���ת��Ϊ����ϵͳ�ָ���
                it.remove();
            }
        } else if (arg == QLatin1String(PLUGINPATH_OPTION)) { //�����������
            it.remove();
            if (it.hasNext()) {
                customPluginPaths << QDir::fromNativeSeparators(it.next());
                it.remove();
            }
        } else if (arg == QLatin1String(TEST_OPTION)) { //��������
            testOptionProvided = true;
        }
    }
	//����û�û����������·�������ǲ���ģʽ,����ϵͳ��ʱĿ¼�½�һ��Ŀ¼��Ϊ����·��
    if (settingsPath.isEmpty() && testOptionProvided) {
        settingsPath = QDir::tempPath() + QString::fromLatin1("/qtc-%1-test-settings")
                .arg(QLatin1String(Core::Constants::IDE_VERSION_LONG));
        settingsPath = QDir::cleanPath(settingsPath);
    }

    //QSettings::SystemScope ,QSettings::UserScope��ö��ָ�������Ƿ��û��ض���ͬһϵͳ�������û�����;
	//UserScope              %APPDATA%\*.ini
	//SystemScope         %COMMON_APPDATA%\*.ini

	//����Ŀ¼�ǿ� ������INI�����ļ��û���Ŀ¼ Ϊ����Ŀ¼
    if (!settingsPath.isEmpty())
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsPath);

    // Must be done before any QSettings class is created
	//���� INI �����ļ�ȫ��Ŀ¼Ϊ qt-creator-opensource-src-3.0.0/src/app/../../bin/../share/qtcreator
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope,
                       QCoreApplication::applicationDirPath() + QLatin1String(SHARE_PATH));

	//��������Ĭ�ϸ�ʽΪini
    QSettings::setDefaultFormat(QSettings::IniFormat);

	// C:/Users/usr/AppData/Roaming/QtProject ����������ļ���
    // plugin manager takes control of this settings object
    QSettings *settings = userSettings();

    QSettings *globalSettings = new QSettings(QSettings::IniFormat, QSettings::SystemScope,
                                              QLatin1String(Core::Constants::IDE_SETTINGSVARIANT_STR),
                                              QLatin1String("QtCreator"));
    PluginManager pluginManager;

	//�����ļ���չ�� Ĭ�ϵ��������
    PluginManager::setFileExtension(QLatin1String("pluginspec"));

	//���ò����������ȫ�ֺ���ͨ���ö���,�����õ����ö������Ϊ�������ĺ��ӽڵ�
    PluginManager::setGlobalSettings(globalSettings);
    PluginManager::setSettings(settings);

    QTranslator translator;
    QTranslator qtTranslator;
    QStringList uiLanguages;

	//��ȡϵͳ֧�ֵ�����
// uiLanguages crashes on Windows with 4.8.0 release builds
#if (QT_VERSION >= 0x040801) || (QT_VERSION >= 0x040800 && !defined(Q_OS_WIN))
    uiLanguages = QLocale::system().uiLanguages();
#else
    uiLanguages << QLocale::system().name();
#endif
    //C:/Users/usr/AppData/Roaming/QtProject  ����û��Ѿ�����֮ǰ�������������� �������û��ĵ������б�
    QString overrideLanguage = settings->value(QLatin1String("General/OverrideLanguage")).toString();
    if (!overrideLanguage.isEmpty())
        uiLanguages.prepend(overrideLanguage);

	//����Ŀ¼Ϊ C:/qt-creator-opensource-src-3.0.0/src/app/../../bin/../share/qtcreator/translations
    const QString &creatorTrPath = QCoreApplication::applicationDirPath()
            + QLatin1String(SHARE_PATH "/translations");

    foreach (QString locale, uiLanguages) {
#if (QT_VERSION >= 0x050000)
		//locale Ϊ������ zh_CN ֮��
        locale = QLocale(locale).name();
#else
        locale.replace(QLatin1Char('-'), QLatin1Char('_')); // work around QTBUG-25973
#endif
		//����C:/qt-creator-opensource-src-3.0.0/src/app/../../bin/../share/qtcreator/translations/qtcreator_zh_CN
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
#else // windows �����ض�ƽ̨�˴���win�Ĵ�������
    QNetworkProxyFactory::setUseSystemConfiguration(true);
#endif
    // Load ������·��ΪĬ��·��+�û�ָ����·��
    const QStringList pluginPaths = getPluginPaths() + customPluginPaths;

    //���ò������Ŀ¼ ��ȥreadĿ¼���ز��������,ǿ�Ƽ�����
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

    const PluginSpecSet plugins = PluginManager::plugins(); //��ȡ�����������в����
    PluginSpec *coreplugin = 0;
    foreach (PluginSpec *spec, plugins) { //����Core�������������
        if (spec->name() == QLatin1String(corePluginNameC)) {
            coreplugin = spec;
            break;
        }
    }
    //û���ҵ�core��������� �򱨴����Ĳ���������
    if (!coreplugin) {
        QString nativePaths = QDir::toNativeSeparators(pluginPaths.join(QLatin1String(",")));
        const QString reason = QCoreApplication::translate("Application", "Could not find 'Core.pluginspec' in %1").arg(nativePaths);
        displayError(msgCoreLoadFailure(reason));
        return 1;
    }
    //core����д����ʺ� ����
    if (coreplugin->hasError()) {
        displayError(msgCoreLoadFailure(coreplugin->errorString()));
        return 1;
    }

    /**********************************************************************************/
    //��foundAppOptions��ص���ʱ���� ��Ϊ����û�д�args������
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
   //���ϴ������ݻ���ֻ���������˳������������Ż�ִ�� ��˵�һ������Ҫ�Ķ�
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

	//�����������й����еõ�����Ϣ���ݸ�������������ո����Ĳ��
    // Set up remote arguments. �����ݵ��� ���� �����������remoteArguments��������
    QObject::connect(&app, SIGNAL(messageReceived(QString,QObject*)),
                     &pluginManager, SLOT(remoteArguments(QString,QObject*)));

    //app��fileOpenRequest����coreplugin�� fileOpenRequest����
    QObject::connect(&app, SIGNAL(fileOpenRequest(QString)), coreplugin->plugin(),
                     SLOT(fileOpenRequest(QString)));

    // quit when last window (relevant window, see WA_QuitOnClose) is closed
    // this should actually be the default, but doesn't work in Qt 5
    // QTBUG-31569
    QObject::connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));
    // shutdown plugin manager on the exit
    QObject::connect(&app, SIGNAL(aboutToQuit()), &pluginManager, SLOT(shutdown()));

    const int r = app.exec();//������Ϣѭ��

    //�Ƴ������������
    cleanupCrashHandler();
    return r;
}