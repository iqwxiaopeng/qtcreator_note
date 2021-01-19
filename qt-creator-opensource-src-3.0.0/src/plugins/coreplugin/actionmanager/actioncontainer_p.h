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

#ifndef ACTIONCONTAINER_P_H
#define ACTIONCONTAINER_P_H

#include "actionmanager_p.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>

namespace Core {
namespace Internal {

	//组结构定义 组id 和 组成员列表
struct Group
{
    Group(const Id &id) : id(id) {}
    Id id;
    QList<QObject *> items; // Command * or ActionContainer *
};

//具体操作容器定义 就是菜单布局
class ActionContainerPrivate : public Core::ActionContainer
{
    Q_OBJECT

public:
    ActionContainerPrivate(Id id);
    ~ActionContainerPrivate() {}

	//默认值是菜单的actionContainer:：Disable，菜单栏的actionContainer::Show.
	//设置菜单行为
    void setOnAllDisabledBehavior(OnAllDisabledBehavior behavior);

	//对于菜单，默认值为ActionContainer:：Disable，对于菜单栏，默认值为ActionContainer : ：Show。
	//获取默认行为
    ActionContainer::OnAllDisabledBehavior onAllDisabledBehavior() const;

	//根据组id返回表示组的操作
    QAction *insertLocation(Id groupId) const;


	//将ID为 group的组添加到操作容器中.使用组,您可以将操作容器分割为逻辑部分,并直接向这些部分添加操作和菜单。
    void appendGroup(Id id);
    void insertGroup(Id before, Id groupId);

	//将操作作为菜单项添加到此操作容器,该操作将作为指定组的最后一项添加
    void addAction(Command *action, Id group = Id());

	//将菜单作为子菜单添加到此操作容器.菜单作为指定组的最后一项
    void addMenu(ActionContainer *menu, Id group = Id());
    void addMenu(ActionContainer *before, ActionContainer *menu, Id group = Id());
    Command *addSeparator(const Context &context, Id group = Id(), QAction **outSeparator = 0);

	//清除容器内的所有节点
    virtual void clear();

	//获取id值
    Id id() const;

	//返回此操作容器表示的qmenu实例,如果此操作容器表示菜单栏,则返回0.
    QMenu *menu() const;

	//返回此操作容器表示的qmenubar实例,如果此操作容器表示菜单,则返回0.
    QMenuBar *menuBar() const;

    virtual void insertAction(QAction *before, QAction *action) = 0;
    virtual void insertMenu(QAction *before, QMenu *menu) = 0;

    virtual void removeAction(QAction *action) = 0;
    virtual void removeMenu(QMenu *menu) = 0;

    virtual bool updateInternal() = 0;

protected:
    bool canAddAction(Command *action) const;
    bool canAddMenu(ActionContainer *menu) const;
    virtual bool canBeAddedToMenu() const = 0;

    // groupId --> list of Command* and ActionContainer*
    QList<Group> m_groups; //容器中的条目

private slots:
    void scheduleUpdate();
    void update();
    void itemDestroyed();

private:
    QList<Group>::const_iterator findGroup(Id groupId) const;
    QAction *insertLocation(QList<Group>::const_iterator group) const;

    OnAllDisabledBehavior m_onAllDisabledBehavior; //容器默认行为 显示,隐藏,不启用
    Id m_id; //容器id
    bool m_updateRequested;
};

/*
Menu的操作容器 针对QT的QMenu特化包装
*/
class MenuActionContainer : public ActionContainerPrivate
{
public:
    explicit MenuActionContainer(Id id);

    void setMenu(QMenu *menu);
    QMenu *menu() const;

    void insertAction(QAction *before, QAction *action);
    void insertMenu(QAction *before, QMenu *menu);

    void removeAction(QAction *action);
    void removeMenu(QMenu *menu);

protected:
    bool canBeAddedToMenu() const;
    bool updateInternal();

private:
    QMenu *m_menu; //容器指代的QMenu对象
};

/*
MenuBar的操作容器 针对QT的QMenuBar特化包装特化
*/
class MenuBarActionContainer : public ActionContainerPrivate
{
public:
    explicit MenuBarActionContainer(Id id);

    void setMenuBar(QMenuBar *menuBar);
    QMenuBar *menuBar() const;

    void insertAction(QAction *before, QAction *action);
    void insertMenu(QAction *before, QMenu *menu);

    void removeAction(QAction *action);
    void removeMenu(QMenu *menu);

protected:
    bool canBeAddedToMenu() const;
    bool updateInternal();

private:
    QMenuBar *m_menuBar; //容器指代的QMenuBar对象
};

} // namespace Internal
} // namespace Core

#endif // ACTIONCONTAINER_P_H
   
