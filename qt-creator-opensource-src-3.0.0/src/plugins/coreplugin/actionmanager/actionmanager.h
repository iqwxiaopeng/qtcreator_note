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
//本类负责注册和管理菜单和菜单项和键盘快捷键,提供统一的访问接口
class CORE_EXPORT ActionManager : public QObject
{
    Q_OBJECT
public:
    static ActionManager *instance();


    //创建具有给定ID的新菜单。
    //返回可用于获取qmenu实例的新actionContainer或者向菜单中添加菜单项。。ActionManager拥有返回的ActionContainer。
    static ActionContainer *createMenu(Id id);

    //创建具有给定ID的新菜单栏。
    //返回可用于获取qmenubar实例的新actionContainer或者将菜单添加到菜单栏。ActionManager拥有返回的ActionContainer。
    static ActionContainer *createMenuBar(Id id);

    //向系统指定ID下注册一个操作。
    //返回一个命令对象，该对象表示应用程序中的操作，属于ActionManager。只要上下文不同，就可以用相同的ID注册多个操作。在这种情况下，实际操作的触发器将转发到当前活动上下文的已注册Qaction。可以从脚本调用可脚本操作，而无需用户与之交互。
    static Command *registerAction(QAction *action, Id id, const Context &context, bool scriptable = false);

    //向系统指定ID下注册一个快捷方式。
    //返回一个命令对象，该对象表示应用程序中的快捷方式，属于ActionManager。只要上下文不同，就可以注册多个具有相同ID的快捷方式。在这种情况下，实际快捷方式的触发器将转发到当前活动上下文的已注册qshortcut。可以从脚本调用可编写脚本的快捷方式，而无需用户与之交互。
    static Command *registerShortcut(QShortcut *shortcut, Id id, const Context &context, bool scriptable = false);

    //返回系统在给定ID下已知的命令对象。
    static Command *command(Id id);

    //返回给定ID下系统已知的IActionContainer对象。
    static ActionContainer *actionContainer(Id id);

    //返回已注册的所有命令。
    static QList<Command *> commands();

    //删除有关指定ID下的操作
    //通常不需要注销操作。注销操作的唯一有效用例是表示用户可定义操作的操作，如自定义定位器筛选器。如果用户删除了这样的操作，它还必须从操作管理器中注销，以使其从快捷方式设置等中消失。
    static void unregisterAction(QAction *action, Id id);

    //删除有关指定ID下的快捷方式
    //通常不需要注销快捷方式。注销快捷方式的唯一有效用例是表示用户可定义操作的快捷方式。如果用户删除了这样的操作，则相应的快捷方式也必须从操作管理器中注销，以使其从快捷方式设置等中消失。
    static void unregisterShortcut(Id id);

	//设置是否开启演示模式
    static void setPresentationModeEnabled(bool enabled);
    static bool isPresentationModeEnabled(); //是否演示模式

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
