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

#ifndef EXTENSIONSYSTEM_PLUGINMANAGER_H
#define EXTENSIONSYSTEM_PLUGINMANAGER_H

#include "extensionsystem_global.h"
#include <aggregation/aggregate.h>

#include <QObject>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QTextStream;
class QSettings;
QT_END_NAMESPACE

namespace ExtensionSystem {
class PluginCollection;
class IPlugin;
class PluginSpec;

namespace Internal {
class PluginManagerPrivate; //����Ĺ�����ʵ����
}

class EXTENSIONSYSTEM_EXPORT PluginManager : public QObject
{
    Q_OBJECT

public:
	//����������
    static PluginManager *instance();

	//��ֵ��������,���������Ĺ�����ʵ�����d��Ա
    PluginManager();
    ~PluginManager();

    //====================================== Object pool operations ����ز�������==========================
	//��������ӵ�������У��Ա���԰����ʹӳ����ٴμ����ö���
    //��������������κ��ڴ����-��ӵĶ������ӳ���ɾ�������ɸ���ö�������ֶ�ɾ��������objectadded�����ź�

    static void addObject(QObject *obj);
    static void removeObject(QObject *obj);
	static QList<QObject *> allObjects(); //�����������ж�����б���ɸѡ����ͨ�����ͻ��˲���Ҫ���ô˺���
    static QReadWriteLock *listLock();
    template <typename T> static QList<T *> getObjects() //��ȡ����ָ�����͵Ķ���
    {
        QReadLocker lock(listLock());
        QList<T *> results;
        QList<QObject *> all = allObjects();
        QList<T *> result;
        foreach (QObject *obj, all) {
            result = Aggregation::query_all<T>(obj); //��������Ǿۺ϶��������ۺ�����������󿴳����壬(ֻҪ�����˸첲 ����Ϊ�Դ����ȵ�Ҳ������)
            if (!result.isEmpty())
                results += result;
        }
        return results;
    }
    template <typename T> static T *getObject() //��ȡһ��ָ�����͵Ķ���
    {
        QReadLocker lock(listLock());
        QList<QObject *> all = allObjects();
        T *result = 0;
        foreach (QObject *obj, all) {
            if ((result = Aggregation::query<T>(obj)) != 0)
                break;
        }
        return result;
    }

	//���ݶ����� ��ȡ����������������е�һ��
    static QObject *getObjectByName(const QString &name);
	//�������� ��ȡ�������м̳��ڴ�����ߴ�������е�һ��
    static QObject *getObjectByClassName(const QString &className);
	//====================================== Object pool operations ����ز�������Ending==========================



    //+++++++++++++++++ Plugin operations �����ط���+++++++++++++++++++++++++++++++++++
	static QList<PluginSpec *> loadQueue(); //������˳�򷵻ز���б� ����Ϊ��������ϵ���Բ��������Ҫ�Ե����ϣ������Ҫ�����˳��

	//���������: ���Լ�����ǰ�����ò������·��ʱ�ҵ������в��������Ĳ��������������ڼ������Ӧ����Ĵ�����Ϣ��״̬��Ϣ��
    static void loadPlugins();

	//��ȡ����������Բ��������·���б�(�����ֹ������һ��·��)
    static QStringList pluginPaths();

	//���ò������·���б����б��е�·���ᱻ�������ݹ�ļ��������xml�����ļ�
    static void setPluginPaths(const QStringList &paths); //���ò��Ŀ¼ ��ȥreadĿ¼���ز��������,ǿ�Ƽ�����

	//�ڲ������·�����ҵ������в����������Ϣ��,�������Լ������������е����������ָ���Ǹ����������ʵ����������ʾ��ǰ���������״̬
    static QList<PluginSpec *> plugins();

	//��ȡ�������� (��Ϊ����ǳ��࣬���Խ����˹��� eg: �������� ţ�� ���⣻ �������� ���� Ѽ���ȣ�)ÿ�ֲ���и������
    static QHash<QString, PluginCollection *> pluginCollections();

	//���ò�������ļ��ĺ�׺��  Ĭ���� .xml
    static void setFileExtension(const QString &extension);

	//��ȡ��������ļ��ĺ�׺�� Ĭ�� .xml
    static QString fileExtension();

	//��ȡ������ع����Ƿ������loadPlugins���ú����,��������������ع��̳��ִ���,���ᷴ�ڴ���
    static bool hasError();
	//+++++++++++++++++ Plugin operations �����ط���Ending+++++++++++++++++++++++++++++++++++


    // -----------------------------Settings ������������������Ŀ----------------------------
	//ע�������Ʒͨ������Ĭ�����ã���������û��������ã���ô�Ͱ��û������� ��ˣ������û�������ȫ��������2���������
	//

