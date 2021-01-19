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

#include "mainwindow.h"
#include "icore.h"
#include "coreconstants.h"
#include "toolsettings.h"
#include "mimetypesettings.h"
#include "fancytabwidget.h"
#include "documentmanager.h"
#include "generalsettings.h"
#include "helpmanager.h"
#include "idocumentfactory.h"
#include "messagemanager.h"
#include "modemanager.h"
#include "mimedatabase.h"
#include "outputpanemanager.h"
#include "plugindialog.h"
#include "vcsmanager.h"
#include "variablemanager.h"
#include "versiondialog.h"
#include "statusbarmanager.h"
#include "id.h"
#include "manhattanstyle.h"
#include "navigationwidget.h"
#include "rightpane.h"
#include "editormanager/ieditorfactory.h"
#include "statusbarwidget.h"
#include "externaltoolmanager.h"
#include "editormanager/systemeditor.h"

#if defined(Q_OS_MAC)
#include "macfullscreen.h"
#endif

#include <app/app_version.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actionmanager_p.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/dialogs/newdialog.h>
#include <coreplugin/dialogs/settingsdialog.h>
#include <coreplugin/dialogs/shortcutsettings.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icorelistener.h>
#include <coreplugin/inavigationwidgetfactory.h>
#include <coreplugin/progressmanager/progressmanager_p.h>
#include <coreplugin/progressmanager/progressview.h>
#include <coreplugin/settingsdatabase.h>
#include <utils/historycompleter.h>
#include <utils/hostosinfo.h>
#include <utils/stylehelper.h>
#include <utils/stringutils.h>
#include <extensionsystem/pluginmanager.h>

#include <QDebug>
#include <QFileInfo>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QDir>
#include <QMimeData>

#include <QApplication>
#include <QCloseEvent>
#include <QMenu>
#include <QPrinter>
#include <QShortcut>
#include <QStatusBar>
#include <QToolButton>
#include <QMessageBox>
#include <QMenuBar>
#include <QPushButton>
#include <QStyleFactory>

using namespace Core;
using namespace Core::Internal;

enum { debugMainWindow = 0 };

MainWindow::MainWindow() :
    Utils::AppMainWindow(),//父类初始化
	m_coreImpl(new ICore(this)), //ICore类允许访问组成qt的基本功能仅有的一个实例由coreplugin创建,由此可见coreplugin的核心功能都在MainWindow
    m_additionalContexts(Constants::C_GLOBAL),
    m_settings(ExtensionSystem::PluginManager::settings()), //用户配置文件
    m_globalSettings(ExtensionSystem::PluginManager::globalSettings()), //全局配置文件

	//以类似于配置方式访问的数据库  参数:文件存放路径 文件名 父节点
    m_settingsDatabase(new SettingsDatabase(QFileInfo(m_settings->fileName()).path(),
                                            QLatin1String("QtCreator"),
                                            this)),
    m_printer(0),//打印机对象
    m_actionManager(new ActionManager(this)), //命令管理类对象,注册Action获得命令对象
    m_editorManager(0), 
    m_externalToolManager(0),
    m_progressManager(new ProgressManagerPrivate), //任务管理器
    m_variableManager(new VariableManager), //变量管理器 （例如有变量 QT_DIR,这个则会保存和管理对应的值）
    m_vcsManager(new VcsManager),
    m_statusBarManager(0), //状态栏管理器 增删状态栏内控件
    m_modeManager(0), //模式管理器  编辑 调试等5种模式
    m_mimeDatabase(new MimeDatabase), //一个mime类型的数据库
    m_helpManager(new HelpManager),
    m_modeStack(new FancyTabWidget(this)), //FancyTabWidget用于创建最左边的工具栏 状态栏目 以及主窗口
    m_navigationWidget(0),
    m_rightPaneWidget(0),
    m_versionDialog(0),
    m_generalSettings(new GeneralSettings),
    m_shortcutSettings(new ShortcutSettings),
    m_toolSettings(new ToolSettings),
    m_mimeTypeSettings(new MimeTypeSettings),
    m_systemEditor(new SystemEditor),
    m_focusToEditor(0),
    m_newAction(0),
    m_openAction(0),
    m_openWithAction(0),
    m_saveAllAction(0),
    m_exitAction(0),
    m_optionsAction(0),
    m_toggleSideBarAction(0),
    m_toggleFullScreenAction(0),
    m_minimizeAction(0),
    m_zoomAction(0),
    m_toggleSideBarButton(new QToolButton)
{
    (void) new DocumentManager(this); //文档管理器 文件名改变啊 修改啊之类管理
    OutputPaneManager::create();

    Utils::HistoryCompleter::setSettings(m_settings);

	//对窗口进行一些熟悉设置
    setWindowTitle(tr("Qt Creator"));
    if (!Utils::HostOsInfo::isMacHost())
        QApplication::setWindowIcon(QIcon(QLatin1String(Constants::ICON_QTLOGO_128)));
    QCoreApplication::setApplicationName(QLatin1String("QtCreator"));
    QCoreApplication::setApplicationVersion(QLatin1String(Core::Constants::IDE_VERSION_LONG));
    QCoreApplication::setOrganizationName(QLatin1String(Constants::IDE_SETTINGSVARIANT_STR));
    QString baseName = QApplication::style()->objectName();
    if (Utils::HostOsInfo::isAnyUnixHost() && !Utils::HostOsInfo::isMacHost()) {
        if (baseName == QLatin1String("windows")) {
            // Sometimes we get the standard windows 95 style as a fallback
            if (QStyleFactory::keys().contains(QLatin1String("Fusion"))) {
                baseName = QLatin1String("fusion"); // Qt5
            } else { // Qt4
                // e.g. if we are running on a KDE4 desktop
                QByteArray desktopEnvironment = qgetenv("DESKTOP_SESSION");
                if (desktopEnvironment == "kde")
                    baseName = QLatin1String("plastique");
                else
                    baseName = QLatin1String("cleanlooks");
            }
        }
    }
    qApp->setStyle(new ManhattanStyle(baseName));
    
	//允许嵌套dock
    setDockNestingEnabled(true);

	//让四个角落（corner）中一个区域"归属于"某个区域（area）
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);

	
    m_modeManager = new ModeManager(this, m_modeStack);

    //菜单栏目的设计布局构建生成
    registerDefaultContainers();

    //创建几个在布局时没创建的菜单子项目,并为菜单栏各个项目创建并添加和绑定操作同时注册到操作管理器
    registerDefaultActions();

    m_navigationWidget = new NavigationWidget(m_toggleSideBarAction);// Window->Show Sidebar的操作对象
    m_rightPaneWidget = new RightPaneWidget();

    m_statusBarManager = new StatusBarManager(this);
    m_messageManager = new MessageManager;
    m_editorManager = new EditorManager(this);
    m_editorManager->hide();
    m_externalToolManager = new ExternalToolManager();
    setCentralWidget(m_modeStack);

    m_progressManager->progressView()->setParent(this);
    m_progressManager->progressView()->setReferenceWidget(m_modeStack->statusBar());
    //当系统焦点发生改变的时候，就会发出focusChanged信号,用updateFocusWidget函数处理这个信号
    connect(QApplication::instance(), SIGNAL(focusChanged(QWidget*,QWidget*)),
            this, SLOT(updateFocusWidget(QWidget*,QWidget*)));
    // Add a small Toolbutton for toggling the navigation widget
    statusBar()->insertPermanentWidget(0, m_toggleSideBarButton);


    statusBar()->setProperty("p_styled", true);
    setAcceptDrops(true);

