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

#ifndef PLUGINSPEC_H
#define PLUGINSPEC_H

#include "extensionsystem_global.h"

#include <QString>
#include <QList>
#include <QHash>

QT_BEGIN_NAMESPACE
class QStringList;
class QRegExp;
QT_END_NAMESPACE

namespace ExtensionSystem {

namespace Internal { //�ڲ�ʹ�õ�,�ӿڿ��������ʵ��,�������Ҳ��ò�ʹ����������ռ��������
    class PluginSpecPrivate;
    class PluginManagerPrivate;
}

class IPlugin;

//��������ļ��� ������Ŀ �ṹ����
struct EXTENSIONSYSTEM_EXPORT PluginDependency
{
    //�������Ͷ��� ���� ���� ��ѡ
    enum Type {
        Required,
        Optional
    };

    PluginDependency() : type(Required) {}

    QString name; //�������Ĳ����
    QString version; //�������Ĳ���汾
    Type type; //������ϵ�Ǳ��뻹�ǿ�ѡ
    bool operator==(const PluginDependency &other) const;
};

//���ݲ������������һ����ϣֵ
uint qHash(const ExtensionSystem::PluginDependency &value);

//��������ļ��� ��������ඨ��(������ ����ֵ ��������)
struct EXTENSIONSYSTEM_EXPORT PluginArgumentDescription
{
    QString name;
    QString parameter;
    QString description;
};

/*
���������
*/
class EXTENSIONSYSTEM_EXPORT PluginSpec
{
public:
	//������  ���ڶ��� ������ϵOK  �Ѿ�����  �Լ���ʼ�� �Լ�����  ���ر�  ��ɾ��
    enum State { Invalid, Read, Resolved, Loaded, Initialized, Running, Stopped, Deleted}; //�����״̬ö��

    ~PluginSpec();

    // information from the xml file, valid after 'Read' state is reached
    QString name() const; //�����
    QString version() const; //����汾
    QString compatVersion() const; //���ݰ汾
    QString vendor() const; //
    QString copyright() const; //��Ȩ
    QString license() const; //��ȨЭ��
    QString description() const; //���������Ϣ
    QString url() const; //���url
    QString category() const; //������
    QRegExp platformSpecification() const; //���ƽ̨������Ϣ
    bool isExperimental() const; //�Ƿ���԰�
    bool isDisabledByDefault() const; //Ĭ�Ͽ������
    bool isEnabledInSettings() const; //�����Ƿ���������
    bool isEffectivelyEnabled() const;//����Ƿ�Ҫ������ʱ�ͼ���
    bool isDisabledIndirectly() const;
    bool isForceEnabled() const; //�Ƿ�ǿ������
    bool isForceDisabled() const;
    QList<PluginDependency> dependencies() const; //��������б�

    typedef QList<PluginArgumentDescription> PluginArgumentDescriptions; //���������
    PluginArgumentDescriptions argumentDescriptions() const; //�����б�

    // other information, valid after 'Read' state is reached
    QString location() const; //λ����Ϣ
    QString filePath() const; //�ļ�Ŀ¼��Ϣ

    void setEnabled(bool value);
    void setDisabledByDefault(bool value);
    void setDisabledIndirectly(bool value);
    void setForceEnabled(bool value);
    void setForceDisabled(bool value);

	//��ȡ����������в���
    QStringList arguments() const;
	//�����ض��ڲ���������в���
    void setArguments(const QStringList &arguments);
    void addArgument(const QString &argument);

    bool provides(const QString &pluginName, const QString &version) const;

    // dependency specs, valid after 'Resolved' state is reached
    QHash<PluginDependency, PluginSpec *> dependencySpecs() const;

    // linked plugin instance, valid after 'Loaded' state is reached
    IPlugin *plugin() const; //������Ӧ�Ĳ��

    // state ״̬��ط���
    State state() const; //��ǰ״̬
    bool hasError() const; //�������
    QString errorString() const; //������Ϣ

private:
    PluginSpec();

    Internal::PluginSpecPrivate *d; //����Ĳ�������Լ��ڲ�����,dָ����Ϊ�˶����Ƽ���
    friend class Internal::PluginManagerPrivate;
    friend class Internal::PluginSpecPrivate; //���������Ϣ����˽�У���ζ�Ų���ƾ�ղ�����ֻ�����ɹ�����������
};

} // namespace ExtensionSystem

#endif // PLUGINSPEC_H

