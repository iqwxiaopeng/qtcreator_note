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

#include "qtsingleapplication.h"
#include "qtlocalpeer.h"

#include <qtlockedfile.h>

#include <QDir>
#include <QFileOpenEvent>
#include <QSharedMemory>
#include <QWidget>

namespace SharedTools {

static const int instancesSize = 1024;

static QString instancesLockFilename(const QString &appSessionId)//这个函数返回一个名为appSessionId的文件路径
{
    const QChar slash(QLatin1Char('/'));
    QString res = QDir::tempPath();//C:/Users/usr/AppData/Local/Temp 取得系统的临时目录
    if (!res.endsWith(slash))
        res += slash;
    return res + appSessionId + QLatin1String("-instances");
}


/*这个函数做了: appid是软件名字
1. 根据appid生成了sessionid
2. 创建了共享内存,并写入id和一些数据
3. 创建了一个lock文件实例
4. 创建服务，并开启监听，同时绑定了新连接回调和数据回调
*/
QtSingleApplication::QtSingleApplication(const QString &appId, int &argc, char **argv)
    : QApplication(argc, argv),
      firstPeer(-1),
      pidPeer(0)
{
    this->appId = appId;

    const QString appSessionId = QtLocalPeer::appSessionId(appId);//得到一个当前进程的唯一ID值,这个值也将作为共享内存ID值

    // This shared memory holds a zero-terminated array of active (or crashed) instances
    instances = new QSharedMemory(appSessionId, this);
    actWin = 0;
    block = false;

    // First instance creates the shared memory, later instances attach to it
    const bool created = instances->create(instancesSize);//开共享内存空间
    if (!created) {
        if (!instances->attach()) {
            qWarning() << "Failed to initialize instances shared memory: "
                       << instances->errorString();
            delete instances;
            instances = 0;
            return;
        }
    }

    // QtLockedFile is used to workaround QTBUG-10364 一个加入了不同锁类型的文件封装类
    QtLockedFile lockfile(instancesLockFilename(appSessionId));//在系统临时目录创建一个名为appSessionId的临时文件后缀-instance

    lockfile.open(QtLockedFile::ReadWrite);
    lockfile.lock(QtLockedFile::WriteLock);
    qint64 *pids = static_cast<qint64 *>(instances->data()); //拿到共享内存区
    if (!created) {
        // Find the first instance that it still running
        // The whole list needs to be iterated in order to append to it
        for (; *pids; ++pids) {
            if (firstPeer == -1 && isRunning(*pids))
                firstPeer = *pids;
        }
    }
    // Add current pid to list and terminate it
    *pids++ = QCoreApplication::applicationPid(); //把当前应用的pid放入 然后放入0
    *pids = 0;
	//创建一个通讯节点
    pidPeer = new QtLocalPeer(this, appId + QLatin1Char('-') +
                              QString::number(QCoreApplication::applicationPid()));
	//绑定接收回调
    connect(pidPeer, SIGNAL(messageReceived(QString,QObject*)), SIGNAL(messageReceived(QString,QObject*)));

	//开启监听 但是不是ip而是监听服务名,并且绑定新连接回调
    pidPeer->isClient();

    lockfile.unlock();
}

QtSingleApplication::~QtSingleApplication()
{
    if (!instances)
        return;
    const qint64 appPid = QCoreApplication::applicationPid();
    QtLockedFile lockfile(instancesLockFilename(QtLocalPeer::appSessionId(appId)));
    lockfile.open(QtLockedFile::ReadWrite);
    lockfile.lock(QtLockedFile::WriteLock);
    // Rewrite array, removing current pid and previously crashed ones
    qint64 *pids = static_cast<qint64 *>(instances->data());
    qint64 *newpids = pids;
    for (; *pids; ++pids) {
        if (*pids != appPid && isRunning(*pids))
            *newpids++ = *pids;
    }
    *newpids = 0;
    lockfile.unlock();
}

bool QtSingleApplication::event(QEvent *event)
{
    if (event->type() == QEvent::FileOpen) {
        QFileOpenEvent *foe = static_cast<QFileOpenEvent*>(event);
        emit fileOpenRequest(foe->file());
        return true;
    }
    return QApplication::event(event);
}

bool QtSingleApplication::isRunning(qint64 pid)
{
    if (pid == -1) {
        pid = firstPeer;
        if (pid == -1)
            return false;
    }

    QtLocalPeer peer(this, appId + QLatin1Char('-') + QString::number(pid, 10));
    return peer.isClient();
}

bool QtSingleApplication::sendMessage(const QString &message, int timeout, qint64 pid)
{
    if (pid == -1) {
        pid = firstPeer;
        if (pid == -1)
            return false;
    }

    QtLocalPeer peer(this, appId + QLatin1Char('-') + QString::number(pid, 10));
    return peer.sendMessage(message, timeout, block);
}

QString QtSingleApplication::applicationId() const
{
    return appId;
}

void QtSingleApplication::setBlock(bool value)
{
    block = value;
}

void QtSingleApplication::setActivationWindow(QWidget *aw, bool activateOnMessage)
{
    actWin = aw;
    if (!pidPeer)
        return;
    if (activateOnMessage)
        connect(pidPeer, SIGNAL(messageReceived(QString,QObject*)), this, SLOT(activateWindow()));
    else
        disconnect(pidPeer, SIGNAL(messageReceived(QString,QObject*)), this, SLOT(activateWindow()));
}


QWidget* QtSingleApplication::activationWindow() const
{
    return actWin;
}


void QtSingleApplication::activateWindow()
{
    if (actWin) {
        actWin->setWindowState(actWin->windowState() & ~Qt::WindowMinimized);
        actWin->raise();
        actWin->activateWindow();
    }
}

} // namespace SharedTools
