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

#ifndef PROGRESSMANAGER_P_H
#define PROGRESSMANAGER_P_H

#include "progressmanager.h"

#include <QFutureWatcher>
#include <QList>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QPointer>
#include <QPropertyAnimation>
#include <QToolButton>

namespace Core {

class StatusBarWidget;

namespace Internal {

class ProgressBar;
class ProgressView;

/*
//ProgressManager��������ʾ��Qt Creator������������û����档
//�����ٱ���֪������Ľ��ȣ�����Qt Creator�����ڵ����½����û���ʾ����ָʾ����
//����ָʾ���������û�ȡ������
//һ���������ɣ�
//1. �������  ��ʾ���𱨸�����״̬�������Qfuture�����й����Ϊ�ض����񴴽��˶���ı���ģʽ����μ����ġ�
//2. ����     ��������ļ�̱��⡣���ڽ���������ʾΪ���⡣
//3. ����     ���ڽ�����ͬһ���� �Ĳ�ͬ������� ���ַ�����ʶ�������磬��������������ʹ����ͬ�����ͱ�ʶ����
//4. ��־λ   ָ��������Ӧ�����ʾ���û���������־��

//Ҫע�������봴��qfuture<void>����Ȼ�����addtask�������˺�������һ��core:��FutureProgress FutureProgress���󣬿����ڽ�һ���Զ������������ۡ�
//�й���ϸ��Ϣ�������Core:��FutureProgress FutureProgress�ĵ���

//������������У������˽����Ϊ�������񴴽�qfuture<void>��������ֳ���ģʽ��
//��2��ʹ��qtconcurrent�����̻߳������һ��ѡ����ֱ��ʹ��qtconcurrent�ڲ�ͬ���߳���ʵ��ͬʱ��������
//qtconcurrent�м�����ͬ�ĺ����������У������ڲ�ͬ���߳�������һ���ຯ����qt creator������src/libs/qtconcurrent/runextensions.h������˸������ݡ�
//���в��������qtconcurrent��������qfuture����������Ҫ��addtask����������ΪProgressManager�ṩ�ġ�
//�뿴һ������locator:��ilocatorfilter����λ��������ʵ����qfutureInterface����Ϊ������ˢ�º�������Щ���ܿ�������
\code
void Filter::refresh(QFutureInterface<void> &future) {
future.setProgressRange(0, MAX);
...
while (!future.isCanceled()) {
// Do a part of the long stuff
...
future.setProgressValue(currentProgress);
...
}
}
\endcode
ʵ��ˢ���ڲ�ͬ�߳��е�������ɸѡ����ˢ�º�����������ʾ��
\code
QFuture<void> task = QtConcurrent::run(&ILocatorFilter::refresh, filters);
Core::FutureProgress *progress = Core::ProgressManager::addTask(task, tr("Indexing"),
Locator::Constants::TASK_INDEX);
\endcode
���ȣ����Ǹ���qtconcurrent����һ���������й�������ˢ�º������̡߳�֮��������ProgressManagerע�᷵�ص�qfuture����

��2���ֶ�Ϊ�̴߳���qtconcurrent��������������Լ��ķ����������������̣߳�����Ҫ�Լ�������Ҫ�Ķ��󣬲���������/ֹͣ״̬��
\code
// We are already running in a different thread here
QFutureInterface<void> *progressObject = new QFutureInterface<void>;
progressObject->setProgressRange(0, MAX);
Core::ProgressManager::addTask(progressObject->future(), tr("DoIt"), MYTASKTYPE);
progressObject->reportStarted();
// Do something
...
progressObject->setProgressValue(currentProgress);
...
// We have done what we needed to do
progressObject->reportFinished();
delete progressObject;
\endcode
�ڵ�һ���У����Ǵ���qfutureInterface�������������Ǳ�������״̬�ķ��������Ǳ���ĵ�һ�����ǽ���ֵ��Ԥ�ڷ�Χ��
����ʹ��ΪQfutureInterface���󴴽����ڲ�Qfuture������ProgressManagerע�����񡣽����������Ǳ��������Ѿ���ʼ����ʼִ�����ǵ�ʵ�ʹ�����
����ͨ��qfutureinterface�еĺ���������ȡ��ڳ�ȡ������ɺ�����ͨ��qfutureInterface���󱨸棬Ȼ����ɾ����

��1���Զ���������
������ʹ��addtask�����������ص�futureProgress���󣬽��Զ���С��������Ϊ��ʾ�ڽ�����������·���������ʹ�ô˶������û���������ָʾ��ʱ�õ�֪ͨ��


enum Core::ProgressManager::ProgressFlag
����ָ����Ϊ��ϸ��Ϣ��������־�������Ĭ�������ǲ������κ���Щ��־��ֵkeeponFinish������ɺ����ָʾ�����ֿɼ���
ֵshowInApplication������Ľ���ָʾ����ϵͳ����������ƽ̨�ϵ�Ӧ�ó���ͼ���ж�����ʾ��֧�ָù��ܵ�RMS��Ŀǰ��Windows7��Mac OS X����


FutureProgress *Core::ProgressManager::addTask(const QFuture<void> &future, const QString &title, const QString &type, ProgressFlags flags = 0)
��ʾ��qfuture����future����������Ľ���ָʾ��������ָʾ����ʾָ���ı���ͽ��������������ͽ�ָ���������������е�������߼����顣
ͨ��Flags�������������ý���ָʾ����������ɺ󱣳ֿɼ�������һ����ʾ�Ѵ�������ָʾ���Ķ��󣬿����ڽ�һ���Զ��塣
FutureProgress���������������ProgressManager��������ֻ֤�����浽��һ���¼�ѭ�����ڣ���ֱ����һ�ε���AddTask��
�����ϣ���ڵ��ô˺���֮������ʹ�÷��ص�FutureProgress������Ҫʹ�ñ������ܣ�������qPointer�а�װ���صĶ��󣬲���ÿ��ʹ��ʱ���0����

void Core::ProgressManager::setApplicationLabel(const QString &text)
��ϵͳ��������ͣ������Ӧ�ó���ͼ���У���ƽ̨��ط�ʽ��ʾ������\\�ı�����������ʾWindows7��Mac OS X�ϵ����ɴ�������

void Core::ProgressManager::cancelTasks(Core::Id type)
Ϊ�������͵������������е�����ƻ�ȡ������ע�⣬ȡ������ȡ����ʵ�ʼ��qfutureInterface:��IsCanceled���Ե���������

void Core::ProgressManager::taskStarted(Core::Id type)
���������͵���������ʱ���͡�

void Core::ProgressManager::allTasksFinished(Core::Id type)
��ĳ���͵������������ʱ���͡�

*/
class ProgressManagerPrivate : public Core::ProgressManager
{
    Q_OBJECT
public:
    ProgressManagerPrivate();
    ~ProgressManagerPrivate();
    void init();
    void cleanup();

