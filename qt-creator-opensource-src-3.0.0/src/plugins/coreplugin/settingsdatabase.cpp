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

#include "settingsdatabase.h"

#include <QDir>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>

/*!
    \class Core::SettingsDatabase
    \brief The SettingsDatabase class offers an alternative to the
    application-wide QSettings that is more
    suitable for storing large amounts of data.

    The settings database is SQLite based, and lazily retrieves data when it
    is asked for. It also does incremental updates of the database rather than
    rewriting the whole file each time one of the settings change.

    The SettingsDatabase API mimics that of QSettings.
*/

using namespace Core;
using namespace Core::Internal;

enum { debug_settings = 0 };

namespace Core {
namespace Internal {

typedef QMap<QString, QVariant> SettingsMap; //key--value 表与数据库对应

class SettingsDatabasePrivate
{
public:
	//拼接所有的key前缀 g1 = UT  g2=PC 则key= UT/PC
    QString effectiveGroup() const
    {
		//将所有字符串列表的字符串联接到单个字符串中，每个元素由给定的分隔符分隔（可以是空字符串）。
		//也就是把 m_groups中所存的字符串连接起来用/间隔输出  eg: [1] a  [2] b --> a/b
        return m_groups.join(QString(QLatin1Char('/'))); 
    }

	//获取拼接前缀后得到的新key值 g1 = UT  g2=PC,key=101 则key= UT/PC/101,因为可以多个前缀
    QString effectiveKey(const QString &key) const
    {
        QString g = effectiveGroup();
        if (!g.isEmpty() && !key.isEmpty()) //末尾继续追加一个 /  
            g += QLatin1Char('/');
        g += key;
        return g;
    }

	//本cache的key是加了前缀的
    SettingsMap m_settings; //key--value 表与数据库对应,数据库的key值 cache

    QStringList m_groups; //key前缀表 例如 g1 = UT  g2=PC,key=101 则key= UT/PC/101,因为可以增加多个前缀
    QStringList m_dirtyKeys; //暂时没有被使用 脏数据就是缓冲,db不一致数据

    QSqlDatabase m_db; //sqlite数据库
};

} // namespace Internal
} // namespace Core

/*
在目录 path下创建名为application的sqlite数据库文件,并创建表 settings,如果表存在会查询表中所有的key,插入到m_settings映射表
*/
SettingsDatabase::SettingsDatabase(const QString &path,
                                   const QString &application,
                                   QObject *parent)
    : QObject(parent)
    , d(new SettingsDatabasePrivate) //构建数据类
{
    const QLatin1Char slash('/'); //分隔符

    // TODO: Don't rely on a path, but determine automatically 
	//如果路径不存在 则创建路径
    QDir pathDir(path);
    if (!pathDir.exists())
        pathDir.mkpath(pathDir.absolutePath());

	//创建sqlite数据库文件 数据库路径为path,数据库名字为 application
    QString fileName = path;
    if (!fileName.endsWith(slash))
        fileName += slash;
    fileName += application;
    fileName += QLatin1String(".db");

	//加载数据库链接  链接名为 settings
    d->m_db = QSqlDatabase::addDatabase(QLatin1String("QSQLITE"), QLatin1String("settings"));
    d->m_db.setDatabaseName(fileName); //设置sqlite数据库文件
    if (!d->m_db.open()) {
        qWarning().nospace() << "Warning: Failed to open settings database at " << fileName << " ("
                             << d->m_db.lastError().driverText() << ")";
    } else {
        // Create the settings table if it doesn't exist yet
		//如果是新创建的数据库 或者表不存在则创建，已经有了，则检索其中的key并插入m_settings映射表
        QSqlQuery query(d->m_db);
        query.prepare(QLatin1String("CREATE TABLE IF NOT EXISTS settings ("
                                    "key PRIMARY KEY ON CONFLICT REPLACE, "
                                    "value)"));
        if (!query.exec())
            qWarning().nospace() << "Warning: Failed to prepare settings database! ("
                                 << query.lastError().driverText() << ")";

        // Retrieve all available keys (values are retrieved lazily)
        if (query.exec(QLatin1String("SELECT key FROM settings"))) {
            while (query.next()) {
                d->m_settings.insert(query.value(0).toString(), QVariant());
            }
        }
    }
}

