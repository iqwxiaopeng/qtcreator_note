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

#ifndef ACTIONCONTAINER_H
#define ACTIONCONTAINER_H

#include "coreplugin/icontext.h"

#include <QObject>

QT_BEGIN_NAMESPACE
class QMenu;
class QMenuBar;
class QAction;
QT_END_NAMESPACE

namespace Core {

class Command;
/*这个类代表QT Creator中的一个菜单 或者 菜单栏*/
class ActionContainer : public QObject
{
    Q_OBJECT

public:
    enum OnAllDisabledBehavior { //菜单行为
        Disable,//禁用
        Hide, //隐藏
        Show //显示
    };


    //默认值是菜单的actionContainer:：Disable，菜单栏的actionContainer : ：Show。
	//设置菜单行为
    virtual void setOnAllDisabledBehavior(OnAllDisabledBehavior behavior) = 0;


    //对于菜单，默认值为ActionContainer:：Disable，对于菜单栏，默认值为ActionContainer : ：Show。
	//获取默认行为
    virtual ActionContainer::OnAllDisabledBehavior onAllDisabledBehavior() const = 0;

	//每一个被管理器管理的玩意都给了这么一个id值
    virtual Id id() const = 0;

	//返回此操作容器表示的qmenu实例,如果此操作容器表示菜单栏,则返回0.
    virtual QMenu *menu() const = 0;

	//返回此操作容器表示的qmenubar实例,如果此操作容器表示菜单,则返回0.
    virtual QMenuBar *menuBar() const = 0;

	//根据组id返回表示组的操作
    virtual QAction *insertLocation(Id group) const = 0;

	//将ID为 group的组添加到操作容器中.使用组,您可以将操作容器分割为逻辑部分,并直接向这些部分添加操作和菜单。
    virtual void appendGroup(Id group) = 0;
    virtual void insertGroup(Id before, Id group) = 0;

	//将操作作为菜单项添加到此操作容器,该操作将作为指定组的最后一项添加
    virtual void addAction(Command *action, Id group = Id()) = 0;

	//将菜单作为子菜单添加到此操作容器.菜单作为指定组的最后一项
    virtual void addMenu(ActionContainer *menu, Id group = Id()) = 0;
    virtual void addMenu(ActionContainer *before, ActionContainer *menu, Id group = Id()) = 0;
    virtual Command *addSeparator(const Context &context, Id group = Id(), QAction **outSeparator = 0) = 0;

    // This clears this menu and submenus from all actions and submenus.
    // It does not destroy the submenus and commands, just removes them from their parents.
    virtual void clear() = 0;
};

} // namespace Core

#endif // ACTIONCONTAINER_H
