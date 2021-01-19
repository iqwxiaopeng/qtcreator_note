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

	//��ṹ���� ��id �� ���Ա�б�
struct Group
{
    Group(const Id &id) : id(id) {}
    Id id;
    QList<QObject *> items; // Command * or ActionContainer *
};

//��������������� ���ǲ˵�����
class ActionContainerPrivate : public Core::ActionContainer
{
    Q_OBJECT

public:
    ActionContainerPrivate(Id id);
    ~ActionContainerPrivate() {}

	//Ĭ��ֵ�ǲ˵���actionContainer:��Disable���˵�����actionContainer::Show.
	//���ò˵���Ϊ
    void setOnAllDisabledBehavior(OnAllDisabledBehavior behavior);

	//���ڲ˵���Ĭ��ֵΪActionContainer:��Disable�����ڲ˵�����Ĭ��ֵΪActionContainer : ��Show��
	//��ȡĬ����Ϊ
    ActionContainer::OnAllDisabledBehavior onAllDisabledBehavior() const;

	//������id���ر�ʾ��Ĳ���
    QAction *insertLocation(Id groupId) const;


	//��IDΪ group������ӵ�����������.ʹ����,�����Խ����������ָ�Ϊ�߼�����,��ֱ������Щ������Ӳ����Ͳ˵���
    void appendGroup(Id id);
    void insertGroup(Id before, Id groupId);

	//��������Ϊ�˵�����ӵ��˲�������,�ò�������Ϊָ��������һ�����
    void addAction(Command *action, Id group = Id());

	//���˵���Ϊ�Ӳ˵���ӵ��˲�������.�˵���Ϊָ��������һ��
    void addMenu(ActionContainer *menu, Id group = Id());
    void addMenu(ActionContainer *before, ActionContainer *menu, Id group = Id());
    Command *addSeparator(const Context &context, Id group = Id(), QAction **outSeparator = 0);

	//��������ڵ����нڵ�
    virtual void clear();

	//��ȡidֵ
    Id id() const;

	//���ش˲���������ʾ��qmenuʵ��,����˲���������ʾ�˵���,�򷵻�0.
    QMenu *menu() const;

	//���ش˲���������ʾ��qmenubarʵ��,����˲���������ʾ�˵�,�򷵻�0.
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
    QList<Group> m_groups; //�����е���Ŀ

private slots:
    void scheduleUpdate();
    void update();
    void itemDestroyed();

private:
    QList<Group>::const_iterator findGroup(Id groupId) const;
    QAction *insertLocation(QList<Group>::const_iterator group) const;

    OnAllDisabledBehavior m_onAllDisabledBehavior; //����Ĭ����Ϊ ��ʾ,����,������
    Id m_id; //����id
    bool m_updateRequested;
};

/*
Menu�Ĳ������� ���QT��QMenu�ػ���װ
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
    QMenu *m_menu; //����ָ����QMenu����
};

/*
MenuBar�Ĳ������� ���QT��QMenuBar�ػ���װ�ػ�
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
    QMenuBar *m_menuBar; //����ָ����QMenuBar����
};

} // namespace Internal
} // namespace Core

#endif // ACTIONCONTAINER_P_H
   
