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

#ifndef ACTIONMANAGER_H
#define ACTIONMANAGER_H

#include "coreplugin/core_global.h"
#include "coreplugin/id.h"
#include <coreplugin/actionmanager/command.h>

#include <QObject>
#include <QList>

QT_BEGIN_NAMESPACE
class QAction;
class QSettings;
class QShortcut;
class QString;
QT_END_NAMESPACE

namespace Core {

class ActionContainer;

namespace Internal { class MainWindow; }
//���ฺ��ע��͹���˵��Ͳ˵���ͼ��̿�ݼ�,�ṩͳһ�ķ��ʽӿ�
class CORE_EXPORT ActionManager : public QObject
{
    Q_OBJECT
public:
    static ActionManager *instance();


    //�������и���ID���²˵���
    //���ؿ����ڻ�ȡqmenuʵ������actionContainer������˵�����Ӳ˵����ActionManagerӵ�з��ص�ActionContainer��
    static ActionContainer *createMenu(Id id);

    //�������и���ID���²˵�����
    //���ؿ����ڻ�ȡqmenubarʵ������actionContainer���߽��˵���ӵ��˵�����ActionManagerӵ�з��ص�ActionContainer��
    static ActionContainer *createMenuBar(Id id);

    //��ϵͳָ��ID��ע��һ��������
    //����һ��������󣬸ö����ʾӦ�ó����еĲ���������ActionManager��ֻҪ�����Ĳ�ͬ���Ϳ�������ͬ��IDע��������������������£�ʵ�ʲ����Ĵ�������ת������ǰ������ĵ���ע��Qaction�����Դӽű����ÿɽű��������������û���֮������
    static Command *registerAction(QAction *action, Id id, const Context &context, bool scriptable = false);

    //��ϵͳָ��ID��ע��һ����ݷ�ʽ��
    //����һ��������󣬸ö����ʾӦ�ó����еĿ�ݷ�ʽ������ActionManager��ֻҪ�����Ĳ�ͬ���Ϳ���ע����������ͬID�Ŀ�ݷ�ʽ������������£�ʵ�ʿ�ݷ�ʽ�Ĵ�������ת������ǰ������ĵ���ע��qshortcut�����Դӽű����ÿɱ�д�ű��Ŀ�ݷ�ʽ���������û���֮������
    static Command *registerShortcut(QShortcut *shortcut, Id id, const Context &context, bool scriptable = false);

    //����ϵͳ�ڸ���ID����֪���������
    static Command *command(Id id);

    //���ظ���ID��ϵͳ��֪��IActionContainer����
    static ActionContainer *actionContainer(Id id);

    //������ע����������
    static QList<Command *> commands();

    //ɾ���й�ָ��ID�µĲ���
    //ͨ������Ҫע��������ע��������Ψһ��Ч�����Ǳ�ʾ�û��ɶ�������Ĳ��������Զ��嶨λ��ɸѡ��������û�ɾ���������Ĳ�������������Ӳ�����������ע������ʹ��ӿ�ݷ�ʽ���õ�����ʧ��
    static void unregisterAction(QAction *action, Id id);

    //ɾ���й�ָ��ID�µĿ�ݷ�ʽ
    //ͨ������Ҫע����ݷ�ʽ��ע����ݷ�ʽ��Ψһ��Ч�����Ǳ�ʾ�û��ɶ�������Ŀ�ݷ�ʽ������û�ɾ���������Ĳ���������Ӧ�Ŀ�ݷ�ʽҲ����Ӳ�����������ע������ʹ��ӿ�ݷ�ʽ���õ�����ʧ��
    static void unregisterShortcut(Id id);

	//�����Ƿ�����ʾģʽ
    static void setPresentationModeEnabled(bool enabled);
    static bool isPresentationModeEnabled(); //�Ƿ���ʾģʽ

signals:
    void commandListChanged();
    void commandAdded(const QString &id);

private:
    ActionManager(QObject *parent = 0);
    ~ActionManager();
    void initialize();
    void saveSettings(QSettings *settings);
    void setContext(const Context &context);

    friend class Core::Internal::MainWindow;
};

} // namespace Core

#endif // ACTIONMANAGER_H
