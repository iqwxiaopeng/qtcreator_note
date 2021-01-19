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

#ifndef IWIZARD_H
#define IWIZARD_H

#include <coreplugin/core_global.h>
#include <coreplugin/id.h>
#include <coreplugin/featureprovider.h>

#include <QIcon>
#include <QObject>
#include <QVariantMap>

namespace Core {

	//所有引导类的基类(引导类：当你选择new C++控制台工程与gui工程的时候，引导肯定是不同的，因此这里只是给定的引导基类，实现交给具体各自的继承体)
class CORE_EXPORT IWizard
    : public QObject
{
    Q_OBJECT
public:
	//引导种类定义
    enum WizardKind {
        FileWizard = 0x01, //文件引导
        ClassWizard = 0x02, //类引导
        ProjectWizard = 0x04 //工程引导
    };
    Q_DECLARE_FLAGS(WizardKinds, WizardKind)
    enum WizardFlag {
        PlatformIndependent = 0x01,
        ForceCapitalLetterForFileName = 0x02
    };
    Q_DECLARE_FLAGS(WizardFlags, WizardFlag)

		//引导包含的数据
    class CORE_EXPORT Data
    {
    public:
        Data() : kind(IWizard::FileWizard) {}

        IWizard::WizardKind kind; //引导类型
        QIcon icon; //引导窗体的图标
        QString description; //引导的详细描述
        QString displayName; //引导概述标题类
        QString id; //引导的id为了排序
        QString category; //引导的类别
        QString displayCategory; //引导的显示类别
        FeatureSet requiredFeatures;
        IWizard::WizardFlags flags;
        QString descriptionImage;
    };

    IWizard(QObject *parent = 0) : QObject(parent) {}
    ~IWizard() {}

    QString id() const { return m_data.id; }
    WizardKind kind() const { return m_data.kind; }
    QIcon icon() const { return m_data.icon; }
    QString description() const { return m_data.description; }
    QString displayName() const { return m_data.displayName; }
    QString category() const { return m_data.category; }
    QString displayCategory() const { return m_data.displayCategory; }
    QString descriptionImage() const { return m_data.descriptionImage; }
    FeatureSet requiredFeatures() const { return m_data.requiredFeatures; }
    WizardFlags flags() const { return m_data.flags; }

    void setData(const Data &data) { m_data = data; }
    void setId(const QString &id) { m_data.id = id; }
    void setWizardKind(WizardKind kind) { m_data.kind = kind; }
    void setIcon(const QIcon &icon) { m_data.icon = icon; }
    void setDescription(const QString &description) { m_data.description = description; }
    void setDisplayName(const QString &displayName) { m_data.displayName = displayName; }
    void setCategory(const QString &category) { m_data.category = category; }
    void setDisplayCategory(const QString &displayCategory) { m_data.displayCategory = displayCategory; }
    void setDescriptionImage(const QString &descriptionImage) { m_data.descriptionImage = descriptionImage; }
    void setRequiredFeatures(const FeatureSet &featureSet) { m_data.requiredFeatures = featureSet; }
    void addRequiredFeature(const Feature &feature) { m_data.requiredFeatures |= feature; }
    void setFlags(WizardFlags flags) { m_data.flags = flags; }

	//运行引导
    virtual void runWizard(const QString &path, QWidget *parent, const QString &platform, const QVariantMap &variables) = 0;

    bool isAvailable(const QString &platformName) const;//判断是否支持某模版
    QStringList supportedPlatforms() const; //获取wizard所支持的模版,返回模版代号同allAvailablePlatforms()而非显示名

    // Utility to find all registered wizards
    static QList<IWizard*> allWizards(); //获取所有注册到管理器的wizard对象
    // Utility to find all registered wizards of a certain kind
    static QList<IWizard*> wizardsOfKind(WizardKind kind);//获取所有注册到管理器的kind类型的wizard对象
    static QStringList allAvailablePlatforms();//获取所有注册到管理器的wizard模版(见iwizard图片中3号)
    static QString displayNameForPlatform(const QString &string);//获取模版的显示名，参数是 allAvailablePlatforms 列表中的条目

private:
    Data m_data;
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::IWizard::WizardKinds)
Q_DECLARE_OPERATORS_FOR_FLAGS(Core::IWizard::WizardFlags)

#endif // IWIZARD_H
