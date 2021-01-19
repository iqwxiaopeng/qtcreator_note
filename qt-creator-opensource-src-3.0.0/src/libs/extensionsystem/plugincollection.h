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

#ifndef PLUGINCOLLECTION_H
#define PLUGINCOLLECTION_H

#include "extensionsystem_global.h"

#include <QList>
#include <QString>

namespace ExtensionSystem {
class PluginSpec;
/*插件集合类 其实只是插件描述文件的集合
*/
class EXTENSIONSYSTEM_EXPORT PluginCollection
{

public:
    //构造 传入插件集合名
    explicit PluginCollection(const QString& name);
    ~PluginCollection();

    //获取集合名
    QString name() const;
    //添加插件描述文件
    void addPlugin(PluginSpec *spec);
    //移除插件描述文件
    void removePlugin(PluginSpec *spec);
    //获取集合中所有的描述文件表
    QList<PluginSpec *> plugins() const;
private:
    QString m_name; //集合名
    QList<PluginSpec *> m_plugins; //集合持有的插件描述文件表

};

} // namespace ExtensionSystem

#endif // PLUGINCOLLECTION_H
