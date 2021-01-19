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

#ifndef FANCYACTIONBAR_H
#define FANCYACTIONBAR_H

#include <QToolButton>

QT_BEGIN_NAMESPACE
class QVBoxLayout;
QT_END_NAMESPACE

namespace Core {
namespace Internal {
    /*工具按钮（QToolButton）有两部分组成：文本text 和 图标icon  注:普通按钮不能带图标*/
    /*自定义工具按钮*/
class FancyToolButton : public QToolButton
{
    Q_OBJECT

    Q_PROPERTY(float fader READ fader WRITE setFader)

public:

    //构造 设置一些基础属性
    FancyToolButton(QWidget *parent = 0);

    //重绘
    void paintEvent(QPaintEvent *event);

    //事件过滤 重写了鼠标移入 移出 提示
    bool event(QEvent *e);

    //自定义必须实现: 尺寸
    QSize sizeHint() const;

    //自定义必须实现: 最小尺寸
    QSize minimumSizeHint() const;

    float m_fader; //色彩渐变率
    float fader() { return m_fader; }
    void setFader(float value) { m_fader = value; update(); }

private slots:

//默认行为改变触发
    void actionChanged();
};

//存放 FancyToolButton 的 buttonbar
class FancyActionBar : public QWidget
{
    Q_OBJECT

public:

    //设置对象名 构建布局
    FancyActionBar(QWidget *parent = 0);

    //重绘
    void paintEvent(QPaintEvent *event);

    //插入 操作项 到指定位置 1.把 QAction用自定义控件封装 2.绑定事件 3.插入布局容器
    void insertAction(int index, QAction *action);

    //在0位置加入操作项 实现同 insertAction
    void addProjectSelector(QAction *action);

    //返回 m_actionsLayout 
    QLayout *actionsLayout() const;

    //自定义控件需要实现  最小尺寸
    QSize minimumSizeHint() const;

private:
    QVBoxLayout *m_actionsLayout; //存放FancyToolButton的布局
};

} // namespace Internal
} // namespace Core

#endif // FANCYACTIONBAR_H