#if defined(Q_OS_MAC)
    MacFullScreen::addFullScreen(this);
#endif
}
//菜单栏 Window->Show Silderbar事件
void MainWindow::setSidebarVisible(bool visible)
{
    if (NavigationWidgetPlaceHolder::current()) {
        if (m_navigationWidget->isSuppressed() && visible) {
            m_navigationWidget->setShown(true);
            m_navigationWidget->setSuppressed(false);
        } else {
            m_navigationWidget->setShown(visible);
        }
    }
}

void MainWindow::setSuppressNavigationWidget(bool suppress)
{
    if (NavigationWidgetPlaceHolder::current())
        m_navigationWidget->setSuppressed(suppress);
}
//设置主题色彩风格
void MainWindow::setOverrideColor(const QColor &color)
{
    m_overrideColor = color;
}

void MainWindow::setIsFullScreen(bool fullScreen)
{
    if (fullScreen)
        m_toggleFullScreenAction->setText(tr("Exit Full Screen"));
    else
        m_toggleFullScreenAction->setText(tr("Enter Full Screen"));
}

MainWindow::~MainWindow()
{
    ExtensionSystem::PluginManager::removeObject(m_shortcutSettings);
    ExtensionSystem::PluginManager::removeObject(m_generalSettings);
    ExtensionSystem::PluginManager::removeObject(m_toolSettings);
    ExtensionSystem::PluginManager::removeObject(m_mimeTypeSettings);
    ExtensionSystem::PluginManager::removeObject(m_systemEditor);
    delete m_externalToolManager;
    m_externalToolManager = 0;
    delete m_messageManager;
    m_messageManager = 0;
    delete m_shortcutSettings;
    m_shortcutSettings = 0;
    delete m_generalSettings;
    m_generalSettings = 0;
    delete m_toolSettings;
    m_toolSettings = 0;
    delete m_mimeTypeSettings;
    m_mimeTypeSettings = 0;
    delete m_systemEditor;
    m_systemEditor = 0;
    delete m_settings;
    m_settings = 0;
    delete m_printer;
    m_printer = 0;
    delete m_vcsManager;
    m_vcsManager = 0;
    //we need to delete editormanager and statusbarmanager explicitly before the end of the destructor,
    //because they might trigger stuff that tries to access data from editorwindow, like removeContextWidget

    // All modes are now gone
    OutputPaneManager::destroy();

    // Now that the OutputPaneManager is gone, is a good time to delete the view
    ExtensionSystem::PluginManager::removeObject(m_outputView);
    delete m_outputView;

    delete m_editorManager;
    m_editorManager = 0;
    delete m_progressManager;
    m_progressManager = 0;
    delete m_statusBarManager;
    m_statusBarManager = 0;
    ExtensionSystem::PluginManager::removeObject(m_coreImpl);
    delete m_coreImpl;
    m_coreImpl = 0;

    delete m_rightPaneWidget;
    m_rightPaneWidget = 0;

    delete m_modeManager;
    m_modeManager = 0;
    delete m_mimeDatabase;
    m_mimeDatabase = 0;

    delete m_helpManager;
    m_helpManager = 0;
    delete m_variableManager;
    m_variableManager = 0;
}

bool MainWindow::init(QString *errorMessage)
{
    Q_UNUSED(errorMessage)

    if (!MimeDatabase::addMimeTypes(QLatin1String(":/core/editormanager/BinFiles.mimetypes.xml"), errorMessage))
        return false;

    ExtensionSystem::PluginManager::addObject(m_coreImpl);//core插件接口类交由插件管理器管理 (注意区分插件加载初始化的接口 与 插件功能接口 这个是插件功能接口)
    m_statusBarManager->init(); //状态栏初始化
    m_modeManager->init();//模型管理器初始化
    m_progressManager->init(); // needs the status bar manager//过程管理器初始化

    ExtensionSystem::PluginManager::addObject(m_generalSettings);
    ExtensionSystem::PluginManager::addObject(m_shortcutSettings);
    ExtensionSystem::PluginManager::addObject(m_toolSettings);
    ExtensionSystem::PluginManager::addObject(m_mimeTypeSettings);
    ExtensionSystem::PluginManager::addObject(m_systemEditor);

    // Add widget to the bottom, we create the view here instead of inside the
    // OutputPaneManager, since the StatusBarManager needs to be initialized before
    m_outputView = new Core::StatusBarWidget;
    m_outputView->setWidget(OutputPaneManager::instance()->buttonsWidget());
    m_outputView->setPosition(Core::StatusBarWidget::Second);
    ExtensionSystem::PluginManager::addObject(m_outputView);
    MessageManager::init();
    return true;
}

void MainWindow::extensionsInitialized()
{
    m_editorManager->init();
    m_statusBarManager->extensionsInitalized();
    OutputPaneManager::instance()->init();
    m_vcsManager->extensionsInitialized();
    m_navigationWidget->setFactories(ExtensionSystem::PluginManager::getObjects<INavigationWidgetFactory>());

    // reading the shortcut settings must be done after all shortcuts have been registered
    m_actionManager->initialize();

    readSettings();
    updateContext();

    emit m_coreImpl->coreAboutToOpen();
    show();
    emit m_coreImpl->coreOpened();
}
//closeEvent 窗口关闭调用
void MainWindow::closeEvent(QCloseEvent *event)
{
    ICore::saveSettings();

    // Save opened files
    bool cancelled;
    QList<IDocument*> notSaved = DocumentManager::saveModifiedDocuments(DocumentManager::modifiedDocuments(), &cancelled);
    if (cancelled || !notSaved.isEmpty()) {
        event->ignore();
        return;
    }

    const QList<ICoreListener *> listeners =
        ExtensionSystem::PluginManager::getObjects<ICoreListener>();
    foreach (ICoreListener *listener, listeners) {
        if (!listener->coreAboutToClose()) {
            event->ignore();
            return;
        }
    }

    emit m_coreImpl->coreAboutToClose();

    writeSettings();

    m_navigationWidget->closeSubWidgets();

    event->accept();
}

