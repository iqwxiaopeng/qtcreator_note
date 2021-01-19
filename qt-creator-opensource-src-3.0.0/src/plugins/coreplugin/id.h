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

#ifndef CORE_ID_H
#define CORE_ID_H

#include "core_global.h"

#include <QMetaType>
#include <QString>

QT_BEGIN_NAMESPACE
class QDataStream;
class QVariant;
QT_END_NAMESPACE

namespace Core {
	//ID���װ��һ����ʶ�����ñ�ʶ�����ض����е�QC��������Ψһ�ġ�
	//ID������ʶ�����Ȥ�Ķ���Ĺ��ߣ������Ͱ�ȫ�Ժ��ٶȱ���ͨ��qstring��qbytearray�ṩ��Ҫ�졣
	//ID���ڲ���ʾΪ32λ��������uid��������������ʾ�ͳ־��Ե�7λ��ASCII�����������
	//��Ϊ qc ��һ���ַַ���ÿ���������10000��uid��˽�з�Χ����Щuid��֤��Ψһ�ġ�
class CORE_EXPORT Id
{
public:
	//ÿ�����10000uid��Χ ����ǰ1000�������ID��,Ҳ���ǲ����ID�Ŵ� 10000*1000��ʼ
    enum { IdsPerPlugin = 10000, ReservedPlugins = 1000 };

    Id() : m_id(0) {}
    Id(int uid) : m_id(uid) {}
    Id(const char *name);

	//����ǰidָ�����ַ�����Ӻ�׺�����������µ�id
    Id withSuffix(int suffix) const;
    Id withSuffix(const char *suffix) const;
    Id withSuffix(const QString &suffix) const;

    //����ǰidָ�����ַ�����Ӻ�׺�����������µ�id
    Id withPrefix(const char *prefix) const;

	//��ȡid������ַ���,��id->����ȡ
    QByteArray name() const;
    QString toString() const; // Avoid.
    QVariant toSetting() const; // Good to use.

	//��idָ���ַ�������baseid��ָ���ĺ�,���¼�����id eg: aab  aa = b������id
    QString suffixAfter(Id baseId) const;

	//�Ƿ����
    bool isValid() const { return m_id; }

	//����������
    bool operator==(Id id) const { return m_id == id.m_id; }
    bool operator==(const char *name) const;
    bool operator!=(Id id) const { return m_id != id.m_id; }
    bool operator!=(const char *name) const { return !operator==(name); }
    bool operator<(Id id) const { return m_id < id.m_id; }
    bool operator>(Id id) const { return m_id > id.m_id; }


    bool alphabeticallyBefore(Id other) const;

	//��ȡid
    int uniqueIdentifier() const { return m_id; }

	//������id
    static Id fromUniqueIdentifier(int uid) { return Id(uid); }

	//���ַ���ת��Ϊid
    static Id fromString(const QString &str); // FIXME: avoid.
    static Id fromName(const QByteArray &ba); // FIXME: avoid.
    static Id fromSetting(const QVariant &variant); // Good to use.

	//��id����ע��id,Ҳ����id->�ַ���ӳ��� �� �ַ���->idӳ���
    static void registerId(int uid, const char *name);

private:
    // Intentionally unimplemented
    Id(const QLatin1String &);
    int m_id; //��ϣֵ
};

//�����ϣ ��Ϊ��theid���Ѿ������,���ֱ�ӷ���
inline uint qHash(const Id &id) { return id.uniqueIdentifier(); }

} // namespace Core

Q_DECLARE_METATYPE(Core::Id)
Q_DECLARE_METATYPE(QList<Core::Id>)

QT_BEGIN_NAMESPACE
QDataStream &operator<<(QDataStream &ds, const Core::Id &id);
QDataStream &operator>>(QDataStream &ds, Core::Id &id);
QT_END_NAMESPACE

#endif // CORE_ID_H
