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
	/*������Ǵ��Ķ���Context�������,��IContext�Ƕ��ڱ�����QT�ؼ��󶨵ķ�װ��*/
class CORE_EXPORT Context
{
public:
    Context() {}

	//���� 
    explicit Context(Id c1) { add(c1); }
    Context(Id c1, Id c2) { add(c1); add(c2); }
    Context(Id c1, Id c2, Id c3) { add(c1); add(c2); add(c3); }

	//�жϰ���
    bool contains(Id c) const { return d.contains(c); }

	//ȡ��С
    int size() const { return d.size(); }
	//�п�
    bool isEmpty() const { return d.isEmpty(); }

	//������ȡֵ
    Id at(int i) const { return d.at(i); }

    // FIXME: Make interface slimmer. ��������ز���
    typedef QList<Id>::const_iterator const_iterator;
    const_iterator begin() const { return d.begin(); }
    const_iterator end() const { return d.end(); }

	//��ֵ������
    int indexOf(Id c) const { return d.indexOf(c); }

	//�Ƴ�
    void removeAt(int i) { d.removeAt(i); }
	//����
    void prepend(Id c) { d.prepend(c); }

	//�Ӳ��� ��Ӧ�ڲ�Ԫ�غϲ�
    void add(const Context &c) { d += c.d; }
    void add(Id c) { d.append(c); }

	//�е�
    bool operator==(const Context &c) const { return d == c.d; }

private:
    QList<Id> d; //id�б� һ��contex�����ж��Ԫ��
};

//Contex��Qt�󶨵ķ�װ��
class CORE_EXPORT IContext : public QObject
{
    Q_OBJECT
public:

	//���ҵĸ��׽ڵ�
    IContext(QObject *parent = 0) : QObject(parent) {}

	//��ȡ��Ӧ��Context
    virtual Context context() const { return m_context; }

	//��ȡ��Ӧ�Ĵ���
    virtual QWidget *widget() const { return m_widget; }

	//��ȡ������ʾ
    virtual QString contextHelpId() const { return m_contextHelpId; }

	//����Context
    virtual void setContext(const Context &context) { m_context = context; }

	//���ô���
    virtual void setWidget(QWidget *widget) { m_widget = widget; }

	//���ð�����Ϣ
    virtual void setContextHelpId(const QString &id) { m_contextHelpId = id; }

protected:
    Context m_context; //���󶨵�Context
    QPointer<QWidget> m_widget; //����Ӧ�Ĵ���
    QString m_contextHelpId; //������Ϣ
};

} // namespace Core

#endif //ICONTEXT_H