// Check for desktop file manager file drop events

static bool isDesktopFileManagerDrop(const QMimeData *d, QStringList *files = 0)
{
    if (files)
        files->clear();
    // Extract dropped files from Mime data.
    if (!d->hasUrls())
        return false;
    const QList<QUrl> urls = d->urls();
    if (urls.empty())
        return false;
    // Try to find local files
    bool hasFiles = false;
    const QList<QUrl>::const_iterator cend = urls.constEnd();
    for (QList<QUrl>::const_iterator it = urls.constBegin(); it != cend; ++it) {
        const QString fileName = it->toLocalFile();
        if (!fileName.isEmpty()) {
            hasFiles = true;
            if (files)
                files->push_back(fileName);
            else
                break; // No result list, sufficient for checking
        }
    }
    return hasFiles;
}
//当鼠标拖拽进入事件 EG拖拽一个文件进编辑器
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (isDesktopFileManagerDrop(event->mimeData()) && m_filesToOpenDelayed.isEmpty())
        event->accept();
    else
        event->ignore();
}
//拖放操作分为两个截然不同的动作: 拖动和放下.dragEnterEvent 与 dropEvent
void MainWindow::dropEvent(QDropEvent *event)
{
    QStringList files;
    if (isDesktopFileManagerDrop(event->mimeData(), &files)) {
        event->accept();
        m_filesToOpenDelayed.append(files);
        QTimer::singleShot(50, this, SLOT(openDelayedFiles()));
    } else {
        event->ignore();
    }
}

void MainWindow::openDelayedFiles()
{
    if (m_filesToOpenDelayed.isEmpty())
        return;
    raiseWindow();
    openFiles(m_filesToOpenDelayed, ICore::SwitchMode);
    m_filesToOpenDelayed.clear();
}

IContext *MainWindow::currentContextObject() const
{
    return m_activeContext.isEmpty() ? 0 : m_activeContext.first();
}

QStatusBar *MainWindow::statusBar() const
{
    return m_modeStack->statusBar();
}

//菜单栏目的设计布局构建
void MainWindow::registerDefaultContainers()
{
    //菜单栏实例
    ActionContainer *menubar = ActionManager::createMenuBar(Constants::MENU_BAR);

    if (!Utils::HostOsInfo::isMacHost()) // System menu bar on Mac
        setMenuBar(menubar->menuBar()); //将创建的菜单栏设置为主窗体的菜单栏

	//想菜单栏实例中插入条目 每个条目都是一个容器,都持有一个容器集
    menubar->appendGroup(Constants::G_FILE); //File
    menubar->appendGroup(Constants::G_EDIT); //Edit
    menubar->appendGroup(Constants::G_VIEW); //View
    menubar->appendGroup(Constants::G_TOOLS); //Tools
    menubar->appendGroup(Constants::G_WINDOW);//Window
    menubar->appendGroup(Constants::G_HELP); //Help

    // File Menu 菜单实例
    ActionContainer *filemenu = ActionManager::createMenu(Constants::M_FILE);
    menubar->addMenu(filemenu, Constants::G_FILE); //添加加到菜单栏
    filemenu->menu()->setTitle(tr("&File"));       //菜单显示名  名为 File
    filemenu->appendGroup(Constants::G_FILE_NEW);  //为菜单添加 操作项 New File
    filemenu->appendGroup(Constants::G_FILE_OPEN); //Open File
    filemenu->appendGroup(Constants::G_FILE_PROJECT);
    filemenu->appendGroup(Constants::G_FILE_SAVE);
    filemenu->appendGroup(Constants::G_FILE_CLOSE);
    filemenu->appendGroup(Constants::G_FILE_PRINT);
    filemenu->appendGroup(Constants::G_FILE_OTHER);
    connect(filemenu->menu(), SIGNAL(aboutToShow()), this, SLOT(aboutToShowRecentFiles())); //aboutToShow 此信号在菜单显示给用户之前发出。


    // Edit Menu
    ActionContainer *medit = ActionManager::createMenu(Constants::M_EDIT);
    menubar->addMenu(medit, Constants::G_EDIT);
    medit->menu()->setTitle(tr("&Edit"));
    medit->appendGroup(Constants::G_EDIT_UNDOREDO);
    medit->appendGroup(Constants::G_EDIT_COPYPASTE);
    medit->appendGroup(Constants::G_EDIT_SELECTALL);
    medit->appendGroup(Constants::G_EDIT_ADVANCED);
    medit->appendGroup(Constants::G_EDIT_FIND);
    medit->appendGroup(Constants::G_EDIT_OTHER);

    // Tools Menu 这个菜单的子项目在这里并没有构建
    ActionContainer *ac = ActionManager::createMenu(Constants::M_TOOLS);
    menubar->addMenu(ac, Constants::G_TOOLS);
    ac->menu()->setTitle(tr("&Tools"));

    // Window Menu
    ActionContainer *mwindow = ActionManager::createMenu(Constants::M_WINDOW);
    menubar->addMenu(mwindow, Constants::G_WINDOW);
    mwindow->menu()->setTitle(tr("&Window"));
    mwindow->appendGroup(Constants::G_WINDOW_SIZE);
    mwindow->appendGroup(Constants::G_WINDOW_VIEWS);
    mwindow->appendGroup(Constants::G_WINDOW_PANES);
    mwindow->appendGroup(Constants::G_WINDOW_SPLIT);
    mwindow->appendGroup(Constants::G_WINDOW_NAVIGATE);
    mwindow->appendGroup(Constants::G_WINDOW_OTHER);

    // Help Menu
    ac = ActionManager::createMenu(Constants::M_HELP);
    menubar->addMenu(ac, Constants::G_HELP);
    ac->menu()->setTitle(tr("&Help"));
    ac->appendGroup(Constants::G_HELP_HELP);
    ac->appendGroup(Constants::G_HELP_ABOUT);
}

