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

#include "removepropertiescommand.h"

#include <QDebug>

namespace QmlDesigner {

RemovePropertiesCommand::RemovePropertiesCommand()
{
}

RemovePropertiesCommand::RemovePropertiesCommand(const QVector<PropertyAbstractContainer> &properties)
    : m_properties(properties)
{
}

QVector<PropertyAbstractContainer> RemovePropertiesCommand::properties() const
{
    return m_properties;
}

QDataStream &operator<<(QDataStream &out, const RemovePropertiesCommand &command)
{
    out << command.properties();

    return out;
}

QDataStream &operator>>(QDataStream &in, RemovePropertiesCommand &command)
{
    in >> command.m_properties;

    return in;
}

QDebug operator <<(QDebug debug, const RemovePropertiesCommand &command)
{
    return debug.nospace() << "RemovePropertiesCommand(properties: " << command.m_properties << ")";
}

}
