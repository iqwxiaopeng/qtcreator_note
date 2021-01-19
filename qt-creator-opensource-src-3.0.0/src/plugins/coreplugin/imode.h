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

#ifndef IMODE_H
#define IMODE_H

#include "icontext.h"
#include "id.h"

#include <QIcon>

namespace Core {
	/*模式接口定义 在Qt Creator中有欢迎模式,编辑模式（EditMode）,设计模式（DesignMode）,调试模式,帮助模式*/
	//左侧导航栏的那几个项   本类仅仅是数据类 一个Mode包含显示名称，图标，优先级，id，类型，状态（Enabled or not)
class CORE_EXPORT IMode : public IContext
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)

public:
    IMode(QObject *parent = 0);

    QString displayName() const { return m_displayName; }
    QIcon icon() const { return m_icon; }
    int priority() const { return m_priority; }
    Id id() const { return m_id; }
    bool isEnabled() const;

    void setEnabled(bool enabled);
    void setDisplayName(const QString &displayName) { m_displayName = displayName; }
    void setIcon(const QIcon &icon) { m_icon = icon; }
    void setPriority(int priority) { m_priority = priority; }
    void setId(Id id) { m_id = id; }

signals:
    void enabledStateChanged(bool enabled);

private:
    QString m_displayName; //显示名称
    QIcon m_icon; //图标
    int m_priority; //优先级
    Id m_id; //uuid
    bool m_isEnabled; //状态 启用or禁止
};

} // namespace Core

#endif // IMODE_H
