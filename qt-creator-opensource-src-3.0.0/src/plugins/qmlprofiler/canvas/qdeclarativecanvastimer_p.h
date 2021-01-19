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

#ifndef QDECLARATIVECANVASTIMER_P_H
#define QDECLARATIVECANVASTIMER_P_H

#include <QJSValue>
#include <qtimer.h>
#include <qlist.h>

QT_BEGIN_NAMESPACE

class CanvasTimer : public QTimer
{
    Q_OBJECT

public:
    CanvasTimer(QObject *parent, const QJSValue &data);

public Q_SLOTS:
    void handleTimeout();
    bool equals(const QJSValue &value){return m_value.equals(value);}

public:
    static void createTimer(QObject *parent, const QJSValue &val, long timeout, bool singleshot);
    static void removeTimer(CanvasTimer *timer);
    static void removeTimer(const QJSValue &);

private:
    QJSValue m_value;

};

QT_END_NAMESPACE

#endif // QDECLARATIVECANVASTIMER_P_H
