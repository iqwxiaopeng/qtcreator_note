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

typedef QMap<QString, QVariant> SettingsMap; //key--value �������ݿ��Ӧ

class SettingsDatabasePrivate
{
public:
	//ƴ�����е�keyǰ׺ g1 = UT  g2=PC ��key= UT/PC
    QString effectiveGroup() const
    {
		//�������ַ����б���ַ������ӵ������ַ����У�ÿ��Ԫ���ɸ����ķָ����ָ��������ǿ��ַ�������
		//Ҳ���ǰ� m_groups��������ַ�������������/������  eg: [1] a  [2] b --> a/b
        return m_groups.join(QString(QLatin1Char('/'))); 
    }

	//��ȡƴ��ǰ׺��õ�����keyֵ g1 = UT  g2=PC,key=101 ��key= UT/PC/101,��Ϊ���Զ��ǰ׺
    QString effectiveKey(const QString &key) const
    {
        QString g = effectiveGroup();
        if (!g.isEmpty() && !key.isEmpty()) //ĩβ����׷��һ�� /  
            g += QLatin1Char('/');
        g += key;
        return g;
    }

	//��cache��key�Ǽ���ǰ׺��
    SettingsMap m_settings; //key--value �������ݿ��Ӧ,���ݿ��keyֵ cache

    QStringList m_groups; //keyǰ׺�� ���� g1 = UT  g2=PC,key=101 ��key= UT/PC/101,��Ϊ�������Ӷ��ǰ׺
    QStringList m_dirtyKeys; //��ʱû�б�ʹ�� �����ݾ��ǻ���,db��һ������

    QSqlDatabase m_db; //sqlite���ݿ�
};

} // namespace Internal
} // namespace Core

/*
��Ŀ¼ path�´�����Ϊapplication��sqlite���ݿ��ļ�,�������� settings,�������ڻ��ѯ�������е�key,���뵽m_settingsӳ���
*/
SettingsDatabase::SettingsDatabase(const QString &path,
                                   const QString &application,
                                   QObject *parent)
    : QObject(parent)
    , d(new SettingsDatabasePrivate) //����������
{
    const QLatin1Char slash('/'); //�ָ���

    // TODO: Don't rely on a path, but determine automatically 
	//���·�������� �򴴽�·��
    QDir pathDir(path);
    if (!pathDir.exists())
        pathDir.mkpath(pathDir.absolutePath());

	//����sqlite���ݿ��ļ� ���ݿ�·��Ϊpath,���ݿ�����Ϊ application
    QString fileName = path;
    if (!fileName.endsWith(slash))
        fileName += slash;
    fileName += application;
    fileName += QLatin1String(".db");

	//�������ݿ�����  ������Ϊ settings
    d->m_db = QSqlDatabase::addDatabase(QLatin1String("QSQLITE"), QLatin1String("settings"));
    d->m_db.setDatabaseName(fileName); //����sqlite���ݿ��ļ�
    if (!d->m_db.open()) {
        qWarning().nospace() << "Warning: Failed to open settings database at " << fileName << " ("
                             << d->m_db.lastError().driverText() << ")";
    } else {
        // Create the settings table if it doesn't exist yet
		//������´��������ݿ� ���߱������򴴽����Ѿ����ˣ���������е�key������m_settingsӳ���
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

//����key  valueֵ 1.����key���ڵ�group�õ��µ�key,����k,v ��cache,����k,v��db
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

//��ȡkey��Ӧ��valueֵ 1.����key���ڵ�group�õ��µ�key  2.���cache�����򷵻� 3.cacheû����,�����ݿ��,����������cache
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

//�ж��Ƿ����keyֵԪ��  ��Ϊ���е�key����cache,���cache�ж��Ƿ����
bool SettingsDatabase::contains(const QString &key) const
{
    return d->m_settings.contains(d->effectiveKey(key));
}

//�Ƴ�key��Ӧ��value 1.��ȡkey 2. ��cache�Ƴ� 3.��db�Ƴ�
void SettingsDatabase::remove(const QString &key)
{
    const QString effectiveKey = d->effectiveKey(key); //cache��key�Ǽ���ǰ׺��

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

//����keyǰ׺ ��Ϊkey = groups/key;���� g1 = UT  g2=PC,key=101 ��key= UT/PC/101,��Ϊ�������Ӷ��ǰ׺
void SettingsDatabase::beginGroup(const QString &prefix)
{
    d->m_groups.append(prefix);
}
//�Ƴ���ĩβ��keyǰ׺
void SettingsDatabase::endGroup()
{
    d->m_groups.removeLast();
}

//��ȡkeyǰ׺
QString SettingsDatabase::group() const
{
    return d->effectiveGroup();
}

//��ȡ����ǰ׺=group()��keyֵ�б� ������iniͬһ��section�µ�Ԫ��
QStringList SettingsDatabase::childKeys() const
{
    QStringList children;

    const QString g = group(); //ȡǰ׺
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
