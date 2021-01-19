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
//���Ŀ��ṩ�Ļ������ܷ�װ ������
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

    //��һ���Ի����û����ԴӴ������ļ��������Ŀ��һ�����н���ѡ��
    //���������ʾΪ�Ի�����⡣�������ļ���·��������û������ģ�������Ĭ��λ�á���Ĭ��Ϊ�ļ���������ǰ�ļ���·��
    static void showNewItemDialog(const QString &title,
                                  const QList<IWizard *> &wizards,
                                  const QString &defaultLocation = QString(),
                                  const QVariantMap &extraVariables = QVariantMap());

    //�򿪡�Ӧ�ó���GUIѡ�����GUI��ѡ����Ի��򣬲���ָ������Ԥ��ѡ��ҳ�档
    //��Щ����������ӦIOptionsPage���ַ���ID��
    static bool showOptionsDialog(Id group, Id page, QWidget *parent = 0);

    //��ʾһ��������Ϣ�����а���һ��������ҳ�İ�ť��
    //Ӧ������ʾ���ô��󲢽��û�ָ�����á�����������öԻ����򷵻�true��
    static bool showWarningWithOptions(const QString &title, const QString &text,
                                       const QString &details = QString(),
                                       Id settingsCategory = Id(),
                                       Id settingsId = Id(),
                                       QWidget *parent = 0);

    //����Ӧ�ó���������ö���������ʹ����������������Ӧ�ó���Χ�����ã���Ự����Ŀ�ض������ò�ͬ����
    //�����Χ��qsettings : ��userscope��Ĭ��ֵ�����򽫴��û������ж�ȡ�û����ã������ص�qc�ṩ��ȫ�����á�
    //�����Χ��qsettings : ��system scope����ֻ��ȡ��ǰ�汾qc������ϵͳ���á��˹��ܽ������ڲ�Ŀ�ġ�
    static QSettings *settings(QSettings::Scope scope = QSettings::UserScope);

    //����Ӧ�ó�����������ݿ⡣
    //�������ݿ���������������ö��������ʺϴ洢�������ݡ���Щ������Ӧ�ó���Χ�ڡ�
    static SettingsDatabase *settingsDatabase();

    //����Ӧ�ó���Ĵ�ӡ������
    //ʼ��ʹ�ô˴�ӡ��������д�ӡ�����Ӧ�ó���Ĳ�ͬ��������ʹ�������á�
    static QPrinter *printer();


    static QString userInterfaceLanguage();

    //����������Ŀģ��͵����������Դ�ľ���·����
    //��Ҫ���ֳ�����������ص�ƽ̨�ض����룬��Ϊ��Mac OS X�ϣ����磬��Դ��Ӧ�ó������һ���֡�
    static QString resourcePath();

    //�����û�Ŀ¼��������Ŀģ�����Դ�ľ���·����
    //ʹ�ô˺������Բ����û�����д�����Դλ�ã����磬�����Զ����ɫ���ģ�塣
    static QString userResourcePath();
    static QString documentationPath();
    static QString libexecPath();

    static QString versionString();
    static QString buildCompatibilityString();

    //������Ӧ�ó��򴰿ڡ������Ի��򸸼����ȵȡ�
    static QWidget *mainWindow();
    static QStatusBar *statusBar();
    /* Raises and activates the window for the widget. This contains workarounds for X11. */
    static void raiseWindow(QWidget *widget);

    //���ص�ǰ�������ĵ������Ķ���
    static IContext *currentContextObject();

    //���ĵ�ǰ������������ġ�
    //ɾ����removeָ���ĸ����������б��������addָ���ĸ����������б�
    // Adds and removes additional active contexts, these contexts are appended
    // to the currently active contexts.
    static void updateAdditionalContexts(const Context &remove, const Context &add);

    //ע�����������Ķ���
    //ע���ֻҪ�ؼ���ý��㣬�������Ķ���ͻ��Զ���õ�ǰ�����Ķ���
    static void addContextObject(IContext *context);
    //����֪�������б���ע�������Ķ���
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
    void coreOpened();//ָʾ�Ѽ������в������ʾ�����ڡ�
    void newItemsDialogRequested();
    void saveSettingsRequested(); //��ʾ�û�������ȫ�����ñ��浽���̡���Ӧ�ó���ر�ʱ������GUI��������ʱ�������������
    void optionsDialogRequested(); //����������ʾGUI����>GUIѡ��Ի���֮ǰִ�в�����

    //������ִ��һЩ�������ڽ���ǰ�Ĳ�����
    //��Ӧ�ó���֤�ڷ������źź�رա�
    //Ϊ�˷�����������Ƕ���ͨ����������ں�������iplugin : ��aboutToShutdown��������һ�����䡣
    void coreAboutToClose();
    void contextAboutToChange(const QList<Core::IContext *> &context); //ָʾ�������Ľ��ܿ��Ϊ��ǰ�����ģ���ζ����С������ý��㣩��

    //ָʾ�������ĸոճ�Ϊ��ǰ�����ģ���ζ����С������ý��㣩��������AdditionalContextsָ��������������ID�Ѹ��ġ�
    void contextChanged(const QList<Core::IContext *> &context, const Core::Context &additionalContexts);
};

} // namespace Core

#endif // ICORE_H