	//���������й������ú��ѽ��ò������Ϣ���û��ض����á�
    static void setSettings(QSettings *settings); //������ͨ���ö��� �����û��������ļ�

	//���������й������ú��ѽ��ò������Ϣ���û��ض����á�
    static QSettings *settings();

	//���������й�Ĭ�Ͻ��ò������Ϣ��ȫ�֣����û��޹أ����á�
    static void setGlobalSettings(QSettings *settings);//����ȫ�����ö��� ����ȫ����Ч�������ļ�

	//���������й�Ĭ�Ͻ��ò������Ϣ��ȫ�֣����û��޹أ����á�
    static QSettings *globalSettings();

	//���û�����Ĭ�����õ��޸�д���û��ض������С�(�����û���qtcreator����ѡ��������ĳ�������������Щ�ͻ�д�룬��ʵ����ͬ�����������������״̬������)
    static void writeSettings();
	// -----------------------------Settings ������������������ĿEnding----------------------------


    //+++++++++++++++++++++++++++++++++ command line arguments+++++++++++++++++++++++++++++++++++

	//���������µĲ������Ȳ�����������Ҳ���ǲ����������ͨ��������Ҫ�򿪵��ļ��б����Է��־��������в�����������ͨ�������иı�qtcreator��ɫ����Ϊ��ɫ
    static QStringList arguments();

	/*
	��ȡ����args�е�������ѡ���б�������з�����
    ����������������ֱ�Ӵ���һЩѡ���- noload<plugin>�����������ע���ѡ����ӵ���������С�
	�����ߣ�Ӧ�ó��򣩿���ͨ������appoptions�б�Ϊѡ��ע���Լ������б�����ɶԵġ�ѡ���ַ�������һ��bool��ָʾѡ���Ƿ���Ҫ������
    Ӧ�ó���ѡ�����Ǹ����κβ����ѡ�
    �����ҵ����κ�Ӧ�ó���ѡ�����foundappoptions������Ϊ�ԣ���option string������argument������
    �޷������������ѡ�����ͨ��arguments��������������
    ���������������ȱ����Ҫһ��ѡ��Ĳ������������errorstring�����ô������������Ϣ

	ע�� ���ϴ�������ṩ���ע������������ָ��Ĺ��ܣ����� -color red �����������������aaa����ȣ�aaaע����-color����Ҫ���������ɫ
	*/
    static bool parseOptions(const QStringList &args,
        const QMap<QString, bool> &appOptions,
        QMap<QString, QString> *foundAppOptions,
        QString *errorString);

	//Ϊ�����а�����ʽ�����������������ѡ� �淶�������� -help���
    static void formatOptions(QTextStream &str, int optionIndentation, int descriptionIndentation);

	//Ϊ�����а�����ʽ����������Ĳ��ѡ��淶��cmd ������������
    static void formatPluginOptions(QTextStream &str, int optionIndentation, int descriptionIndentation);

	//Ϊ�����а�����ʽ������淶�İ汾�� �淶�� cmd ��������ļ��汾�����
    static void formatPluginVersions(QTextStream &str);

	//���л����ѡ��Ͳ������Ա�ͨ��qtsingleapplication���͵����ַ���������myplugin - option1 - option2����������1����2����
	//�Դ�ð�ŵĹؼ��ֿ�ͷ���б����������
    static QString serializedArguments();

	//�������� Ӧ���������� ��ʱ����д
    static bool testRunRequested();
	//��������ʱ ���ݴ�ŵ�Ŀ¼ ����������������������
    static QString testDataDirectory();

	//����һ��������Ŀ����ʾ�������ʱ������ʱ�䡣 ��ʱ����д
    static void profilingReport(const char *what, const PluginSpec *spec = 0);

	//���س��������е�ϵͳ win��mac��Linux֮��
    static QString platformName();

signals:
    void objectAdded(QObject *obj);
    void aboutToRemoveObject(QObject *obj);

    void pluginsChanged(); //�����ڲ���һ�� QMetaObject::activate(this, &staticMetaObject, 2, nullptr); 1�����źŶ��� 2�����źŶ����Ԫ���� 3�ź�������� 4��κͷ���ֵ
    
    void initializationDone();

public slots:
    void remoteArguments(const QString &serializedArguments, QObject *socket); //socket�յ����ݵ���
    void shutdown(); //�ر��źŵ���

private slots:
    void startTests();
    friend class Internal::PluginManagerPrivate; //�ڲ�ʵ�������Ե�ˣ������ɷ��ʱ���˽�з���
};

} // namespace ExtensionSystem

#endif // EXTENSIONSYSTEM_PLUGINMANAGER_H
