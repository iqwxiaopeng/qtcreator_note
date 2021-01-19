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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "icontext.h"
#include "icore.h"

#include <utils/appmainwindow.h>

#include <QMap>
#include <QColor>

QT_BEGIN_NAMESPACE
class QSettings;
class QShortcut;
class QPrinter;
class QToolButton;
QT_END_NAMESPACE

namespace Core {

class ActionManager;
class StatusBarWidget;
class EditorManager;
class ExternalToolManager;
class DocumentManager;
class HelpManager;
class IDocument;
class IWizard;
class MessageManager;
class MimeDatabase;
class ModeManager;
class ProgressManager;
class NavigationWidget;
class RightPaneWidget;
class SettingsDatabase;
class VariableManager;
class VcsManager;

namespace Internal {

class ActionManagerPrivate;
class FancyTabWidget;
class GeneralSettings;
class ProgressManagerPrivate;
class ShortcutSettings;
class ToolSettings;
class MimeTypeSettings;
class StatusBarManager;
class VersionDialog;
class SystemEditor;

class MainWindow : public Utils::AppMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();

    bool init(QString *errorMessage);
    void extensionsInitialized();
    void aboutToShutdown();

    IContext *contextObject(QWidget *widget);
    void addContextObject(IContext *contex);
    void removeContextObject(IContext *contex);

    Core::IDocument *openFiles(const QStringList &fileNames, ICore::OpenFilesFlags flags);

    QSettings *settings(QSettings::Scope scope) const;
    inline SettingsDatabase *settingsDatabase() const { return m_settingsDatabase; }
    virtual QPrinter *printer() const;
    IContext * currentContextObject() const;
    QStatusBar *statusBar() const;

    void updateAdditionalContexts(const Context &remove, const Context &add);

    void setSuppressNavigationWidget(bool suppress);

	//设置主题色彩风格
    void setOverrideColor(const QColor &color);

	//设置是否全屏
    void setIsFullScreen(bool fullScreen);
signals:
    void windowActivated();

public slots:
    void newFile();//菜单栏目 File->NewFile 事件
    void openFileWith();//菜单栏目 File->OpenFileWith 事件
    void exit();//菜单栏目 File->Exit 事件
    void setFullScreen(bool on);//菜单栏目 Window->Full Screen 事件

    //菜单栏目 File->NewFile...事件具体调用
    void showNewItemDialog(const QString &title,
                           const QList<IWizard *> &wizards,
                           const QString &defaultLocation = QString(),
                           const QVariantMap &extraVariables = QVariantMap());
    //菜单栏目 Tools->Options Tools->External->Configure...事件
    bool showOptionsDialog(Id category = Id(), Id page = Id(), QWidget *parent = 0);

    bool showWarningWithOptions(const QString &title, const QString &text,
                                const QString &details = QString(),
                                Id settingsCategory = Id(),
                                Id settingsId = Id(),
                                QWidget *parent = 0);

protected:
    //change event一般是当前widget状态改变后触发的如字体改变,语言改变之类的.该方法主要捕获改变事件,当语言改变后,执行相关操作
    virtual void changeEvent(QEvent *e);

    //closeEvent 窗口关闭调用
    virtual void closeEvent(QCloseEvent *event);

