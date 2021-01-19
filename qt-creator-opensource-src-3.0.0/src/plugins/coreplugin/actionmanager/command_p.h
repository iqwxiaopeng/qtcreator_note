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

#ifndef COMMAND_P_H
#define COMMAND_P_H

#include "command.h"

#include <coreplugin/id.h>
#include <coreplugin/icontext.h>

#include <utils/proxyaction.h>

#include <QList>
#include <QMultiMap>
#include <QPointer>
#include <QMap>
#include <QKeySequence>

namespace Core {
namespace Internal {
	//������Ƕ�һ��action�ķ�װ �� ��menu��button ���߽ݾ��ķ�װ, ֻ��ͨ��ActionManager::registerAction()���
class CommandPrivate : public Core::Command
{
    Q_OBJECT
public:
    CommandPrivate(Id id);
    virtual ~CommandPrivate() {}

	//����û�û���Զ����ݷ�ʽ�򽫿�ݷ�ʽ����ΪĬ�Ͽ�ݷ�ʽ,��ʹ�ô�ѡ��
    void setDefaultKeySequence(const QKeySequence &key);

	//���ؿ����ڼ���������Ĭ�ϼ��̿�ݷ�ʽ��
    QKeySequence defaultKeySequence() const;

    void setKeySequence(const QKeySequence &key);

	//����������"���̿�ݷ�ʽ����"�Ի����б�ʾ������ı�.��������ô�����,���ִ���û��ɼ������еĵ�ǰ�ı������������¶����ԣ�.
    void setDescription(const QString &text);

	//���ر�ʾ������������ı�
    QString description() const;

    Id id() const;

    Context context() const;

	//�������������������
    void setAttribute(CommandAttribute attr);

	//�Ӹ������������ɾ������
    void removeAttribute(CommandAttribute attr);

	//���������Ƿ�������Լ�
    bool hasAttribute(CommandAttribute attr) const;

    virtual void setCurrentContext(const Context &context) = 0;

	//���ص�ǰ�����������Ĵ��м��̿�ݷ�ʽ���ӱ�ʾ��ʽ���ַ�����
    QString stringWithAppendedShortcut(const QString &str) const;

protected:
    Context m_context;
    CommandAttributes m_attributes;
    Id m_id;
    QKeySequence m_defaultKey;
    QString m_defaultText;
    bool m_isKeyInitialized;
};

//��һ����ݼ��ķ�װ
class Shortcut : public CommandPrivate
{
    Q_OBJECT
public:
    Shortcut(Id id);

    void setKeySequence(const QKeySequence &key);

	//���ط����������ĵ�ǰ���̿�ݷ�ʽ
    QKeySequence keySequence() const;

    void setShortcut(QShortcut *shortcut);
    QShortcut *shortcut() const;

    QAction *action() const { return 0; }

    void setContext(const Context &context);
    Context context() const;
    void setCurrentContext(const Context &context);

	//���������Ƿ���е�ǰ�����ĵĻ�������ݷ�ʽ
    bool isActive() const;

	//���������Ƿ���Խű���.һ���ű�������Դӽű�����,�������û���������
    bool isScriptable() const;

	//���������Ƿ����Ϊ��ǰ�����Ľű���
    bool isScriptable(const Context &) const;
    void setScriptable(bool value);

private:
    QShortcut *m_shortcut;
    bool m_scriptable;
};

//����QAction�ķ�װ
class Action : public CommandPrivate
{
    Q_OBJECT
public:
    Action(Id id);

    void setKeySequence(const QKeySequence &key);

	//���ط����������ĵ�ǰ���̿�ݷ�ʽ
    QKeySequence keySequence() const;

    QAction *action() const;
    QShortcut *shortcut() const { return 0; }

    void setCurrentContext(const Context &context);

	//���������Ƿ���е�ǰ�����ĵĻ�������ݷ�ʽ
    bool isActive() const;
    void addOverrideAction(QAction *action, const Context &context, bool scriptable);
    void removeOverrideAction(QAction *action);
    bool isEmpty() const;

	//���������Ƿ���Խű���.һ���ű�������Դӽű�����,�������û���������
    bool isScriptable() const;

	//���������Ƿ����Ϊ��ǰ�����Ľű���
    bool isScriptable(const Context &context) const;

    void setAttribute(CommandAttribute attr);
    void removeAttribute(CommandAttribute attr);

private slots:
    void updateActiveState();

private:
    void setActive(bool state);

    Utils::ProxyAction *m_action;//���ʵ���Ͼ���һ��QAction����
    QString m_toolTip;

    QMap<Id, QPointer<QAction> > m_contextActionMap;
    QMap<QAction*, bool> m_scriptableMap;
    bool m_active;
    bool m_contextInitialized;
};

} // namespace Internal
} // namespace Core

#endif // COMMAND_P_H

