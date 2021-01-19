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
/*本类的作用就是把多个对象绑定到一起，做成一个整体，同时持有一个静态全局表，提供查询某个对象在那个整理中的功能，也就是一个对象只能在一个整体中*/

#ifndef AGGREGATE_H
#define AGGREGATE_H

#include "aggregation_global.h"

#include <QObject>
#include <QList>
#include <QHash>
#include <QReadWriteLock>
#include <QReadLocker>

//聚合命名空间包含对绑定相关组件的支持，这样，每个组件都对外公开其他组件的属性和行为。
//捆绑到聚合的组件可以相互“转换”，并具有耦合的生命周期。有关详细信息和示例，请参阅聚合文档

namespace Aggregation {

class AGGREGATION_EXPORT Aggregate : public QObject
{
    Q_OBJECT

public:
    Aggregate(QObject *parent = 0);
    virtual ~Aggregate();

    void add(QObject *component);
    void remove(QObject *component);

    template <typename T> T *component() {
        QReadLocker(&lock());
        foreach (QObject *component, m_components) {
            if (T *result = qobject_cast<T *>(component))
                return result;
        }
        return (T *)0;
    }

    template <typename T> QList<T *> components() {
        QReadLocker(&lock());
        QList<T *> results;
        foreach (QObject *component, m_components) {
            if (T *result = qobject_cast<T *>(component)) {
                results << result;
            }
        }
        return results;
    }

	//返回对象所属于的聚合对象（如果有）。否则返回0。
    static Aggregate *parentAggregate(QObject *obj);
    static QReadWriteLock &lock();

signals:
    void changed();

private slots:
    void deleteSelf(QObject *obj);

private:
    static QHash<QObject *, Aggregate *> &aggregateMap();

    QList<QObject *> m_components;
};

// get a component via global template function
template <typename T> T *query(Aggregate *obj)
{
    if (!obj)
        return (T *)0;
    return obj->template component<T>();
}

template <typename T> T *query(QObject *obj)
{
    if (!obj)
        return (T *)0;
    T *result = qobject_cast<T *>(obj);
    if (!result) {
        QReadLocker(&lock());
        Aggregate *parentAggregation = Aggregate::parentAggregate(obj);
        result = (parentAggregation ? query<T>(parentAggregation) : 0);
    }
    return result;
}

// get all components of a specific type via template function
template <typename T> QList<T *> query_all(Aggregate *obj)
{
    if (!obj)
        return QList<T *>();
    return obj->template components<T>();
}

template <typename T> QList<T *> query_all(QObject *obj)
{
    if (!obj)
        return QList<T *>();
    QReadLocker(&lock());
    Aggregate *parentAggregation = Aggregate::parentAggregate(obj);
    QList<T *> results;
    if (parentAggregation)
        results = query_all<T>(parentAggregation);
    else if (T *result = qobject_cast<T *>(obj))
        results.append(result);
    return results;
}

} // namespace Aggregation

#endif // AGGREGATE_H