    //当鼠标拖拽进入事件 EG拖拽一个文件进编辑器 拖放操作分为两个截然不同的动作: 拖动和放下.dragEnterEvent 与 dropEvent
    virtual void dragEnterEvent(QDragEnterEvent *event);
    //拖放操作分为两个截然不同的动作: 拖动和放下.dragEnterEvent 与 dropEvent
    virtual void dropEvent(QDropEvent *event);

private slots:
    void openFile(); //菜单栏目 File->OpenFile 事件
    void aboutToShowRecentFiles();//处理File菜单的aboutToShow信号 此信号在菜单显示给用户之前发出,就是点击File菜单立刻会先调用这个函数
    void openRecentFile();//菜单栏目 File->OpenRecentFile 事件
    void setFocusToEditor();//按键 esc 事件处理
    void saveAll();//菜单栏目 File->Save All 事件
    void aboutQtCreator();//菜单栏目 Help->AboutQtCreator 事件
    void aboutPlugins();//菜单栏目 Help->AboutPlugins 事件
    void updateFocusWidget(QWidget *old, QWidget *now);//当系统焦点发生改变的时候，就会发出focusChanged信号,用updateFocusWidget函数处理这个信号
    void setSidebarVisible(bool visible);//菜单栏 Window->Show Silderbar事件
    void destroyVersionDialog();//About->About QtCreator菜单项弹出的关于对话框的关闭操作事件
    void openDelayedFiles();

private:
    void updateContextObject(const QList<IContext *> &context);
    void updateContext();

    void registerDefaultContainers();//菜单栏目的设计布局构建生成
    void registerDefaultActions();//创建几个在布局时没创建的菜单子项目,并为菜单栏各个项目创建并添加和绑定操作同时注册到操作管理器

    void readSettings();
    void writeSettings();

    ICore *m_coreImpl;
    Context m_additionalContexts;
    QSettings *m_settings; //用户配置文件
    QSettings *m_globalSettings; //全局配置文件
    SettingsDatabase *m_settingsDatabase; //根据用户配置文件创建的配置数据库
    mutable QPrinter *m_printer; //QT的打印类
    ActionManager *m_actionManager;//命令管理类对象,注册Action获得命令对象
    EditorManager *m_editorManager;
    ExternalToolManager *m_externalToolManager;
    MessageManager *m_messageManager;
    ProgressManagerPrivate *m_progressManager;
    VariableManager *m_variableManager;
    VcsManager *m_vcsManager;
    StatusBarManager *m_statusBarManager;
    ModeManager *m_modeManager; //模型管理器 提供了FancyTabWidget和Mode之间的通信桥梁
    MimeDatabase *m_mimeDatabase;
    HelpManager *m_helpManager;
    FancyTabWidget *m_modeStack; //FancyTabWidget用于创建最左边的工具栏,它包括FancyTabBar和CornerWidget两个部分,同时它还管理着一个StackedLayout和状态栏
    NavigationWidget *m_navigationWidget;
    RightPaneWidget *m_rightPaneWidget;
    Core::StatusBarWidget *m_outputView;
    VersionDialog *m_versionDialog;

    QList<IContext *> m_activeContext;

    QMap<QWidget *, IContext *> m_contextWidgets;

    GeneralSettings *m_generalSettings;
    ShortcutSettings *m_shortcutSettings;
    ToolSettings *m_toolSettings;
    MimeTypeSettings *m_mimeTypeSettings;
    SystemEditor *m_systemEditor;

    // actions
    QShortcut *m_focusToEditor;
    QAction *m_newAction;//File->New File的操作对象
    QAction *m_openAction;//File->Open File的操作对象
    QAction *m_openWithAction;//File->Open File With的操作对象
    QAction *m_saveAllAction;//File->Save All的操作对象
    QAction *m_exitAction;//File->Exit的操作对象
    QAction *m_optionsAction;//Tools->Options的操作对象
    QAction *m_toggleSideBarAction;//Window->Show Sidebar的操作对象
    QAction *m_toggleModeSelectorAction;
    QAction *m_toggleFullScreenAction;//Window->Full Screenr的操作对象
    QAction *m_minimizeAction;//只对mac有效
    QAction *m_zoomAction;

    QToolButton *m_toggleSideBarButton;
    QColor m_overrideColor; //主题色彩风格 设置了这个后,界面上的左侧导航栏，菜单栏,状态条都会是这个风格色

    QStringList m_filesToOpenDelayed;
};

} // namespace Internal
} // namespace Core

#endif // MAINWINDOW_H
