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
    /*���߰�ť��QToolButton������������ɣ��ı�text �� ͼ��icon  ע:��ͨ��ť���ܴ�ͼ��*/
    /*�Զ��幤�߰�ť*/
class FancyToolButton : public QToolButton
{
    Q_OBJECT

    Q_PROPERTY(float fader READ fader WRITE setFader)

public:

    //���� ����һЩ��������
    FancyToolButton(QWidget *parent = 0);

    //�ػ�
    void paintEvent(QPaintEvent *event);

    //�¼����� ��д��������� �Ƴ� ��ʾ
    bool event(QEvent *e);

    //�Զ������ʵ��: �ߴ�
    QSize sizeHint() const;

    //�Զ������ʵ��: ��С�ߴ�
    QSize minimumSizeHint() const;

    float m_fader; //ɫ�ʽ�����
    float fader() { return m_fader; }
    void setFader(float value) { m_fader = value; update(); }

private slots:

//Ĭ����Ϊ�ı䴥��
    void actionChanged();
};

//��� FancyToolButton �� buttonbar
class FancyActionBar : public QWidget
{
    Q_OBJECT

public:

    //���ö����� ��������
    FancyActionBar(QWidget *parent = 0);

    //�ػ�
    void paintEvent(QPaintEvent *event);

    //���� ������ ��ָ��λ�� 1.�� QAction���Զ���ؼ���װ 2.���¼� 3.���벼������
    void insertAction(int index, QAction *action);

    //��0λ�ü�������� ʵ��ͬ insertAction
    void addProjectSelector(QAction *action);

    //���� m_actionsLayout 
    QLayout *actionsLayout() const;

    //�Զ���ؼ���Ҫʵ��  ��С�ߴ�
    QSize minimumSizeHint() const;

private:
    QVBoxLayout *m_actionsLayout; //���FancyToolButton�Ĳ���
};

} // namespace Internal
} // namespace Core

#endif // FANCYACTIONBAR_H