//创建几个在布局时没创建的菜单子项目,并为菜单栏各个项目创建并添加和绑定操作同时注册到操作管理器
void MainWindow::registerDefaultActions()
{
    //拿到各个菜单
    ActionContainer *mfile = ActionManager::actionContainer(Constants::M_FILE);
    ActionContainer *medit = ActionManager::actionContainer(Constants::M_EDIT);
    ActionContainer *mtools = ActionManager::actionContainer(Constants::M_TOOLS);
    //Window菜单
    ActionContainer *mwindow = ActionManager::actionContainer(Constants::M_WINDOW);
    ActionContainer *mhelp = ActionManager::actionContainer(Constants::M_HELP);

    Context globalContext(Constants::C_GLOBAL);

    // File menu separators 为File菜单操作项增加分隔符
    mfile->addSeparator(globalContext, Constants::G_FILE_SAVE);
    mfile->addSeparator(globalContext, Constants::G_FILE_PRINT);
    mfile->addSeparator(globalContext, Constants::G_FILE_CLOSE);
    mfile->addSeparator(globalContext, Constants::G_FILE_OTHER);
    // Edit menu separators为Edit菜单操作项增加分隔符
    medit->addSeparator(globalContext, Constants::G_EDIT_COPYPASTE);
    medit->addSeparator(globalContext, Constants::G_EDIT_SELECTALL);
    medit->addSeparator(globalContext, Constants::G_EDIT_FIND);
    medit->addSeparator(globalContext, Constants::G_EDIT_ADVANCED);

    // Return to editor shortcut: Note this requires Qt to fix up
    // handling of shortcut overrides in menus, item views, combos....
    m_focusToEditor = new QShortcut(this);//这个命令并非菜单栏相关而是处理键盘按键 esc 键事件
    Command *cmd = ActionManager::registerShortcut(m_focusToEditor, Constants::S_RETURNTOEDITOR, globalContext);
    cmd->setDefaultKeySequence(QKeySequence(Qt::Key_Escape)); //按键 esc 事件处理
    connect(m_focusToEditor, SIGNAL(activated()), this, SLOT(setFocusToEditor()));

    // New File Action 就是绑定了操作项目到菜单项目  由以下补助可以看出,此处抽象了操作为命令,并且让操作管理器来生成管理所有的命令
    QIcon icon = QIcon::fromTheme(QLatin1String("document-new"), QIcon(QLatin1String(Constants::ICON_NEWFILE))); //菜单File的操作项New File的icon
    m_newAction = new QAction(icon, tr("&New File or Project..."), this);//创建菜单File的操作项New File的操作实例QAction
    cmd = ActionManager::registerAction(m_newAction, Constants::NEW, globalContext); //注册操作到操作管理器,得到命令对象
    cmd->setDefaultKeySequence(QKeySequence::New); //设置快捷键
    mfile->addAction(cmd, Constants::G_FILE_NEW);  //为菜单File的操作项New File绑定操作命令实例 cmd
    connect(m_newAction, SIGNAL(triggered()), this, SLOT(newFile())); //为操作实例设置具体执行函数

    // Open Action
    icon = QIcon::fromTheme(QLatin1String("document-open"), QIcon(QLatin1String(Constants::ICON_OPENFILE)));
    m_openAction = new QAction(icon, tr("&Open File or Project..."), this);
    cmd = ActionManager::registerAction(m_openAction, Constants::OPEN, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Open);
    mfile->addAction(cmd, Constants::G_FILE_OPEN);
    connect(m_openAction, SIGNAL(triggered()), this, SLOT(openFile()));

    // Open With Action
    m_openWithAction = new QAction(tr("Open File &With..."), this);
    cmd = ActionManager::registerAction(m_openWithAction, Constants::OPEN_WITH, globalContext);
    mfile->addAction(cmd, Constants::G_FILE_OPEN);
    connect(m_openWithAction, SIGNAL(triggered()), this, SLOT(openFileWith()));

    // File->Recent Files Menu 调用ActionManager::createMenu因为构建方法并没有构建这个菜单项
    ActionContainer *ac = ActionManager::createMenu(Constants::M_FILE_RECENTFILES);
    mfile->addMenu(ac, Constants::G_FILE_OPEN);
    ac->menu()->setTitle(tr("Recent &Files"));
    ac->setOnAllDisabledBehavior(ActionContainer::Show);

    // Save Action 这个菜单项并没有绑定具体的操作处理函数 并且也没有启用
    icon = QIcon::fromTheme(QLatin1String("document-save"), QIcon(QLatin1String(Constants::ICON_SAVEFILE)));
    QAction *tmpaction = new QAction(icon, tr("&Save"), this);
    tmpaction->setEnabled(false); //File->Save 菜单项目默认不启用 不可点击
    cmd = ActionManager::registerAction(tmpaction, Constants::SAVE, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Save);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(tr("Save"));
    mfile->addAction(cmd, Constants::G_FILE_SAVE);

    // Save As Action
    icon = QIcon::fromTheme(QLatin1String("document-save-as"));
    tmpaction = new QAction(icon, tr("Save &As..."), this);
    tmpaction->setEnabled(false);
    cmd = ActionManager::registerAction(tmpaction, Constants::SAVEAS, globalContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Ctrl+Shift+S") : QString()));
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(tr("Save As..."));
    mfile->addAction(cmd, Constants::G_FILE_SAVE);

    // SaveAll Action
    m_saveAllAction = new QAction(tr("Save A&ll"), this);
    cmd = ActionManager::registerAction(m_saveAllAction, Constants::SAVEALL, globalContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? QString() : tr("Ctrl+Shift+S")));
    mfile->addAction(cmd, Constants::G_FILE_SAVE);
    connect(m_saveAllAction, SIGNAL(triggered()), this, SLOT(saveAll()));

    // Print Action
    icon = QIcon::fromTheme(QLatin1String("document-print"));
    tmpaction = new QAction(icon, tr("&Print..."), this);
    tmpaction->setEnabled(false);
    cmd = ActionManager::registerAction(tmpaction, Constants::PRINT, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Print);
    mfile->addAction(cmd, Constants::G_FILE_PRINT);

    // Exit Action
    icon = QIcon::fromTheme(QLatin1String("application-exit"));
    m_exitAction = new QAction(icon, tr("E&xit"), this);
    cmd = ActionManager::registerAction(m_exitAction, Constants::EXIT, globalContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Q")));
    mfile->addAction(cmd, Constants::G_FILE_OTHER);
    connect(m_exitAction, SIGNAL(triggered()), this, SLOT(exit()));

    // Undo Action
    icon = QIcon::fromTheme(QLatin1String("edit-undo"), QIcon(QLatin1String(Constants::ICON_UNDO)));
    tmpaction = new QAction(icon, tr("&Undo"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::UNDO, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Undo);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(tr("Undo"));
    medit->addAction(cmd, Constants::G_EDIT_UNDOREDO);
    tmpaction->setEnabled(false);

    // Redo Action
    icon = QIcon::fromTheme(QLatin1String("edit-redo"), QIcon(QLatin1String(Constants::ICON_REDO)));
    tmpaction = new QAction(icon, tr("&Redo"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::REDO, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Redo);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(tr("Redo"));
    medit->addAction(cmd, Constants::G_EDIT_UNDOREDO);
    tmpaction->setEnabled(false);

    // Cut Action
    icon = QIcon::fromTheme(QLatin1String("edit-cut"), QIcon(QLatin1String(Constants::ICON_CUT)));
    tmpaction = new QAction(icon, tr("Cu&t"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::CUT, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Cut);
    medit->addAction(cmd, Constants::G_EDIT_COPYPASTE);
    tmpaction->setEnabled(false);

    // Copy Action
    icon = QIcon::fromTheme(QLatin1String("edit-copy"), QIcon(QLatin1String(Constants::ICON_COPY)));
    tmpaction = new QAction(icon, tr("&Copy"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::COPY, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Copy);
    medit->addAction(cmd, Constants::G_EDIT_COPYPASTE);
    tmpaction->setEnabled(false);

    // Paste Action
    icon = QIcon::fromTheme(QLatin1String("edit-paste"), QIcon(QLatin1String(Constants::ICON_PASTE)));
    tmpaction = new QAction(icon, tr("&Paste"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::PASTE, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Paste);
    medit->addAction(cmd, Constants::G_EDIT_COPYPASTE);
    tmpaction->setEnabled(false);

    // Select All
    icon = QIcon::fromTheme(QLatin1String("edit-select-all"));
    tmpaction = new QAction(icon, tr("Select &All"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::SELECTALL, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::SelectAll);
    medit->addAction(cmd, Constants::G_EDIT_SELECTALL);
    tmpaction->setEnabled(false);

    // Goto Action
    icon = QIcon::fromTheme(QLatin1String("go-jump"));
    tmpaction = new QAction(icon, tr("&Go to Line..."), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::GOTO, globalContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+L")));
    medit->addAction(cmd, Constants::G_EDIT_OTHER);
    tmpaction->setEnabled(false);

    // Options Action
    mtools->appendGroup(Constants::G_TOOLS_OPTIONS); //Tools菜单下 增加子菜单Options但是这也是一组项目
    mtools->addSeparator(globalContext, Constants::G_TOOLS_OPTIONS); //添加一个分隔符
    m_optionsAction = new QAction(tr("&Options..."), this); //创建一个操作对象
    cmd = ActionManager::registerAction(m_optionsAction, Constants::OPTIONS, globalContext); //注册操作 拿到操作命令实例
    if (UseMacShortcuts) { //不是mac 不用看下面代码
        cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+,")));
        cmd->action()->setMenuRole(QAction::PreferencesRole);
    }
    mtools->addAction(cmd, Constants::G_TOOLS_OPTIONS); //给Tools的菜单组Options增加操作
    connect(m_optionsAction, SIGNAL(triggered()), this, SLOT(showOptionsDialog())); //绑定具体的操作函数实现

    if (UseMacShortcuts) { //不是mac 不用看下面代码
        // Minimize Action
        m_minimizeAction = new QAction(tr("Minimize"), this);
        cmd = ActionManager::registerAction(m_minimizeAction, Constants::MINIMIZE_WINDOW, globalContext);
        cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+M")));
        mwindow->addAction(cmd, Constants::G_WINDOW_SIZE);
        connect(m_minimizeAction, SIGNAL(triggered()), this, SLOT(showMinimized()));

        // Zoom Action
        m_zoomAction = new QAction(tr("Zoom"), this);
        cmd = ActionManager::registerAction(m_zoomAction, Constants::ZOOM_WINDOW, globalContext);
        mwindow->addAction(cmd, Constants::G_WINDOW_SIZE);
        connect(m_zoomAction, SIGNAL(triggered()), this, SLOT(showMaximized()));

        // Window separator
        mwindow->addSeparator(globalContext, Constants::G_WINDOW_SIZE);
    }

    // Show Sidebar Action
    //1.创建Action命令
    m_toggleSideBarAction = new QAction(QIcon(QLatin1String(Constants::ICON_TOGGLE_SIDEBAR)),
                                        tr("Show Sidebar"), this);
    m_toggleSideBarAction->setCheckable(true);
    cmd = ActionManager::registerAction(m_toggleSideBarAction, Constants::TOGGLE_SIDEBAR, globalContext);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Ctrl+0") : tr("Alt+0")));
    connect(m_toggleSideBarAction, SIGNAL(triggered(bool)), this, SLOT(setSidebarVisible(bool)));
    m_toggleSideBarButton->setDefaultAction(cmd->action());
    //2. 添加命令到菜单 Window->Show Sidebar命令
    mwindow->addAction(cmd, Constants::G_WINDOW_VIEWS);
    m_toggleSideBarAction->setEnabled(false);

    // Show Mode Selector Action
    m_toggleModeSelectorAction = new QAction(tr("Show Mode Selector"), this);
    m_toggleModeSelectorAction->setCheckable(true);
    cmd = ActionManager::registerAction(m_toggleModeSelectorAction, Constants::TOGGLE_MODE_SELECTOR, globalContext);
    connect(m_toggleModeSelectorAction, SIGNAL(triggered(bool)), ModeManager::instance(), SLOT(setModeSelectorVisible(bool)));
    mwindow->addAction(cmd, Constants::G_WINDOW_VIEWS);

#if defined(Q_OS_MAC)
    const QString fullScreenActionText(tr("Enter Full Screen"));
    bool supportsFullScreen = MacFullScreen::supportsFullScreen();
#else
    const QString fullScreenActionText(tr("Full Screen"));
    bool supportsFullScreen = true;
#endif
    if (supportsFullScreen) {
        // Full Screen Action
        m_toggleFullScreenAction = new QAction(fullScreenActionText, this);
        m_toggleFullScreenAction->setMenuRole(QAction::NoRole);
        m_toggleFullScreenAction->setCheckable(!Utils::HostOsInfo::isMacHost());
        cmd = ActionManager::registerAction(m_toggleFullScreenAction, Constants::TOGGLE_FULLSCREEN, globalContext);
        cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Ctrl+Meta+F") : tr("Ctrl+Shift+F11")));
        cmd->setAttribute(Command::CA_UpdateText); /* for Mac */
        mwindow->addAction(cmd, Constants::G_WINDOW_SIZE);
        connect(m_toggleFullScreenAction, SIGNAL(triggered(bool)), this, SLOT(setFullScreen(bool)));
    }

    // Window->Views 调用ActionManager::createMenu因为菜单栏布局构建没有这个
    ActionContainer *mviews = ActionManager::createMenu(Constants::M_WINDOW_VIEWS);
    mwindow->addMenu(mviews, Constants::G_WINDOW_VIEWS);//Window加入子菜单
    mviews->menu()->setTitle(tr("&Views"));

    // About IDE Action
    icon = QIcon::fromTheme(QLatin1String("help-about"));
    if (Utils::HostOsInfo::isMacHost())
        tmpaction = new QAction(icon, tr("About &Qt Creator"), this); // it's convention not to add dots to the about menu
    else
        tmpaction = new QAction(icon, tr("About &Qt Creator..."), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::ABOUT_QTCREATOR, globalContext);
    if (Utils::HostOsInfo::isMacHost())
        cmd->action()->setMenuRole(QAction::ApplicationSpecificRole);
    mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
    tmpaction->setEnabled(true);
    connect(tmpaction, SIGNAL(triggered()), this,  SLOT(aboutQtCreator()));

    //About Plugins Action
    tmpaction = new QAction(tr("About &Plugins..."), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::ABOUT_PLUGINS, globalContext);
    if (Utils::HostOsInfo::isMacHost())
        cmd->action()->setMenuRole(QAction::ApplicationSpecificRole);
    mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
    tmpaction->setEnabled(true);
    connect(tmpaction, SIGNAL(triggered()), this,  SLOT(aboutPlugins()));
    // About Qt Action
//    tmpaction = new QAction(tr("About &Qt..."), this);
//    cmd = ActionManager::registerAction(tmpaction, Constants:: ABOUT_QT, globalContext);
//    mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
//    tmpaction->setEnabled(true);
//    connect(tmpaction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    // About sep
    if (!Utils::HostOsInfo::isMacHost()) { // doesn't have the "About" actions in the Help menu
        tmpaction = new QAction(this);
        tmpaction->setSeparator(true);
        cmd = ActionManager::registerAction(tmpaction, "QtCreator.Help.Sep.About", globalContext);
        mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
    }
}
//菜单栏目 File->NewFile 事件
void MainWindow::newFile()
{
    showNewItemDialog(tr("New", "Title of dialog"), IWizard::allWizards(), QString());
}
//菜单栏目 File->OpenFile 事件
void MainWindow::openFile()
{
    openFiles(EditorManager::getOpenFileNames(), ICore::SwitchMode);
}

static QList<IDocumentFactory*> getNonEditorDocumentFactories()
{
    const QList<IDocumentFactory*> allFileFactories =
        ExtensionSystem::PluginManager::getObjects<IDocumentFactory>();
    QList<IDocumentFactory*> nonEditorFileFactories;
    foreach (IDocumentFactory *factory, allFileFactories) {
        if (!qobject_cast<IEditorFactory *>(factory))
            nonEditorFileFactories.append(factory);
    }
    return nonEditorFileFactories;
}

static IDocumentFactory *findDocumentFactory(const QList<IDocumentFactory*> &fileFactories,
                                     const QFileInfo &fi)
{
    if (const MimeType mt = MimeDatabase::findByFile(fi)) {
        const QString type = mt.type();
        foreach (IDocumentFactory *factory, fileFactories) {
            if (factory->mimeTypes().contains(type))
                return factory;
        }
    }
    return 0;
}

/*! Either opens \a fileNames with editors or loads a project.
 *
 *  \a flags can be used to stop on first failure, indicate that a file name
 *  might include line numbers and/or switch mode to edit mode.
 *
 *  \returns the first opened document. Required to support the -block flag
 *  for client mode.
 *
 *  \sa IPlugin::remoteArguments()
 */
IDocument *MainWindow::openFiles(const QStringList &fileNames, ICore::OpenFilesFlags flags)
{
    QList<IDocumentFactory*> nonEditorFileFactories = getNonEditorDocumentFactories();//从插件对象池中找到支持这个文件的IDocumentFactory对象(不是IEditFactory的)
    IDocument *res = 0;

    foreach (const QString &fileName, fileNames) {
        const QFileInfo fi(fileName);
        const QString absoluteFilePath = fi.absoluteFilePath();
        if (IDocumentFactory *documentFactory = findDocumentFactory(nonEditorFileFactories, fi)) {
            IDocument *document = documentFactory->open(absoluteFilePath); //找到处理此类文件的documentFactory，则为这个文件创建document对象
            if (!document) {
                if (flags & ICore::StopOnLoadFail)
                    return res;
            } else {
                if (!res)
                    res = document;
                if (flags & ICore::SwitchMode) //切换模式到编辑模式
                    ModeManager::activateMode(Id(Core::Constants::MODE_EDIT));
            }
        } else { //找不到对应的documentFactory,则使用EditorManager打开，创建一个editor对象，编辑
            QFlags<EditorManager::OpenEditorFlag> emFlags;
            if (flags & ICore::CanContainLineNumbers)
                emFlags |=  EditorManager::CanContainLineNumber;
            IEditor *editor = EditorManager::openEditor(absoluteFilePath, Id(), emFlags);
            if (!editor) {
                if (flags & ICore::StopOnLoadFail)
                    return res;
            } else if (!res) {
                res = editor->document(); //编辑器打开成功，则编辑器使用的document作为返回
            }
        }
    }
    return res;
}
//按键 esc 事件处理
void MainWindow::setFocusToEditor()
{
    m_editorManager->doEscapeKeyFocusMoveMagic();
}
//菜单栏目 File->NewFile...事件具体调用
void MainWindow::showNewItemDialog(const QString &title,
                                          const QList<IWizard *> &wizards,
                                          const QString &defaultLocation,
                                          const QVariantMap &extraVariables)
{
    // Scan for wizards matching the filter and pick one. Don't show
    // dialog if there is only one.
    IWizard *wizard = 0;
    QString selectedPlatform;
    switch (wizards.size()) {
    case 0:
        break;
    case 1:
        wizard = wizards.front();
        break;
    default: {
        NewDialog dlg(this); //newdialog.ui File->NewFile弹出的工程选择框
        dlg.setWizards(wizards);
        dlg.setWindowTitle(title);
        wizard = dlg.showDialog(); //选择了一个引导
        selectedPlatform = dlg.selectedPlatform(); //所选择的 模版
    }
        break;
    }

    if (!wizard)
        return;

    QString path = defaultLocation; //这块是 ""
    if (path.isEmpty()) {
        switch (wizard->kind()) {
        case IWizard::ProjectWizard:
            // Project wizards: Check for projects directory or
            // use last visited directory of file dialog. Never start
            // at current.
			//获取工程文件所在目录
            path = DocumentManager::useProjectsDirectory() ?
                       DocumentManager::projectsDirectory() :
                       DocumentManager::fileDialogLastVisitedDirectory(); //上次访问的目录
            break;
        default:
            path = DocumentManager::fileDialogInitialDirectory(); //非工程引导，则给默认目录
            break;
        }
    }

	//运行选择的引导
    wizard->runWizard(path, this, selectedPlatform, extraVariables);
}
//菜单栏目 Tools->Options Tools->External->Configure...事件
bool MainWindow::showOptionsDialog(Id category, Id page, QWidget *parent)
{
    emit m_coreImpl->optionsDialogRequested();
    if (!parent)
        parent = this;
    SettingsDialog *dialog = SettingsDialog::getSettingsDialog(parent, category, page);
    return dialog->execDialog();
}
//菜单栏目 File->Save All 事件
void MainWindow::saveAll()
{
    DocumentManager::saveModifiedDocumentsSilently(DocumentManager::modifiedDocuments());
}
//菜单栏目 File->Exit 事件
void MainWindow::exit()
{
    // this function is most likely called from a user action
    // that is from an event handler of an object
    // since on close we are going to delete everything
    // so to prevent the deleting of that object we
    // just append it
    QTimer::singleShot(0, this,  SLOT(close()));
}
//菜单栏目 File->OpenFileWith 事件
void MainWindow::openFileWith()
{
    foreach (const QString &fileName, EditorManager::getOpenFileNames()) {
        bool isExternal;
        const Id editorId = EditorManager::getOpenWithEditorId(fileName, &isExternal);
        if (!editorId.isValid())
            continue;
        if (isExternal)
            EditorManager::openExternalEditor(fileName, editorId);
        else
            EditorManager::openEditor(fileName, editorId);
    }
}

QSettings *MainWindow::settings(QSettings::Scope scope) const
{
    if (scope == QSettings::UserScope)
        return m_settings;
    else
        return m_globalSettings;
}

IContext *MainWindow::contextObject(QWidget *widget)
{
    return m_contextWidgets.value(widget);
}

void MainWindow::addContextObject(IContext *context)
{
    if (!context)
        return;
    QWidget *widget = context->widget();
    if (m_contextWidgets.contains(widget))
        return;

    m_contextWidgets.insert(widget, context);
}

void MainWindow::removeContextObject(IContext *context)
{
    if (!context)
        return;

    QWidget *widget = context->widget();
    if (!m_contextWidgets.contains(widget))
        return;

    m_contextWidgets.remove(widget);
    if (m_activeContext.removeAll(context) > 0)
        updateContextObject(m_activeContext);
}
//change event一般是当前widget状态改变后触发的如字体改变,语言改变之类的.该方法主要捕获改变事件,当语言改变后,执行相关操作
void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    if (e->type() == QEvent::ActivationChange) {
        if (isActiveWindow()) {
            if (debugMainWindow)
                qDebug() << "main window activated";
            emit windowActivated();
        }
    } else if (e->type() == QEvent::WindowStateChange) {
        if (Utils::HostOsInfo::isMacHost()) {
            bool minimized = isMinimized();
            if (debugMainWindow)
                qDebug() << "main window state changed to minimized=" << minimized;
            m_minimizeAction->setEnabled(!minimized);
            m_zoomAction->setEnabled(!minimized);
        } else {
            bool isFullScreen = (windowState() & Qt::WindowFullScreen) != 0;
            m_toggleFullScreenAction->setChecked(isFullScreen);
        }
    }
}
//当系统焦点发生改变的时候，就会发出focusChanged信号,用updateFocusWidget函数处理这个信号
void MainWindow::updateFocusWidget(QWidget *old, QWidget *now)
{
    Q_UNUSED(old)

    // Prevent changing the context object just because the menu or a menu item is activated
    if (qobject_cast<QMenuBar*>(now) || qobject_cast<QMenu*>(now))
        return;

    QList<IContext *> newContext;
    if (QWidget *p = qApp->focusWidget()) {
        IContext *context = 0;
        while (p) {
            context = m_contextWidgets.value(p);
            if (context)
                newContext.append(context);
            p = p->parentWidget();
        }
    }

    // ignore toplevels that define no context, like popups without parent
    if (!newContext.isEmpty() || qApp->focusWidget() == focusWidget())
        updateContextObject(newContext);
}

void MainWindow::updateContextObject(const QList<IContext *> &context)
{
    emit m_coreImpl->contextAboutToChange(context);
    m_activeContext = context;
    updateContext();
    if (debugMainWindow) {
        qDebug() << "new context objects =" << context;
        foreach (IContext *c, context)
            qDebug() << (c ? c->widget() : 0) << (c ? c->widget()->metaObject()->className() : 0);
    }
}

void MainWindow::aboutToShutdown()
{
    disconnect(QApplication::instance(), SIGNAL(focusChanged(QWidget*,QWidget*)),
               this, SLOT(updateFocusWidget(QWidget*,QWidget*)));
    m_activeContext.clear();
    hide();
}

static const char settingsGroup[] = "MainWindow";
static const char colorKey[] = "Color";
static const char windowGeometryKey[] = "WindowGeometry";
static const char windowStateKey[] = "WindowState";
static const char modeSelectorVisibleKey[] = "ModeSelectorVisible";

void MainWindow::readSettings()
{
    m_settings->beginGroup(QLatin1String(settingsGroup));

    if (m_overrideColor.isValid()) {
        Utils::StyleHelper::setBaseColor(m_overrideColor);
        // Get adapted base color.
        m_overrideColor = Utils::StyleHelper::baseColor();
    } else {
        Utils::StyleHelper::setBaseColor(
                m_settings->value(QLatin1String(colorKey),
                                  QColor(Utils::StyleHelper::DEFAULT_BASE_COLOR)).value<QColor>());
    }

    if (!restoreGeometry(m_settings->value(QLatin1String(windowGeometryKey)).toByteArray()))
        resize(1008, 700); // size without window decoration
    restoreState(m_settings->value(QLatin1String(windowStateKey)).toByteArray());

    bool modeSelectorVisible = m_settings->value(QLatin1String(modeSelectorVisibleKey), true).toBool();
    ModeManager::setModeSelectorVisible(modeSelectorVisible);
    m_toggleModeSelectorAction->setChecked(modeSelectorVisible);

    m_settings->endGroup();

    m_editorManager->readSettings();
    m_navigationWidget->restoreSettings(m_settings);
    m_rightPaneWidget->readSettings(m_settings);
}

void MainWindow::writeSettings()
{
    m_settings->beginGroup(QLatin1String(settingsGroup));

    if (!(m_overrideColor.isValid() && Utils::StyleHelper::baseColor() == m_overrideColor))
        m_settings->setValue(QLatin1String(colorKey), Utils::StyleHelper::requestedBaseColor());

    m_settings->setValue(QLatin1String(windowGeometryKey), saveGeometry());
    m_settings->setValue(QLatin1String(windowStateKey), saveState());
    m_settings->setValue(QLatin1String(modeSelectorVisibleKey), ModeManager::isModeSelectorVisible());

    m_settings->endGroup();

    DocumentManager::saveSettings();
    m_actionManager->saveSettings(m_settings);
    m_editorManager->saveSettings();
    m_navigationWidget->saveSettings(m_settings);
}

void MainWindow::updateAdditionalContexts(const Context &remove, const Context &add)
{
    foreach (const Id id, remove) {
        if (!id.isValid())
            continue;

        int index = m_additionalContexts.indexOf(id);
        if (index != -1)
            m_additionalContexts.removeAt(index);
    }

    foreach (const Id id, add) {
        if (!id.isValid())
            continue;

        if (!m_additionalContexts.contains(id))
            m_additionalContexts.prepend(id);
    }

    updateContext();
}

void MainWindow::updateContext()
{
    Context contexts;

    foreach (IContext *context, m_activeContext)
        contexts.add(context->context());

    contexts.add(m_additionalContexts);

    Context uniquecontexts;
    for (int i = 0; i < contexts.size(); ++i) {
        const Id id = contexts.at(i);
        if (!uniquecontexts.contains(id))
            uniquecontexts.add(id);
    }

    m_actionManager->setContext(uniquecontexts);
    emit m_coreImpl->contextChanged(m_activeContext, m_additionalContexts);
}
//处理File菜单的aboutToShow信号 此信号在菜单显示给用户之前发出,就是点击File菜单立刻会先调用这个函数
void MainWindow::aboutToShowRecentFiles()
{
    ActionContainer *aci =
        ActionManager::actionContainer(Constants::M_FILE_RECENTFILES);
    aci->menu()->clear();

    bool hasRecentFiles = false;
    foreach (const DocumentManager::RecentFile &file, DocumentManager::recentFiles()) {
        hasRecentFiles = true;
        QAction *action = aci->menu()->addAction(
                    QDir::toNativeSeparators(Utils::withTildeHomePath(file.first)));
        action->setData(qVariantFromValue(file));
        connect(action, SIGNAL(triggered()), this, SLOT(openRecentFile()));
    }
    aci->menu()->setEnabled(hasRecentFiles);

    // add the Clear Menu item
    if (hasRecentFiles) {
        aci->menu()->addSeparator();
        QAction *action = aci->menu()->addAction(QCoreApplication::translate(
                                                     "Core", Core::Constants::TR_CLEAR_MENU));
        connect(action, SIGNAL(triggered()), DocumentManager::instance(), SLOT(clearRecentFiles()));
    }
}

void MainWindow::openRecentFile()
{
    if (const QAction *action = qobject_cast<const QAction*>(sender())) {
        const DocumentManager::RecentFile file = action->data().value<DocumentManager::RecentFile>();
        EditorManager::openEditor(file.first, file.second);
    }
}
//菜单栏目 Help->AboutQtCreator 事件
void MainWindow::aboutQtCreator()
{
    if (!m_versionDialog) {
        m_versionDialog = new VersionDialog(this);
        //对话框结束挂接处理
        connect(m_versionDialog, SIGNAL(finished(int)), 
                this, SLOT(destroyVersionDialog()));
    }
    m_versionDialog->show();
}

//About->About QtCreator菜单项弹出的关于对话框的关闭操作事件
void MainWindow::destroyVersionDialog()
{
    if (m_versionDialog) {
        m_versionDialog->deleteLater();
        m_versionDialog = 0;
    }
}
//菜单栏目 Help->AboutPlugins 事件
void MainWindow::aboutPlugins()
{
    PluginDialog dialog(this);
    dialog.exec();
}

QPrinter *MainWindow::printer() const
{
    if (!m_printer)
        m_printer = new QPrinter(QPrinter::HighResolution);
    return m_printer;
}
//菜单栏目 Window->Full Screen 事件
void MainWindow::setFullScreen(bool on)
{
#if defined(Q_OS_MAC)
    Q_UNUSED(on)
    MacFullScreen::toggleFullScreen(this);
#else
    if (bool(windowState() & Qt::WindowFullScreen) == on)
        return;

    if (on) {
        setWindowState(windowState() | Qt::WindowFullScreen);
        //statusBar()->hide();
        //menuBar()->hide();
    } else {
        setWindowState(windowState() & ~Qt::WindowFullScreen);
        //menuBar()->show();
        //statusBar()->show();
    }
#endif
}

// Display a warning with an additional button to open
// the debugger settings dialog if settingsId is nonempty.

bool MainWindow::showWarningWithOptions(const QString &title,
                                        const QString &text,
                                        const QString &details,
                                        Id settingsCategory,
                                        Id settingsId,
                                        QWidget *parent)
{
    if (parent == 0)
        parent = this;
    QMessageBox msgBox(QMessageBox::Warning, title, text,
                       QMessageBox::Ok, parent);
    if (!details.isEmpty())
        msgBox.setDetailedText(details);
    QAbstractButton *settingsButton = 0;
    if (settingsId.isValid() || settingsCategory.isValid())
        settingsButton = msgBox.addButton(tr("Settings..."), QMessageBox::AcceptRole);
    msgBox.exec();
    if (settingsButton && msgBox.clickedButton() == settingsButton)
        return showOptionsDialog(settingsCategory, settingsId);
    return false;
}