    FutureProgress *doAddTask(const QFuture<void> &future, const QString &title, Id type,
                            ProgressFlags flags);

    void doSetApplicationLabel(const QString &text);
    ProgressView *progressView();

public slots:
    void doCancelTasks(Core::Id type);

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private slots:
    void taskFinished();
    void cancelAllRunningTasks();
    void setApplicationProgressRange(int min, int max);
    void setApplicationProgressValue(int value);
    void setApplicationProgressVisible(bool visible);
    void disconnectApplicationTask();
    void updateSummaryProgressBar();
    void fadeAwaySummaryProgress();
    void summaryProgressFinishedFading();
    void progressDetailsToggled(bool checked);
    void updateVisibility();
    void updateVisibilityWithDelay();
    void updateStatusDetailsWidget();

    void slotRemoveTask();
private:
    void readSettings();
    void initInternal();
    void stopFadeOfSummaryProgress();

    bool hasError() const;
    bool isLastFading() const;

    void removeOldTasks(Id type, bool keepOne = false);
    void removeOneOldTask();
    void removeTask(FutureProgress *task);
    void deleteTask(FutureProgress *task);

    QPointer<ProgressView> m_progressView;
    QList<FutureProgress *> m_taskList;
    QMap<QFutureWatcher<void> *, Id> m_runningTasks;
    QFutureWatcher<void> *m_applicationTask;
    Core::StatusBarWidget *m_statusBarWidgetContainer;
    QWidget *m_statusBarWidget;
    QWidget *m_summaryProgressWidget;
    QHBoxLayout *m_summaryProgressLayout;
    QWidget *m_currentStatusDetailsWidget;
    QPointer<FutureProgress> m_currentStatusDetailsProgress;
    ProgressBar *m_summaryProgressBar;
    QGraphicsOpacityEffect *m_opacityEffect;
    QPointer<QPropertyAnimation> m_opacityAnimation;
    bool m_progressViewPinned;
    bool m_hovered;
};

class ToggleButton : public QToolButton
{
    Q_OBJECT
public:
    ToggleButton(QWidget *parent);
    QSize sizeHint() const;
    void paintEvent(QPaintEvent *event);
};

} // namespace Internal
} // namespace Core

#endif // PROGRESSMANAGER_P_H
