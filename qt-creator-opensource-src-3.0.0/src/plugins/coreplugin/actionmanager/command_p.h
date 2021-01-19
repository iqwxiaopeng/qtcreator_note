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
	//这个类是对一个action的封装 是 对menu，button 或者捷径的封装, 只能通过ActionManager::registerAction()获得
class CommandPrivate : public Core::Command
{
    Q_OBJECT
public:
    CommandPrivate(Id id);
    virtual ~CommandPrivate() {}

	//如果用户没有自定义快捷方式或将快捷方式重置为默认快捷方式,则使用此选项
    void setDefaultKeySequence(const QKeySequence &key);

	//返回可用于激活此命令的默认键盘快捷方式。
    QKeySequence defaultKeySequence() const;

    void setKeySequence(const QKeySequence &key);

	//设置用于在"键盘快捷方式设置"对话框中表示命令的文本.如果不设置此设置,则会执行用户可见操作中的当前文本（在许多情况下都可以）.
    void setDescription(const QString &text);

	//返回表示本命令的描述文本
    QString description() const;

    Id id() const;

    Context context() const;

	//向此命令的属性添加属性
    void setAttribute(CommandAttribute attr);

	//从该命令的属性中删除属性
    void removeAttribute(CommandAttribute attr);

	//返回命令是否具有属性集
    bool hasAttribute(CommandAttribute attr) const;

    virtual void setCurrentContext(const Context &context) = 0;

	//返回当前分配给此命令的带有键盘快捷方式附加表示形式的字符串。
    QString stringWithAppendedShortcut(const QString &str) const;

protected:
    Context m_context;
    CommandAttributes m_attributes;
    Id m_id;
    QKeySequence m_defaultKey;
    QString m_defaultText;
    bool m_isKeyInitialized;
};

//对一个快捷键的封装
class Shortcut : public CommandPrivate
{
    Q_OBJECT
public:
    Shortcut(Id id);

    void setKeySequence(const QKeySequence &key);

	//返回分配给此命令的当前键盘快捷方式
    QKeySequence keySequence() const;

    void setShortcut(QShortcut *shortcut);
    QShortcut *shortcut() const;

    QAction *action() const { return 0; }

    void setContext(const Context &context);
    Context context() const;
    void setCurrentContext(const Context &context);

	//返回命令是否具有当前上下文的活动操作或快捷方式
    bool isActive() const;

	//返回命令是否可以脚本化.一个脚本命令可以从脚本调用,而无需用户与它互动
    bool isScriptable() const;

	//返回命令是否可以为当前上下文脚本化
    bool isScriptable(const Context &) const;
    void setScriptable(bool value);

private:
    QShortcut *m_shortcut;
    bool m_scriptable;
};

//对于QAction的封装
class Action : public CommandPrivate
{
    Q_OBJECT
public:
    Action(Id id);

    void setKeySequence(const QKeySequence &key);

	//返回分配给此命令的当前键盘快捷方式
    QKeySequence keySequence() const;

    QAction *action() const;
    QShortcut *shortcut() const { return 0; }

    void setCurrentContext(const Context &context);

	//返回命令是否具有当前上下文的活动操作或快捷方式
    bool isActive() const;
    void addOverrideAction(QAction *action, const Context &context, bool scriptable);
    void removeOverrideAction(QAction *action);
    bool isEmpty() const;

	//返回命令是否可以脚本化.一个脚本命令可以从脚本调用,而无需用户与它互动
    bool isScriptable() const;

	//返回命令是否可以为当前上下文脚本化
    bool isScriptable(const Context &context) const;

    void setAttribute(CommandAttribute attr);
    void removeAttribute(CommandAttribute attr);

private slots:
    void updateActiveState();

private:
    void setActive(bool state);

    Utils::ProxyAction *m_action;//这个实际上就是一个QAction对象
    QString m_toolTip;

    QMap<Id, QPointer<QAction> > m_contextActionMap;
    QMap<QAction*, bool> m_scriptableMap;
    bool m_active;
    bool m_contextInitialized;
};

} // namespace Internal
} // namespace Core

#endif // COMMAND_P_H

