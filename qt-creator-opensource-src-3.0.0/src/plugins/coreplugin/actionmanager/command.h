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

#ifndef COMMAND_H
#define COMMAND_H

#include <coreplugin/core_global.h>
#include <coreplugin/id.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QAction;
class QShortcut;
class QKeySequence;
QT_END_NAMESPACE


namespace Core {

class Context;

#ifdef Q_OS_MAC
enum { UseMacShortcuts = 1 };
#else
enum { UseMacShortcuts = 0 };
#endif

/*
The Command class represents an action, such as a menu item, tool button, or shortcut.
A Command has two basic properties: a default shortcut and a default text. The default
������Ƕ�һ��action�ķ�װ �� ��menu��button ���߽ݾ��ķ�װ,ֻ��ͨ��ActionManager::registerAction()���
*/
class CORE_EXPORT Command : public QObject
{
    Q_OBJECT
public:
    enum CommandAttribute {
        CA_Hide = 1, //ָʾQAction����ʾ������
        CA_UpdateText = 2,//����QAction��Text
        CA_UpdateIcon = 4, //����QAction��Icon
        CA_NonConfigurable = 8 //ָʾQAction���̿�ݼ���Ӧ�����û�����
    };
    Q_DECLARE_FLAGS(CommandAttributes, CommandAttribute)

	//����û�û���Զ����ݷ�ʽ�򽫿�ݷ�ʽ����ΪĬ�Ͽ�ݷ�ʽ,��ʹ�ô�ѡ��
    virtual void setDefaultKeySequence(const QKeySequence &key) = 0;

	//���ؿ����ڼ���������Ĭ�ϼ��̿�ݷ�ʽ��
    virtual QKeySequence defaultKeySequence() const = 0;

	//���ط����������ĵ�ǰ���̿�ݷ�ʽ
    virtual QKeySequence keySequence() const = 0;

    // explicitly set the description (used e.g. in shortcut settings)
    // default is to use the action text for actions, or the whatsThis for shortcuts,
    // or, as a last fall back if these are empty, the command ID string
    // override the default e.g. if the text is context dependent and contains file names etc
	//����������"���̿�ݷ�ʽ����"�Ի����б�ʾ������ı�.��������ô�����,���ִ���û��ɼ������еĵ�ǰ�ı������������¶����ԣ�.
    virtual void setDescription(const QString &text) = 0;
	//���ر�ʾ������������ı�
    virtual QString description() const = 0;

    virtual Id id() const = 0;

	//���ش�������û��ɼ�����.����������ʾ��ݷ�ʽ,�򷵻�NULL
    virtual QAction *action() const = 0;

	//���ش�����Ŀ�ݷ�ʽ.����������ʾһ������,�򷵻ؿ�ֵ.
    virtual QShortcut *shortcut() const = 0;


    virtual Context context() const = 0;

	//�������������������
    virtual void setAttribute(CommandAttribute attr) = 0;

	//�Ӹ������������ɾ������
    virtual void removeAttribute(CommandAttribute attr) = 0;

	//���������Ƿ�������Լ�
    virtual bool hasAttribute(CommandAttribute attr) const = 0;

	//���������Ƿ���е�ǰ�����ĵĻ�������ݷ�ʽ
    virtual bool isActive() const = 0;

    virtual void setKeySequence(const QKeySequence &key) = 0;

	//���ص�ǰ�����������Ĵ��м��̿�ݷ�ʽ���ӱ�ʾ��ʽ���ַ�����
    virtual QString stringWithAppendedShortcut(const QString &str) const = 0;

	//���������Ƿ���Խű���.һ���ű�������Դӽű�����,�������û���������
    virtual bool isScriptable() const = 0;
	//���������Ƿ����Ϊ��ǰ�����Ľű���
    virtual bool isScriptable(const Context &) const = 0;

signals:

    //�������������ļ��̿�ݷ�ʽ����ʱ����,���統�û��ڡ����̿�ݷ�ʽ���á��Ի�����������ʱ��
    void keySequenceChanged();
    void activeStateChanged();
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::Command::CommandAttributes)

#endif // COMMAND_H
