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

#ifndef QMLPROFILERSIMPLEMODEL_H
#define QMLPROFILERSIMPLEMODEL_H

#include "qmlprofiler_global.h"

#include <qmldebug/qmlprofilereventlocation.h>

#include <QObject>
#include <QVector>
#include <QStringList>

namespace QmlProfiler {

class QmlProfilerModelManager;

// stores the data from the client as-is
class QMLPROFILER_EXPORT QmlProfilerSimpleModel : public QObject
{
    Q_OBJECT
public:
    struct QmlEventData {
        QString displayName;
        int eventType;
        int bindingType;
        qint64 startTime;
        qint64 duration;
        QStringList data;
        QmlDebug::QmlEventLocation location;
        qint64 numericData1;
        qint64 numericData2;
        qint64 numericData3;
        qint64 numericData4;
        qint64 numericData5;
    };

    explicit QmlProfilerSimpleModel(QObject *parent = 0);
    ~QmlProfilerSimpleModel();

    virtual void clear();
    bool isEmpty() const;
    const QVector<QmlEventData> &getEvents() const;
    int count() const;
    void addQmlEvent(int type, int bindingType, qint64 startTime, qint64 duration, const QStringList &data, const QmlDebug::QmlEventLocation &location,
                     qint64 ndata1, qint64 ndata2, qint64 ndata3, qint64 ndata4, qint64 ndata5);
    qint64 lastTimeMark() const;
    virtual void complete();

    static QString getHashString(const QmlProfilerSimpleModel::QmlEventData &event);

signals:
    void changed();

protected:
    QVector<QmlEventData> eventList;
    QmlProfilerModelManager *m_modelManager;
    int m_modelId;
};

}

#endif
