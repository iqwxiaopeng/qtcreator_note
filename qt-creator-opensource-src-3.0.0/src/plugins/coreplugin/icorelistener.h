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

#ifndef ICORELISTENER_H
#define ICORELISTENER_H

#include "core_global.h"
#include <QObject>

namespace Core {
class IEditor;

//ICoreListener提供一些钩子（或者说相应函数）响应核心插件发出的事件
//任何继承与ICoreListener的类的变量的接口函数返回false,整个过程（如关闭过程）就会终止
//在使用时，需要将这些变量添加到对象池当中,并要在析构的时候从对象池移出
class CORE_EXPORT ICoreListener : public QObject
{
    Q_OBJECT
public:
    ICoreListener(QObject *parent = 0) : QObject(parent) {}
    virtual ~ICoreListener() {}

    virtual bool editorAboutToClose(IEditor * /*editor*/) { return true; }
    virtual bool coreAboutToClose() { return true; }
};

} // namespace Core

#endif // ICORELISTENER_H
