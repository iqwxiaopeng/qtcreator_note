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

	//��������ɫ�ʷ��
    void setOverrideColor(const QColor &color);

	//�����Ƿ�ȫ��
    void setIsFullScreen(bool fullScreen);
signals:
    void windowActivated();

public slots:
    void newFile();//�˵���Ŀ File->NewFile �¼�
    void openFileWith();//�˵���Ŀ File->OpenFileWith �¼�
    void exit();//�˵���Ŀ File->Exit �¼�
    void setFullScreen(bool on);//�˵���Ŀ Window->Full Screen �¼�

    //�˵���Ŀ File->NewFile...�¼��������
    void showNewItemDialog(const QString &title,
                           const QList<IWizard *> &wizards,
                           const QString &defaultLocation = QString(),
                           const QVariantMap &extraVariables = QVariantMap());
    //�˵���Ŀ Tools->Options Tools->External->Configure...�¼�
    bool showOptionsDialog(Id category = Id(), Id page = Id(), QWidget *parent = 0);

    bool showWarningWithOptions(const QString &title, const QString &text,
                                const QString &details = QString(),
                                Id settingsCategory = Id(),
                                Id settingsId = Id(),
                                QWidget *parent = 0);

protected:
    //change eventһ���ǵ�ǰwidget״̬�ı�󴥷���������ı�,���Ըı�֮���.�÷�����Ҫ����ı��¼�,�����Ըı��,ִ����ز���
    virtual void changeEvent(QEvent *e);

    //closeEvent ���ڹرյ���
    virtual void closeEvent(QCloseEvent *event);

    //�������ק�����¼� EG��קһ���ļ����༭�� �ϷŲ�����Ϊ������Ȼ��ͬ�Ķ���: �϶��ͷ���.dragEnterEvent �� dropEvent
    virtual void dragEnterEvent(QDragEnterEvent *event);
    //�ϷŲ�����Ϊ������Ȼ��ͬ�Ķ���: �϶��ͷ���.dragEnterEvent �� dropEvent
    virtual void dropEvent(QDropEvent *event);

private slots:
    void openFile(); //�˵���Ŀ File->OpenFile �¼�
    void aboutToShowRecentFiles();//����File�˵���aboutToShow�ź� ���ź��ڲ˵���ʾ���û�֮ǰ����,���ǵ��File�˵����̻��ȵ����������
    void openRecentFile();//�˵���Ŀ File->OpenRecentFile �¼�
    void setFocusToEditor();//���� esc �¼�����
    void saveAll();//�˵���Ŀ File->Save All �¼�
    void aboutQtCreator();//�˵���Ŀ Help->AboutQtCreator �¼�
    void aboutPlugins();//�˵���Ŀ Help->AboutPlugins �¼�
    void updateFocusWidget(QWidget *old, QWidget *now);//��ϵͳ���㷢���ı��ʱ�򣬾ͻᷢ��focusChanged�ź�,��updateFocusWidget������������ź�
    void setSidebarVisible(bool visible);//�˵��� Window->Show Silderbar�¼�
    void destroyVersionDialog();//About->About QtCreator�˵�����Ĺ��ڶԻ���Ĺرղ����¼�
    void openDelayedFiles();

private:
    void updateContextObject(const QList<IContext *> &context);
    void updateContext();

    void registerDefaultContainers();//�˵���Ŀ����Ʋ��ֹ�������
    void registerDefaultActions();//���������ڲ���ʱû�����Ĳ˵�����Ŀ,��Ϊ�˵���������Ŀ��������ӺͰ󶨲���ͬʱע�ᵽ����������

    void readSettings();
    void writeSettings();

    ICore *m_coreImpl;
    Context m_additionalContexts;
    QSettings *m_settings; //�û������ļ�
    QSettings *m_globalSettings; //ȫ�������ļ�
    SettingsDatabase *m_settingsDatabase; //�����û������ļ��������������ݿ�
    mutable QPrinter *m_printer; //QT�Ĵ�ӡ��
    ActionManager *m_actionManager;//������������,ע��Action����������
    EditorManager *m_editorManager;
    ExternalToolManager *m_externalToolManager;
    MessageManager *m_messageManager;
    ProgressManagerPrivate *m_progressManager;
    VariableManager *m_variableManager;
    VcsManager *m_vcsManager;
    StatusBarManager *m_statusBarManager;
    ModeManager *m_modeManager; //ģ�͹����� �ṩ��FancyTabWidget��Mode֮���ͨ������
    MimeDatabase *m_mimeDatabase;
    HelpManager *m_helpManager;
    FancyTabWidget *m_modeStack; //FancyTabWidget���ڴ�������ߵĹ�����,������FancyTabBar��CornerWidget��������,ͬʱ����������һ��StackedLayout��״̬��
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
    QAction *m_newAction;//File->New File�Ĳ�������
    QAction *m_openAction;//File->Open File�Ĳ�������
    QAction *m_openWithAction;//File->Open File With�Ĳ�������
    QAction *m_saveAllAction;//File->Save All�Ĳ�������
    QAction *m_exitAction;//File->Exit�Ĳ�������
    QAction *m_optionsAction;//Tools->Options�Ĳ�������
    QAction *m_toggleSideBarAction;//Window->Show Sidebar�Ĳ�������
    QAction *m_toggleModeSelectorAction;
    QAction *m_toggleFullScreenAction;//Window->Full Screenr�Ĳ�������
    QAction *m_minimizeAction;//ֻ��mac��Ч
    QAction *m_zoomAction;

    QToolButton *m_toggleSideBarButton;
    QColor m_overrideColor; //����ɫ�ʷ�� �����������,�����ϵ���ർ�������˵���,״̬��������������ɫ

    QStringList m_filesToOpenDelayed;
};

} // namespace Internal
} // namespace Core

#endif // MAINWINDOW_H
