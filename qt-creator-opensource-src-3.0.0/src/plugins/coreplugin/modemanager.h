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

#ifndef MODEMANAGER_H
#define MODEMANAGER_H

#include <coreplugin/core_global.h>
#include <coreplugin/id.h>
#include <QObject>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Core {

class IMode;

namespace Internal {
    class MainWindow;
    class FancyTabWidget;
}
//ModeManager�ṩ��FancyTabWidget��Mode֮���ͨ������
class CORE_EXPORT ModeManager : public QObject
{
    Q_OBJECT

public:
    //��ȡ������ʵ��
    static QObject *instance();

	//��ȡ��ǰ�� ģʽ tab����ǰѡ��{��ӭ���� ���� �༭���� ���� ...}
    static IMode *currentMode();

	//����ģʽid��ȡģʽ {����ľ�̬�������ڲ�����һ��ģʽ�� List<IMode>}
    static IMode *mode(Id id);
    //�������ȼ���Corner����������
    static void addAction(QAction *action, int priority);

    //����������ȼ��Ĳ�����Ŀ ����Ŀѡ������
    static void addProjectSelector(QAction *action);

	//��Corner ����һ����
    static void addWidget(QWidget *widget);

	//����id ����Tab����ǰ��Ծ�˵���(Ҳ�������õ�ǰģʽ)
    static void activateMode(Id id);

    //���ý��㵽��ǰģʽ���� ���õ�ǰ�����������ʾ
    static void setFocusToCurrentMode();

    //��ȡ ģ��ѡ���� ��ർ��  �Ƿ���ʾ
    static bool isModeSelectorVisible();

public slots:
//���� ģ��ѡ���� ��ർ��  �Ƿ���ʾ
    static void setModeSelectorVisible(bool visible);

signals:
    void currentModeAboutToChange(Core::IMode *mode);

    // the default argument '=0' is important for connects without the oldMode argument.
    void currentModeChanged(Core::IMode *mode, Core::IMode *oldMode = 0);

private slots:
//����id ����Tab����ǰ��Ծ�˵���(Ҳ�������õ�ǰģʽ) ��raise����
    void slotActivateMode(int id);

    //����������ģʽ
    void objectAdded(QObject *obj);

    //�Ƴ�ģʽ 1.ģʽ���Ƴ� 2.��ݰ������Ƴ� 3.Tab�˵����Ƴ� 4.�������Ƴ�ģʽ���
    void aboutToRemoveObject(QObject *obj);


    void currentTabAboutToChange(int index);
    void currentTabChanged(int index);

    //ģʽ��ݷ�ʽ�ı���Ϣ����
    void updateModeToolTip();

    //����ģʽ������Ϣ
    void enabledStateChanged();

    //PluginManger initializationDone �źŴ���
    void handleStartup();

    //ICore��coreAboutToClose �źŴ���
    void handleShutdown();

private:
	//���� ��Ҫ������ �Լ� �������е�����  (FancyTabWidget��������ർ���Լ�����������)
    explicit ModeManager(Internal::MainWindow *mainWindow, Internal::FancyTabWidget *modeStack);
    ~ModeManager();

	//��ʼ�� ����� �Ƴ�model�¼�
    static void init();

    friend class Core::Internal::MainWindow;
};

} // namespace Core

#endif // MODEMANAGER_H
