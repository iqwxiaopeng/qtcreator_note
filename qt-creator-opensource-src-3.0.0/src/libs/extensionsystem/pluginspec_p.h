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

#ifndef PLUGINSPEC_P_H
#define PLUGINSPEC_P_H

#include "pluginspec.h"
#include "iplugin.h"

#include <QObject>
#include <QStringList>
#include <QXmlStreamReader>
#include <QRegExp>

namespace ExtensionSystem {

class IPlugin;
class PluginManager;

namespace Internal {

    /*��������� �ڲ�ʵ����
    */
class EXTENSIONSYSTEM_EXPORT PluginSpecPrivate : public QObject
{
    Q_OBJECT

public:
    PluginSpecPrivate(PluginSpec *spec);

    //1. ��ȡ��������ļ��е���Ϣ����ʵ���������������
    //2. ���²��������״̬Ϊ�Ѿ���
    bool read(const QString &fileName);//��ȡһ����������ļ�
    bool provides(const QString &pluginName, const QString &version) const; //�жϱ�����Ƿ�������ΪpluginName�汾ΪpluginVersion�Ĳ��

    //�����Ĳ���specs��������Ŀ¼�����ѵ������в����������
    bool resolveDependencies(const QList<PluginSpec *> &specs);//�����������Ƿ��ڲ������specs�����ݼ�������ǿ����ϵ ���ɲ��ӳ���dependencySpecs
    bool loadLibrary(); //���ز����ָ���Ķ�̬�� ���������ʵ��
    bool initializePlugin(); //��ʼ����� ���ò����initialize����
    bool initializeExtensions();//����������Ŀ���г�ʼ��
    bool delayedInitialize(); //���ò�����ӳٳ�ʼ�� delayedInitialize
    IPlugin::ShutdownFlag stop();
    void kill();

    QString name; //�����
    QString version;//����汾
    QString compatVersion;//������ݰ汾
    bool experimental;//�Ƿ���԰汾
    bool disabledByDefault;//���Ĭ�ϲ����ñ��
    QString vendor;//����ṩ��
    QString copyright;//����İ�Ȩ
    QString license;//�������ȨЭ��
    QString description;//����ľ�������
    QString url;//�����url��ַ
    QString category;//������������
    QRegExp platformSpecification; //�洢���ƽ̨��������
    QList<PluginDependency> dependencies; //����������� (һ�����������������������)
    bool enabledInSettings;//�������ñ�� �� �����ñ���෴
    bool disabledIndirectly; //������ñ�� Ϊ���򲻻��ڳ�������ʱ����
    bool forceEnabled; //ǿ���������ر��
    bool forceDisabled; //ǿ������������

    QString location; //��������ļ����ڵľ���Ŀ¼
    QString filePath; //��������ļ��ľ���λ��
    QStringList arguments; //����������в��� ��������

    QHash<PluginDependency, PluginSpec *> dependencySpecs; //����������������Ӧ��ϵ�� PluginDependency���������Ϣ PluginSpec���������Ϣ
    PluginSpec::PluginArgumentDescriptions argumentDescriptions;//��������ļ��еĲ�����
    IPlugin *plugin; //ָ����صĲ������ **����Ǿ���Ĳ������

    PluginSpec::State state; //��������ļ���ǰ��״̬ EG:�ѱ���ȡ״̬ PluginSpec::Read;
    bool hasError; //����������̴����¼ �����־
    QString errorString; //������Ϣ��¼

    //�ж��Ƿ�֧�ָð汾 �ڲ����� versionRegExp�����ж�
    static bool isValidVersion(const QString &version);
    static int versionCompare(const QString &version1, const QString &version2); // �汾�Ƚ� 

    //������������ļ�����ñ�� Ϊ�� ����ζ�Ų��������ʱ���ᱻ����
    void disableIndirectlyIfDependencyDisabled();


private:
    PluginSpec *q; //������ָ���Ķ���ӿڵ�ָ��(�ӿ�ͨ��dָ��ָ���࣬����ͨ��qָ��ָ�����ӿڶ���)

    bool reportError(const QString &err);
    void readPluginSpec(QXmlStreamReader &reader);//����xml��ʽ�Ĳ����������
    void readDependencies(QXmlStreamReader &reader); //��xml�ڵ��ȡ�������
    void readDependencyEntry(QXmlStreamReader &reader);//��ȡһ��������Ϣ������������
    void readArgumentDescriptions(QXmlStreamReader &reader); // �����������
    void readArgumentDescription(QXmlStreamReader &reader); //��ȡһ��������Ϣ�����������
    bool readBooleanValue(QXmlStreamReader &reader, const char *key);//��ȡkey ��Ŀboolֵ

    // ��ǰ֧�ֲ���汾������ƥ��
    static QRegExp &versionRegExp();
};

} // namespace Internal
} // namespace ExtensionSystem

#endif // PLUGINSPEC_P_H
