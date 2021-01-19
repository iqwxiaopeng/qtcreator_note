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
	//ID类封装了一个标识符，该标识符在特定运行的QC进程中是唯一的。
	//ID被用作识别感兴趣的对象的工具，其类型安全性和速度比普通的qstring或qbytearray提供的要快。
	//ID在内部表示为32位整数（其uid），并与用于显示和持久性的7位纯ASCII名称相关联。
	//作为 qc 的一部分分发的每个插件都有10000个uid的私有范围，这些uid保证是唯一的。
class CORE_EXPORT Id
{
public:
	//每个插件10000uid范围 保留前1000个插件的ID号,也就是插件的ID号从 10000*1000开始
    enum { IdsPerPlugin = 10000, ReservedPlugins = 1000 };

    Id() : m_id(0) {}
    Id(int uid) : m_id(uid) {}
    Id(const char *name);

	//将当前id指代的字符串添加后缀后重新生成新的id
    Id withSuffix(int suffix) const;
    Id withSuffix(const char *suffix) const;
    Id withSuffix(const QString &suffix) const;

    //将当前id指代的字符串添加后缀后重新生成新的id
    Id withPrefix(const char *prefix) const;

	//获取id代表的字符串,从id->串表取
    QByteArray name() const;
    QString toString() const; // Avoid.
    QVariant toSetting() const; // Good to use.

	//本id指代字符串跳过baseid所指代的后,重新计算新id eg: aab  aa = b计算新id
    QString suffixAfter(Id baseId) const;

	//是否可用
    bool isValid() const { return m_id; }

	//定义计算操作
    bool operator==(Id id) const { return m_id == id.m_id; }
    bool operator==(const char *name) const;
    bool operator!=(Id id) const { return m_id != id.m_id; }
    bool operator!=(const char *name) const { return !operator==(name); }
    bool operator<(Id id) const { return m_id < id.m_id; }
    bool operator>(Id id) const { return m_id > id.m_id; }


    bool alphabeticallyBefore(Id other) const;

	//获取id
    int uniqueIdentifier() const { return m_id; }

	//构建新id
    static Id fromUniqueIdentifier(int uid) { return Id(uid); }

	//将字符串转换为id
    static Id fromString(const QString &str); // FIXME: avoid.
    static Id fromName(const QByteArray &ba); // FIXME: avoid.
    static Id fromSetting(const QVariant &variant); // Good to use.

	//像id池子注册id,也就是id->字符串映射表 和 字符串->id映射表
    static void registerId(int uid, const char *name);

private:
    // Intentionally unimplemented
    Id(const QLatin1String &);
    int m_id; //哈希值
};

//计算哈希 因为在theid中已经算过了,因此直接返回
inline uint qHash(const Id &id) { return id.uniqueIdentifier(); }

} // namespace Core

Q_DECLARE_METATYPE(Core::Id)
Q_DECLARE_METATYPE(QList<Core::Id>)

QT_BEGIN_NAMESPACE
QDataStream &operator<<(QDataStream &ds, const Core::Id &id);
QDataStream &operator>>(QDataStream &ds, Core::Id &id);
QT_END_NAMESPACE

#endif // CORE_ID_H
