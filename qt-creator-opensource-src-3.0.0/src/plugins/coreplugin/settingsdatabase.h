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

#ifndef SETTINGSDATABASE_H
#define SETTINGSDATABASE_H

#include "core_global.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>

namespace Core {

namespace Internal {
class SettingsDatabasePrivate;
}

//settingsDatabase类提供了应用范围更广的qsettings适用于存储大量数据。
//设置数据库是基于sqlite的，当它被要求。
//它还执行数据库的增量更新，而不是每次更改其中一个设置时重写整个文件。设置数据库API与qsettings类似。
class CORE_EXPORT SettingsDatabase : public QObject
{
public:
	//在目录 path下创建名为application的sqlite数据库文件,并创建表 settings,如果表存在会查询表中所有的key,插入到m_settings映射表
    SettingsDatabase(const QString &path, const QString &application, QObject *parent = 0);
    ~SettingsDatabase();

	//设置key  value值 1.根据key所在的group得到新的key,存入k,v 到cache,存入k,v到db
    void setValue(const QString &key, const QVariant &value);

	//获取key对应的value值 1.根据key所在的group得到新的key  2.如果cache命中则返回 3.cache没命中,从数据库查,并插入结果到cache
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;

	//判断是否存在key值元素  因为所有的key都被cache,因此cache判断是否包含
    bool contains(const QString &key) const;

	//移除key对应的value 1.获取key 2. 从cache移除 3.从db移除
    void remove(const QString &key);

	//增加key前缀 因为key = groups/key;例如 g1 = UT  g2=PC,key=101 则key= UT/PC/101,因为可以增加多个前缀
    void beginGroup(const QString &prefix);

	//移除最末尾的key前缀
    void endGroup();

	//获取key前缀
    QString group() const;

	//获取所有前缀group()的key值列表 类似于ini同一个section下的元素
    QStringList childKeys() const;

	//没有实现的预留同步cache与db
    void sync();

private:

	//数据类  持有数据库 以及 cache
    Internal::SettingsDatabasePrivate *d;
};

} // namespace Core

#endif // SETTINGSDATABASE_H
