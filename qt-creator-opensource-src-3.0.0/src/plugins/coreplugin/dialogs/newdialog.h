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

#ifndef NEWDIALOG_H
#define NEWDIALOG_H

#include "iwizard.h"

#include <QDialog>
#include <QIcon>
#include <QList>

QT_BEGIN_NAMESPACE
class QAbstractProxyModel;
class QModelIndex;
class QSortFilterProxyModel;
class QPushButton;
class QStandardItem;
class QStandardItemModel;
class QStringList;
QT_END_NAMESPACE

namespace Core {

namespace Internal {

namespace Ui {
    class NewDialog;
}

class NewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewDialog(QWidget *parent);
    virtual ~NewDialog();

	//参数为所有注册到系统的引导对象  设置界面的数据对象 界面是view，参数是界面的model
    void setWizards(QList<IWizard*> wizards);

    Core::IWizard *showDialog();

	//模版选择 (参考IWizard图片对应函数名) [所有模版，桌面模版],返回模版名
    QString selectedPlatform() const;


    int selectedWizardOption() const;

private slots:
    void currentCategoryChanged(const QModelIndex &);
    void currentItemChanged(const QModelIndex &);
    void okButtonClicked();
    void updateOkButton();
    void setSelectedPlatform(const QString &platform);

private:
    Core::IWizard *currentWizard() const;
    void addItem(QStandardItem *topLEvelCategoryItem, IWizard *wizard);

    Ui::NewDialog *m_ui;
    QStandardItemModel *m_model;//处理二维数据的数据模型
    QAbstractProxyModel *m_twoLevelProxyModel;
    QSortFilterProxyModel *m_filterProxyModel;
    QPushButton *m_okButton; //选择按钮
    QIcon m_dummyIcon; //具体wizard的默认图标
    QList<QStandardItem*> m_categoryItems; //存放所有的类别
};

} // namespace Internal
} // namespace Core

#endif // NEWDIALOG_H
