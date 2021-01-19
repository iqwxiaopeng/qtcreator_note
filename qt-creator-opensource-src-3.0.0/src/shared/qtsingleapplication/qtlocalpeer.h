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

#include <qtlockedfile.h>

#include <QLocalServer>
#include <QLocalSocket>
#include <QDir>
/*
一个纯粹的收发消息的服务类
*/
namespace SharedTools {

class QtLocalPeer : public QObject
{
    Q_OBJECT

public:
	//这个函数调用appSessionId创建session值并在系统临时目录创建了lock文件
    explicit QtLocalPeer(QObject *parent = 0, const QString &appId = QString());

	//开启对服务的监听 并绑定新连接事件 到receiveConnection
    bool isClient();

    bool sendMessage(const QString &message, int timeout, bool block);
    QString applicationId() const
        { return id; }

	//这个函数把字符串appID转换为一个与当前进程id相关的另外一个字符串id
    static QString appSessionId(const QString &appId);

Q_SIGNALS:
	//数据到来处理函数
    void messageReceived(const QString &message, QObject *socket);

protected Q_SLOTS:
//连接到来事件处理函数
    void receiveConnection();

protected:
    QString id; //构建函数传入的appid
    QString socketName; //服务名 由 appSessioId函数传入appid生成
    QLocalServer* server; //服务套接字
    QtLockedFile lockFile; //初始化时候创建的锁文件
};

} // namespace SharedTools
