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

#ifndef ICONTEXT_H
#define ICONTEXT_H

#include <coreplugin/core_global.h>
#include <coreplugin/id.h>

#include <QList>
#include <QObject>
#include <QPointer>
#include <QWidget>

namespace Core {
	/*这个类是纯的对于Context的设计类,而IContext是对于本类与QT控件绑定的封装类*/
class CORE_EXPORT Context
{
public:
    Context() {}

	//构造 
    explicit Context(Id c1) { add(c1); }
    Context(Id c1, Id c2) { add(c1); add(c2); }
    Context(Id c1, Id c2, Id c3) { add(c1); add(c2); add(c3); }

	//判断包含
    bool contains(Id c) const { return d.contains(c); }

	//取大小
    int size() const { return d.size(); }
	//判空
    bool isEmpty() const { return d.isEmpty(); }

	//按索引取值
    Id at(int i) const { return d.at(i); }

    // FIXME: Make interface slimmer. 迭代器相关操作
    typedef QList<Id>::const_iterator const_iterator;
    const_iterator begin() const { return d.begin(); }
    const_iterator end() const { return d.end(); }

	//按值求索引
    int indexOf(Id c) const { return d.indexOf(c); }

	//移除
    void removeAt(int i) { d.removeAt(i); }
	//增加
    void prepend(Id c) { d.prepend(c); }

	//加操作 对应内部元素合并
    void add(const Context &c) { d += c.d; }
    void add(Id c) { d.append(c); }

	//判等
    bool operator==(const Context &c) const { return d == c.d; }

private:
    QList<Id> d; //id列表 一个contex可以有多个元素
};

//Contex与Qt绑定的封装类
class CORE_EXPORT IContext : public QObject
{
    Q_OBJECT
public:

	//所挂的父亲节点
    IContext(QObject *parent = 0) : QObject(parent) {}

	//获取对应的Context
    virtual Context context() const { return m_context; }

	//获取对应的窗体
    virtual QWidget *widget() const { return m_widget; }

	//获取帮助提示
    virtual QString contextHelpId() const { return m_contextHelpId; }

	//设置Context
    virtual void setContext(const Context &context) { m_context = context; }

	//设置窗体
    virtual void setWidget(QWidget *widget) { m_widget = widget; }

	//设置帮助消息
    virtual void setContextHelpId(const QString &id) { m_contextHelpId = id; }

protected:
    Context m_context; //所绑定的Context
    QPointer<QWidget> m_widget; //所对应的窗体
    QString m_contextHelpId; //帮助信息
};

} // namespace Core

#endif //ICONTEXT_H
