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

#ifndef ICORE_H
#define ICORE_H

#include "core_global.h"
#include "id.h"

#include <QObject>
#include <QSettings>

QT_BEGIN_NAMESPACE
class QPrinter;
class QStatusBar;
class QWidget;
template <class T> class QList;
QT_END_NAMESPACE

namespace Core {
class IWizard;
class Context;
class IContext;
class ProgressManager;
class SettingsDatabase;
class VcsManager;

namespace Internal { class MainWindow; }
//核心库提供的基本功能封装 单例类
class CORE_EXPORT ICore : public QObject
{
    Q_OBJECT

    friend class Internal::MainWindow;
    explicit ICore(Internal::MainWindow *mw);
    ~ICore();

public:
    // This should only be used to acccess the signals, so it could
    // theoretically return an QObject *. For source compatibility
    // it returns a ICore.
    static ICore *instance();

    //打开一个对话框，用户可以从创建新文件、类或项目的一组向导中进行选择。
    //标题参数显示为对话框标题。将创建文件的路径（如果用户不更改）设置在默认位置。它默认为文件管理器当前文件的路径
    static void showNewItemDialog(const QString &title,
                                  const QList<IWizard *> &wizards,
                                  const QString &defaultLocation = QString(),
                                  const QVariantMap &extraVariables = QVariantMap());

    //打开“应用程序GUI选项”（或“GUI首选项”）对话框，并在指定组中预先选择页面。
    //这些参数引用相应IOptionsPage的字符串ID。
    static bool showOptionsDialog(Id group, Id page, QWidget *parent = 0);

    //显示一条警告消息，其中包含一个打开设置页的按钮。
    //应用于显示配置错误并将用户指向设置。如果接受设置对话框，则返回true。
    static bool showWarningWithOptions(const QString &title, const QString &text,
                                       const QString &details = QString(),
                                       Id settingsCategory = Id(),
                                       Id settingsId = Id(),
                                       QWidget *parent = 0);

    //返回应用程序的主设置对象。您可以使用它来检索或设置应用程序范围的设置（与会话或项目特定的设置不同）。
    //如果范围是qsettings : ：userscope（默认值），则将从用户设置中读取用户设置，并返回到qc提供的全局设置。
    //如果范围是qsettings : ：system scope，则只读取当前版本qc附带的系统设置。此功能仅用于内部目的。
    static QSettings *settings(QSettings::Scope scope = QSettings::UserScope);

    //返回应用程序的设置数据库。
    //设置数据库用于替代常规设置对象。它更适合存储大量数据。这些设置在应用程序范围内。
    static SettingsDatabase *settingsDatabase();

    //返回应用程序的打印机对象。
    //始终使用此打印机对象进行打印，因此应用程序的不同部分重新使用其设置。
    static QPrinter *printer();


    static QString userInterfaceLanguage();

    //返回用于项目模板和调试器宏等资源的绝对路径。
    //需要这种抽象来避免各地的平台特定代码，因为在Mac OS X上，例如，资源是应用程序包的一部分。
    static QString resourcePath();

    //返回用户目录中用于项目模板等资源的绝对路径。
    //使用此函数可以查找用户可以写入的资源位置，例如，允许自定义调色板或模板。
    static QString userResourcePath();
    static QString documentationPath();
    static QString libexecPath();

    static QString versionString();
    static QString buildCompatibilityString();

    //返回主应用程序窗口。用作对话框父级，等等。
    static QWidget *mainWindow();
    static QStatusBar *statusBar();
    /* Raises and activates the window for the widget. This contains workarounds for X11. */
    static void raiseWindow(QWidget *widget);

    //返回当前主上下文的上下文对象。
    static IContext *currentContextObject();

    //更改当前活动的其他上下文。
    //删除由remove指定的附加上下文列表，并添加由add指定的附加上下文列表。
    // Adds and removes additional active contexts, these contexts are appended
    // to the currently active contexts.
    static void updateAdditionalContexts(const Context &remove, const Context &add);

    //注册其他上下文对象。
    //注册后，只要控件获得焦点，此上下文对象就会自动获得当前上下文对象。
    static void addContextObject(IContext *context);
    //从已知上下文列表中注销上下文对象。
    static void removeContextObject(IContext *context);

    enum OpenFilesFlags {
        None = 0,
        SwitchMode = 1,
        CanContainLineNumbers = 2,
         /// Stop loading once the first file fails to load
        StopOnLoadFail = 4
    };
    static void openFiles(const QStringList &fileNames, OpenFilesFlags flags = None);

    static void emitNewItemsDialogRequested();

    static void saveSettings();

signals:
    void coreAboutToOpen();
    void coreOpened();//指示已加载所有插件并显示主窗口。
    void newItemsDialogRequested();
    void saveSettingsRequested(); //表示用户已请求将全局设置保存到磁盘。当应用程序关闭时，或者GUI保存所有时发生这种情况。
    void optionsDialogRequested(); //允许插件在显示GUI工具>GUI选项对话框之前执行操作。

    //允许插件执行一些生命周期结束前的操作。
    //该应用程序保证在发出该信号后关闭。
    //为了方便起见，它是对普通插件生命周期函数（即iplugin : ：aboutToShutdown（））的一个补充。
    void coreAboutToClose();
    void contextAboutToChange(const QList<Core::IContext *> &context); //指示新上下文将很快成为当前上下文（意味着其小部件获得焦点）。

    //指示新上下文刚刚成为当前上下文（意味着其小部件获得焦点），或者由AdditionalContexts指定的其他上下文ID已更改。
    void contextChanged(const QList<Core::IContext *> &context, const Core::Context &additionalContexts);
};

} // namespace Core

#endif // ICORE_H