SettingsDatabase::~SettingsDatabase()
{
    sync();

    delete d;
    QSqlDatabase::removeDatabase(QLatin1String("settings"));
}

//设置key  value值 1.根据key所在的group得到新的key,存入k,v 到cache,存入k,v到db
void SettingsDatabase::setValue(const QString &key, const QVariant &value)
{
    const QString effectiveKey = d->effectiveKey(key);

    // Add to cache
    d->m_settings.insert(effectiveKey, value);

    if (!d->m_db.isOpen())
        return;

    // Instant apply (TODO: Delay writing out settings)
    QSqlQuery query(d->m_db);
    query.prepare(QLatin1String("INSERT INTO settings VALUES (?, ?)"));
    query.addBindValue(effectiveKey);
    query.addBindValue(value);
    query.exec();

    if (debug_settings)
        qDebug() << "Stored:" << effectiveKey << "=" << value;
}

//获取key对应的value值 1.根据key所在的group得到新的key  2.如果cache命中则返回 3.cache没命中,从数据库查,并插入结果到cache
QVariant SettingsDatabase::value(const QString &key, const QVariant &defaultValue) const
{
    const QString effectiveKey = d->effectiveKey(key);
    QVariant value = defaultValue;

    SettingsMap::const_iterator i = d->m_settings.constFind(effectiveKey);
    if (i != d->m_settings.constEnd() && i.value().isValid()) {
        value = i.value();
    } else if (d->m_db.isOpen()) {
        // Try to read the value from the database
        QSqlQuery query(d->m_db);
        query.prepare(QLatin1String("SELECT value FROM settings WHERE key = ?"));
        query.addBindValue(effectiveKey);
        query.exec();
        if (query.next()) {
            value = query.value(0);

            if (debug_settings)
                qDebug() << "Retrieved:" << effectiveKey << "=" << value;
        }

        // Cache the result
        d->m_settings.insert(effectiveKey, value);
    }

    return value;
}

//判断是否存在key值元素  因为所有的key都被cache,因此cache判断是否包含
bool SettingsDatabase::contains(const QString &key) const
{
    return d->m_settings.contains(d->effectiveKey(key));
}

//移除key对应的value 1.获取key 2. 从cache移除 3.从db移除
void SettingsDatabase::remove(const QString &key)
{
    const QString effectiveKey = d->effectiveKey(key); //cache的key是加了前缀的

    // Remove keys from the cache
    foreach (const QString &k, d->m_settings.keys()) {
        // Either it's an exact match, or it matches up to a /
        if (k.startsWith(effectiveKey)
            && (k.length() == effectiveKey.length()
                || k.at(effectiveKey.length()) == QLatin1Char('/')))
        {
            d->m_settings.remove(k);
        }
    }

    if (!d->m_db.isOpen())
        return;

    // Delete keys from the database
    QSqlQuery query(d->m_db);
    query.prepare(QLatin1String("DELETE FROM settings WHERE key = ? OR key LIKE ?"));
    query.addBindValue(effectiveKey);
    query.addBindValue(QString(effectiveKey + QLatin1String("/%")));
    query.exec();
}

//增加key前缀 因为key = groups/key;例如 g1 = UT  g2=PC,key=101 则key= UT/PC/101,因为可以增加多个前缀
void SettingsDatabase::beginGroup(const QString &prefix)
{
    d->m_groups.append(prefix);
}
//移除最末尾的key前缀
void SettingsDatabase::endGroup()
{
    d->m_groups.removeLast();
}

//获取key前缀
QString SettingsDatabase::group() const
{
    return d->effectiveGroup();
}

//获取所有前缀=group()的key值列表 类似于ini同一个section下的元素
QStringList SettingsDatabase::childKeys() const
{
    QStringList children;

    const QString g = group(); //取前缀
    QMapIterator<QString, QVariant> i(d->m_settings);
    while (i.hasNext()) {
        const QString &key = i.next().key();
        if (key.startsWith(g) && key.indexOf(QLatin1Char('/'), g.length() + 1) == -1)
            children.append(key.mid(g.length() + 1));
    }

    return children;
}

void SettingsDatabase::sync()
{
    // TODO: Delay writing of dirty keys and save them here
}
