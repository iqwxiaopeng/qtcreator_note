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

#include <QApplication>

QT_FORWARD_DECLARE_CLASS(QSharedMemory)

namespace SharedTools {

class QtLocalPeer;

class QtSingleApplication : public QApplication
{
    Q_OBJECT

public:
	/*�������
	1. ����appid������sessionid
	2. �����˹����ڴ�,��д��id��һЩ����
	3. ������һ��lock�ļ�ʵ��
	4. �������񣬲�����������ͬʱ���������ӻص������ݻص�
	*/
    QtSingleApplication(const QString &id, int &argc, char **argv);
    ~QtSingleApplication();

    bool isRunning(qint64 pid = -1);

    void setActivationWindow(QWidget* aw, bool activateOnMessage = true);
    QWidget* activationWindow() const;
    bool event(QEvent *event);

    QString applicationId() const;
    void setBlock(bool value);

public Q_SLOTS:
    bool sendMessage(const QString &message, int timeout = 5000, qint64 pid = -1);
    void activateWindow();

Q_SIGNALS:
	//�յ���Ϣ�ص�
    void messageReceived(const QString &message, QObject *socket);
    void fileOpenRequest(const QString &file);

private:
    QString instancesFileName(const QString &appId);

    qint64 firstPeer;
    QSharedMemory *instances; //�����ڴ����
    QtLocalPeer *pidPeer; //ͨѶ�������
    QWidget *actWin;
    QString appId; //����ʱ�����appID������
    bool block;
};

} // namespace SharedTools