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

#ifndef IPLUGIN_H
#define IPLUGIN_H

#include "extensionsystem_global.h"

#include <QObject>
#if QT_VERSION >= 0x050000
#    include <QtPlugin>
#endif

namespace ExtensionSystem {

namespace Internal {
    class IPluginPrivate;
    class PluginSpecPrivate;
}

class PluginManager;
class PluginSpec;
//һ�������2������� : �����ļ� �� һ��ʵ����IPlugin�ӿڵ�ʵ�� ����ڳ����˲������������
/*����ӿ���*/
class EXTENSIONSYSTEM_EXPORT IPlugin : public QObject
{
    Q_OBJECT

public:
    enum ShutdownFlag {
        SynchronousShutdown, //��Ҫ�ȴ�������������رղ��ܹر��Լ� ��ʱ��ô��Ϊ
        AsynchronousShutdown //����ȴ������ر�
    };

    IPlugin();
	//�����������ͷŵ� �����ע����Զ��ͷŶ���
    virtual ~IPlugin();

    //��ʼ������,�ڲ��������ʱ�����(����������ϵ�ӵ����ϳ�ʼ�������������)
    virtual bool initialize(const QStringList &arguments, QString *errorString) = 0;

    //�����в����initialize���������ú�,���øú���(���ʱ�����Ѿ���ʼ������˿��Թ�������ʱȥ����һЩ�������ע������������)
    virtual void extensionsInitialized() = 0;

    //�����в����extensionsInitialized������������Ժ���е���(��ʱ�����ò����һЩ��������ǰ��׼������)
    virtual bool delayedInitialize() { return false; }

    //�رշ�ʽ ͬ���ر� �����첽�ر�
    virtual ShutdownFlag aboutToShutdown() { return SynchronousShutdown; }

	//������qtcreator����ʱ��������һ��ʵ��ʱʹ�� - client����ִ��ʱ�������������е�ʵ���е��ò���Ĵ˺�����
	//����ض��Ĳ�����һ��ѡ���д��ݣ�������Ĳ�����һ�������д��ݡ�
	//���ʹ�� - block���򷵻���ֹ����ֱ��������ٵ�qobject��
    virtual QObject *remoteCommand(const QStringList & /* options */, const QStringList & /* arguments */) { return 0; }

	//������˲����Ӧ��PlugInspect ��������ļ�
    PluginSpec *pluginSpec() const;

	//�ڲ����������ע��obj�ı�������
    void addObject(QObject *obj);

	//�ڲ����������ע�����ͷ�obj�ı������� ��IPluginʵ��������ʱ��ͨ��addautoreleasedObject��ӵ����еĶ��󽫰�ע����෴˳���Զ�ɾ����
    void addAutoReleasedObject(QObject *obj);

	//�Ӳ��������ע������ı�������
    void removeObject(QObject *obj);

signals:
    void asynchronousShutdownFinished(); //����첽�رս����ص�

private:
    Internal::IPluginPrivate *d; //���������,�ŵ�һ������ָ�������Ϊ�˼��ݶ����ƣ������޸ĺ�lib���ø���ֻ��Ҫ����dll����Ϊ���ݶ�����������ݲ���ı�ͷ�ļ�

    friend class Internal::PluginSpecPrivate;
};

} // namespace ExtensionSystem

// The macros Q_EXPORT_PLUGIN, Q_EXPORT_PLUGIN2 become obsolete in Qt 5.
#if QT_VERSION >= 0x050000
#    if defined(Q_EXPORT_PLUGIN)
#        undef Q_EXPORT_PLUGIN
#        undef Q_EXPORT_PLUGIN2
#    endif
#    define Q_EXPORT_PLUGIN(plugin)
#    define Q_EXPORT_PLUGIN2(function, plugin)
#else
#    define Q_PLUGIN_METADATA(x)
#endif

#endif // IPLUGIN_H
