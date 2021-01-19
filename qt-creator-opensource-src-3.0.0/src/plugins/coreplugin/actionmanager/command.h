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
这个类是对一个action的封装 是 对menu，button 或者捷径的封装,只能通过ActionManager::registerAction()获得
*/
class CORE_EXPORT Command : public QObject
{
    Q_OBJECT
public:
    enum CommandAttribute {
        CA_Hide = 1, //指示QAction的显示和隐藏
        CA_UpdateText = 2,//更新QAction的Text
        CA_UpdateIcon = 4, //更新QAction的Icon
        CA_NonConfigurable = 8 //指示QAction键盘快捷键不应该由用户配置
    };
    Q_DECLARE_FLAGS(CommandAttributes, CommandAttribute)

	//如果用户没有自定义快捷方式或将快捷方式重置为默认快捷方式,则使用此选项
    virtual void setDefaultKeySequence(const QKeySequence &key) = 0;

	//返回可用于激活此命令的默认键盘快捷方式。
    virtual QKeySequence defaultKeySequence() const = 0;

	//返回分配给此命令的当前键盘快捷方式
    virtual QKeySequence keySequence() const = 0;

    // explicitly set the description (used e.g. in shortcut settings)
    // default is to use the action text for actions, or the whatsThis for shortcuts,
    // or, as a last fall back if these are empty, the command ID string
    // override the default e.g. if the text is context dependent and contains file names etc
	//设置用于在"键盘快捷方式设置"对话框中表示命令的文本.如果不设置此设置,则会执行用户可见操作中的当前文本（在许多情况下都可以）.
    virtual void setDescription(const QString &text) = 0;
	//返回表示本命令的描述文本
    virtual QString description() const = 0;

    virtual Id id() const = 0;

	//返回此命令的用户可见操作.如果该命令表示快捷方式,则返回NULL
    virtual QAction *action() const = 0;

	//返回此命令的快捷方式.如果该命令表示一个操作,则返回空值.
    virtual QShortcut *shortcut() const = 0;


    virtual Context context() const = 0;

	//向此命令的属性添加属性
    virtual void setAttribute(CommandAttribute attr) = 0;

	//从该命令的属性中删除属性
    virtual void removeAttribute(CommandAttribute attr) = 0;

	//返回命令是否具有属性集
    virtual bool hasAttribute(CommandAttribute attr) const = 0;

	//返回命令是否具有当前上下文的活动操作或快捷方式
    virtual bool isActive() const = 0;

    virtual void setKeySequence(const QKeySequence &key) = 0;

	//返回当前分配给此命令的带有键盘快捷方式附加表示形式的字符串。
    virtual QString stringWithAppendedShortcut(const QString &str) const = 0;

	//返回命令是否可以脚本化.一个脚本命令可以从脚本调用,而无需用户与它互动
    virtual bool isScriptable() const = 0;
	//返回命令是否可以为当前上下文脚本化
    virtual bool isScriptable(const Context &) const = 0;

signals:

    //当分配给此命令的键盘快捷方式更改时发送,例如当用户在“键盘快捷方式设置”对话框中设置它时。
    void keySequenceChanged();
    void activeStateChanged();
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::Command::CommandAttributes)

#endif // COMMAND_H
