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

#ifndef EXTENSIONSYSTEM_INVOKER_H
#define EXTENSIONSYSTEM_INVOKER_H

#include "extensionsystem_global.h"

#include <QMetaMethod>
#include <QMetaObject>
#include <QMetaType>
#include <QVarLengthArray>

namespace ExtensionSystem {

//封装反射类
class EXTENSIONSYSTEM_EXPORT InvokerBase
{
public:
    InvokerBase();
    ~InvokerBase();

	//反射调用是否成功
    bool wasSuccessful() const;

	//设置调用类型(跨线程 还是 不跨
    void setConnectionType(Qt::ConnectionType connectionType);

	//增加一个参数
    template <class T> void addArgument(const T &t)
    {
        arg[lastArg++] = QGenericArgument(typeName<T>(), &t);
    }

	//设置接收返回的数据对象
    template <class T> void setReturnValue(T &t)
    {
        useRet = true;
        ret = QGenericReturnArgument(typeName<T>(), &t);
    }

	//调用 target对象的名字叫slot的方法
    void invoke(QObject *target, const char *slot);

private:
    InvokerBase(const InvokerBase &); // Unimplemented.
    template <class T> const char *typeName()
    {
        return QMetaType::typeName(qMetaTypeId<T>());
    }
    QObject *target; //反射调用作用的对象
    QGenericArgument arg[10]; //参数数组 最多可以有十个参数
    QGenericReturnArgument ret; //接收调用反回值
    QVarLengthArray<char, 512> sig; //反射协议序列化，qt内部实现要求
    int lastArg;//参数个数 
    bool success; //调用是否成功
    bool useRet; //是否有返回值
    Qt::ConnectionType connectionType; //调用类型 同步或者异步(如果是跨线程则需要扔到队列 否则 直接观察者就行)
    mutable bool nag; //反射对象是否已经使用过(每个对象只能用一次)
};

//更详细封装  0参数0返回  1参数带返回，2参数带返回 方法封装类
template <class Result>
class Invoker : public InvokerBase
{
public:
    Invoker(QObject *target, const char *slot)
    {
        InvokerBase::invoke(target, slot);
    }

    template <class T0>
    Invoker(QObject *target, const char *slot, const T0 &t0)
    {
        setReturnValue(result);
        addArgument(t0);
        InvokerBase::invoke(target, slot);
    }

    template <class T0, class T1>
    Invoker(QObject *target, const char *slot, const T0 &t0, const T1 &t1)
    {
        setReturnValue(result);
        addArgument(t0);
        addArgument(t1);
        InvokerBase::invoke(target, slot);
    }

    template <class T0, class T1, class T2>
    Invoker(QObject *target, const char *slot, const T0 &t0,
        const T1 &t1, const T2 &t2)
    {
        setReturnValue(result);
        addArgument(t0);
        addArgument(t1);
        addArgument(t2);
        InvokerBase::invoke(target, slot);
    }

    operator Result() const { return result; }

private:
    Result result;
};

//针对返回值为void的反射对象封装
template<> class Invoker<void> : public InvokerBase
{
public:
    Invoker(QObject *target, const char *slot)
    {
        InvokerBase::invoke(target, slot);
    }

    template <class T0>
    Invoker(QObject *target, const char *slot, const T0 &t0)
    {
        addArgument(t0);
        InvokerBase::invoke(target, slot);
    }

    template <class T0, class T1>
    Invoker(QObject *target, const char *slot, const T0 &t0, const T1 &t1)
    {
        addArgument(t0);
        addArgument(t1);
        InvokerBase::invoke(target, slot);
    }

    template <class T0, class T1, class T2>
    Invoker(QObject *target, const char *slot, const T0 &t0,
        const T1 &t1, const T2 &t2)
    {
        addArgument(t0);
        addArgument(t1);
        addArgument(t2);
        InvokerBase::invoke(target, slot);
    }
};

//无参数带返回
template <class Result>
Result invokeHelper(InvokerBase &in, QObject *target, const char *slot)
{
    Result result;
    in.setReturnValue(result);
    in.invoke(target, slot);
    return result;
}

//无参数无返回
template <>
inline void invokeHelper<void>(InvokerBase &in, QObject *target, const char *slot)
{
    in.invoke(target, slot);
}
//封装了反射体到方法
template<class Result>
Result invoke(QObject *target, const char *slot)
{
    InvokerBase in;
    return invokeHelper<Result>(in, target, slot);
}

template<class Result, class T0>
Result invoke(QObject *target, const char *slot, const T0 &t0)
{
    InvokerBase in;
    in.addArgument(t0);
    return invokeHelper<Result>(in, target, slot);
}

template<class Result, class T0, class T1>
Result invoke(QObject *target, const char *slot, const T0 &t0, const T1 &t1)
{
    InvokerBase in;
    in.addArgument(t0);
    in.addArgument(t1);
    return invokeHelper<Result>(in, target, slot);
}

template<class Result, class T0, class T1, class T2>
Result invoke(QObject *target, const char *slot,
    const T0 &t0, const T1 &t1, const T2 &t2)
{
    InvokerBase in;
    in.addArgument(t0);
    in.addArgument(t1);
    in.addArgument(t2);
    return invokeHelper<Result>(in, target, slot);
}

} // namespace ExtensionSystem

#endif // EXTENSIONSYSTEM_INVOKER_H
