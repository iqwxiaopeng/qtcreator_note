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

//settingsDatabase���ṩ��Ӧ�÷�Χ�����qsettings�����ڴ洢�������ݡ�
//�������ݿ��ǻ���sqlite�ģ�������Ҫ��
//����ִ�����ݿ���������£�������ÿ�θ�������һ������ʱ��д�����ļ����������ݿ�API��qsettings���ơ�
class CORE_EXPORT SettingsDatabase : public QObject
{
public:
	//��Ŀ¼ path�´�����Ϊapplication��sqlite���ݿ��ļ�,�������� settings,�������ڻ��ѯ�������е�key,���뵽m_settingsӳ���
    SettingsDatabase(const QString &path, const QString &application, QObject *parent = 0);
    ~SettingsDatabase();

	//����key  valueֵ 1.����key���ڵ�group�õ��µ�key,����k,v ��cache,����k,v��db
    void setValue(const QString &key, const QVariant &value);

	//��ȡkey��Ӧ��valueֵ 1.����key���ڵ�group�õ��µ�key  2.���cache�����򷵻� 3.cacheû����,�����ݿ��,����������cache
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;

	//�ж��Ƿ����keyֵԪ��  ��Ϊ���е�key����cache,���cache�ж��Ƿ����
    bool contains(const QString &key) const;

	//�Ƴ�key��Ӧ��value 1.��ȡkey 2. ��cache�Ƴ� 3.��db�Ƴ�
    void remove(const QString &key);

	//����keyǰ׺ ��Ϊkey = groups/key;���� g1 = UT  g2=PC,key=101 ��key= UT/PC/101,��Ϊ�������Ӷ��ǰ׺
    void beginGroup(const QString &prefix);

	//�Ƴ���ĩβ��keyǰ׺
    void endGroup();

	//��ȡkeyǰ׺
    QString group() const;

	//��ȡ����ǰ׺group()��keyֵ�б� ������iniͬһ��section�µ�Ԫ��
    QStringList childKeys() const;

	//û��ʵ�ֵ�Ԥ��ͬ��cache��db
    void sync();

private:

	//������  �������ݿ� �Լ� cache
    Internal::SettingsDatabasePrivate *d;
};

} // namespace Core

#endif // SETTINGSDATABASE_H
