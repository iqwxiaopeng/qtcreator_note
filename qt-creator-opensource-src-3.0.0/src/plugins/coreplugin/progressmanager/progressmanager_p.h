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
//ProgressManager类用于显示在Qt Creator中运行任务的用户界面。
//它跟踪被告知的任务的进度，并在Qt Creator主窗口的右下角向用户显示进度指示器。
//进度指示器还允许用户取消任务。
//一个任务的组成：
//1. 任务抽象  表示负责报告任务状态的任务的Qfuture对象。有关如何为特定任务创建此对象的编码模式，请参见下文。
//2. 标题     描述任务的简短标题。这在进度条上显示为标题。
//3. 类型     用于将属于同一任务 的不同任务分组 的字符串标识符。例如，所有搜索操作都使用相同的类型标识符。
//4. 标志位   指定进度条应如何显示给用户的其他标志。

//要注册任务，请创建qfuture<void>对象，然后调用addtask（）。此函数返回一个core:：FutureProgress FutureProgress对象，可用于进一步自定义进度条的外观。
//有关详细信息，请参阅Core:：FutureProgress FutureProgress文档。

//在下面的内容中，您将了解如何为您的任务创建qfuture<void>对象的两种常见模式。
//第2节使用qtconcurrent创建线程化任务第一个选项是直接使用qtconcurrent在不同的线程中实际同时启动任务。
//qtconcurrent有几个不同的函数可以运行，例如在不同的线程中运行一个类函数。qt creator本身在src/libs/qtconcurrent/runextensions.h中添加了更多内容。
//运行并发任务的qtconcurrent函数返回qfuture对象。这是您要在addtask（）函数中为ProgressManager提供的。
//请看一下例如locator:：ilocatorfilter。定位器过滤器实现以qfutureInterface对象为参数的刷新函数。这些功能看起来像：
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
实际刷新在不同线程中调用所有筛选器的刷新函数，如下所示：
\code
QFuture<void> task = QtConcurrent::run(&ILocatorFilter::refresh, filters);
Core::FutureProgress *progress = Core::ProgressManager::addTask(task, tr("Indexing"),
Locator::Constants::TASK_INDEX);
\endcode
首先，我们告诉qtconcurrent启动一个调用所有过滤器的刷新函数的线程。之后，我们向ProgressManager注册返回的qfuture对象。

第2节手动为线程创建qtconcurrent对象如果任务有自己的方法来创建和运行线程，则需要自己创建必要的对象，并报告启动/停止状态。
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
在第一行中，我们创建qfutureInterface对象，它将是我们报告任务状态的方法。我们报告的第一件事是进度值的预期范围。
我们使用为QfutureInterface对象创建的内部Qfuture对象向ProgressManager注册任务。接下来，我们报告任务已经开始并开始执行我们的实际工作，
定期通过qfutureinterface中的函数报告进度。在长取操作完成后，我们通过qfutureInterface对象报告，然后将其删除。

第1节自定义进度外观
您可以使用addtask（）函数返回的futureProgress对象，将自定义小部件设置为显示在进度条本身的下方。还可以使用此对象在用户单击进度指示器时得到通知。


enum Core::ProgressManager::ProgressFlag
用于指定行为详细信息的其他标志。任务的默认设置是不设置任何这些标志。值keeponFinish任务完成后进度指示器保持可见。
值showInApplication此任务的进度指示器在系统的任务栏或平台上的应用程序图标中额外显示。支持该功能的RMS（目前是Windows7和Mac OS X）。


FutureProgress *Core::ProgressManager::addTask(const QFuture<void> &future, const QString &title, const QString &type, ProgressFlags flags = 0)
显示由qfuture对象future给定的任务的进度指示器。进度指示器显示指定的标题和进度条。任务类型将指定与其他正在运行的任务的逻辑分组。
通过Flags参数，您可以让进度指示器在任务完成后保持可见。返回一个表示已创建进度指示器的对象，可用于进一步自定义。
FutureProgress对象的生命周期由ProgressManager管理，并保证只能生存到下一个事件循环周期，或直到下一次调用AddTask。
如果您希望在调用此函数之后立即使用返回的FutureProgress，则需要使用保护功能（例如在qPointer中包装返回的对象，并在每次使用时检查0）。

void Core::ProgressManager::setApplicationLabel(const QString &text)
在系统任务栏或停靠区的应用程序图标中，以平台相关方式显示给定的\\文本。这用于显示Windows7和Mac OS X上的生成错误数。

void Core::ProgressManager::cancelTasks(Core::Id type)
为给定类型的所有正在运行的任务计划取消。请注意，取消功能取决于实际检查qfutureInterface:：IsCanceled属性的运行任务。

void Core::ProgressManager::taskStarted(Core::Id type)
当给定类型的任务启动时发送。

void Core::ProgressManager::allTasksFinished(Core::Id type)
当某类型的所有任务完成时发送。

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
