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

#ifndef BUILDCONFIGURATION_H
#define BUILDCONFIGURATION_H

#include "projectexplorer_export.h"
#include "projectconfiguration.h"

#include <utils/environment.h>
#include <utils/fileutils.h>

namespace Utils { class AbstractMacroExpander; }

namespace ProjectExplorer {

class BuildConfiguration;
class BuildInfo;
class NamedWidget;
class BuildStepList;
class Kit;
class Target;
class IOutputParser;

class PROJECTEXPLORER_EXPORT BuildConfiguration : public ProjectConfiguration
{
    Q_OBJECT

public:
    // ctors are protected
    virtual ~BuildConfiguration();

    Utils::FileName buildDirectory() const;
    Utils::FileName rawBuildDirectory() const;

    virtual ProjectExplorer::NamedWidget *createConfigWidget() = 0;
    virtual QList<NamedWidget *> createSubConfigWidgets();

    // Maybe the BuildConfiguration is not the best place for the environment
    Utils::Environment baseEnvironment() const;
    QString baseEnvironmentText() const;
    Utils::Environment environment() const;
    void setUserEnvironmentChanges(const QList<Utils::EnvironmentItem> &diff);
    QList<Utils::EnvironmentItem> userEnvironmentChanges() const;
    bool useSystemEnvironment() const;
    void setUseSystemEnvironment(bool b);

    QList<Core::Id> knownStepLists() const;
    BuildStepList *stepList(Core::Id id) const;

    virtual bool fromMap(const QVariantMap &map);
    virtual QVariantMap toMap() const;

    Target *target() const;

    virtual bool isEnabled() const;
    virtual QString disabledReason() const;

    Utils::AbstractMacroExpander *macroExpander();

    enum BuildType {
        Unknown,
        Debug,
        Release
    };
    virtual BuildType buildType() const = 0;

signals:
    void environmentChanged();
    void buildDirectoryChanged();
    void enabledChanged();

protected:
    BuildConfiguration(Target *target, const Core::Id id);
    BuildConfiguration(Target *target, BuildConfiguration *source);

    virtual void setBuildDirectory(const Utils::FileName &dir);
    void cloneSteps(BuildConfiguration *source);

private slots:
    void handleKitUpdate();
    void emitBuildDirectoryChanged();

private:
    void emitEnvironmentChanged();

    bool m_clearSystemEnvironment;
    QList<Utils::EnvironmentItem> m_userEnvironmentChanges;
    QList<BuildStepList *> m_stepLists;
    Utils::AbstractMacroExpander *m_macroExpander;
    Utils::FileName m_buildDirectory;
    Utils::FileName m_lastEmmitedBuildDirectory;
    mutable Utils::Environment m_cachedEnvironment;
};

class PROJECTEXPLORER_EXPORT IBuildConfigurationFactory :
    public QObject
{
    Q_OBJECT

public:
    explicit IBuildConfigurationFactory(QObject *parent = 0);
    virtual ~IBuildConfigurationFactory();

    // The priority is negative if this factory can not create anything for the target.
    // It is 0 for the "default" factory that wants to handle the target.
    // Add 100 for each specialization.
    virtual int priority(const Target *parent) const = 0;
    // List of build information that can be used to create a new build configuration via
    // "Add Build Configuration" button.
    virtual QList<BuildInfo *> availableBuilds(const Target *parent) const = 0;

    virtual int priority(const Kit *k, const QString &projectPath) const = 0;
    // List of build information that can be used to initially set up a new build configuration.
    virtual QList<BuildInfo *> availableSetups(const Kit *k, const QString &projectPath) const = 0;

    virtual BuildConfiguration *create(Target *parent, const BuildInfo *info) const = 0;

    // used to recreate the runConfigurations when restoring settings
    virtual bool canRestore(const Target *parent, const QVariantMap &map) const = 0;
    virtual BuildConfiguration *restore(Target *parent, const QVariantMap &map) = 0;
    virtual bool canClone(const Target *parent, BuildConfiguration *product) const = 0;
    virtual BuildConfiguration *clone(Target *parent, BuildConfiguration *product) = 0;

    static IBuildConfigurationFactory *find(Target *parent, const QVariantMap &map);
    static IBuildConfigurationFactory *find(Kit *k, const QString &projectPath);
    static IBuildConfigurationFactory *find(Target *parent);
    static IBuildConfigurationFactory *find(Target *parent, BuildConfiguration *bc);

signals:
    void availableCreationIdsChanged();
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::BuildConfiguration *)

#endif // BUILDCONFIGURATION_H