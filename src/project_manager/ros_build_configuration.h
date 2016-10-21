/**
 * @author Levi Armstrong
 * @date January 1, 2016
 *
 * @copyright Copyright (c) 2016, Southwest Research Institute
 *
 * @license Software License Agreement (Apache License)\n
 * \n
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at\n
 * \n
 * http://www.apache.org/licenses/LICENSE-2.0\n
 * \n
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef ROSBUILDCONFIGURATION_H
#define ROSBUILDCONFIGURATION_H

#include "ros_utils.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/namedwidget.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/environmentwidget.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>
#include <QCheckBox>
#include <QProcess>
#include <QMenu>

namespace Utils {
class FileName;
class PathChooser;
} // namespace Utils

namespace ROSProjectManager {
namespace Internal {

class ROSBuildConfigurationFactory;
class ROSBuildSettingsWidget;
class ROSBuildInfo;
namespace Ui { class ROSBuildConfiguration; }

class ROSBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT
    friend class ROSBuildConfigurationFactory;

public:
    explicit ROSBuildConfiguration(ProjectExplorer::Target *parent);

    ProjectExplorer::NamedWidget *createConfigWidget() override;
    QList<ProjectExplorer::NamedWidget *> createSubConfigWidgets() override;

    BuildType buildType() const;

    QVariantMap toMap() const override;

    ROSUtils::BuildSystem buildSystem() const;
    void setBuildSystem(ROSUtils::BuildSystem &build_system);

    ROSUtils::BuildType cmakeBuildType() const;
    void setCMakeBuildType(ROSUtils::BuildType &buildType);

    void sourceWorkspace();

protected:
    ROSBuildConfiguration(ProjectExplorer::Target *parent, ROSBuildConfiguration *source);
    ROSBuildConfiguration(ProjectExplorer::Target *parent, Core::Id id);

    bool fromMap(const QVariantMap &map) override;

    friend class ROSBuildSettingsWidget;

private:
    ROSUtils::BuildSystem m_buildSystem;
    ROSUtils::BuildType m_cmakeBuildType;
    ProjectExplorer::NamedWidget *m_buildEnvironmentWidget;

};

class ROSBuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
{
    Q_OBJECT

public:
    explicit ROSBuildConfigurationFactory(QObject *parent = 0);
    ~ROSBuildConfigurationFactory();

    int priority(const ProjectExplorer::Target *parent) const;
    QList<ProjectExplorer::BuildInfo *> availableBuilds(const ProjectExplorer::Target *parent) const;
    int priority(const ProjectExplorer::Kit *k, const QString &projectPath) const;
    QList<ProjectExplorer::BuildInfo *> availableSetups(const ProjectExplorer::Kit *k,
                                                        const QString &projectPath) const;
    ProjectExplorer::BuildConfiguration *create(ProjectExplorer::Target *parent,
                                                const ProjectExplorer::BuildInfo *info) const;

    bool canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const;
    ProjectExplorer::BuildConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source);
    bool canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);
private:
    bool canHandle(const ProjectExplorer::Target *t) const;

    ROSBuildInfo *createBuildInfo(const ProjectExplorer::Kit *k, const ROSUtils::BuildSystem &build_system, const ROSUtils::BuildType &type) const;
};

class ROSBuildSettingsWidget : public ProjectExplorer::NamedWidget
{
    Q_OBJECT

public:
    ROSBuildSettingsWidget(ROSBuildConfiguration *bc);
    ~ROSBuildSettingsWidget();

private slots:
    void buildSystemChanged(int index);

private:
    Ui::ROSBuildConfiguration *m_ui;
    ROSBuildConfiguration *m_buildConfiguration;
};

class ROSBuildEnvironmentWidget : public ProjectExplorer::NamedWidget
{
  Q_OBJECT

public:
  ROSBuildEnvironmentWidget(ProjectExplorer::BuildConfiguration *bc);

private slots:
  void environmentModelUserChangesChanged();
  void clearSystemEnvironmentCheckBoxClicked(bool checked);
  void environmentChanged();

protected:
  ProjectExplorer::EnvironmentWidget *m_buildEnvironmentWidget;
  QCheckBox *m_clearSystemEnvironmentCheckBox;
  ProjectExplorer::BuildConfiguration *m_buildConfiguration;
};

class ROSBuildInfo : public ProjectExplorer::BuildInfo
{
public:
    ROSBuildInfo(const ProjectExplorer::IBuildConfigurationFactory *f) :
        ProjectExplorer::BuildInfo(f) { }

    ROSBuildInfo(const Internal::ROSBuildConfiguration *bc) :
        ProjectExplorer::BuildInfo(ProjectExplorer::IBuildConfigurationFactory::find(bc->target()))
    {
        displayName = bc->displayName();
        buildDirectory = bc->buildDirectory();
        kitId = bc->target()->kit()->id();
        environment = bc->environment();
        buildSystem = bc->buildSystem();
        cmakeBuildType = bc->cmakeBuildType();
    }

    Utils::Environment environment;
    ROSUtils::BuildSystem buildSystem;
    ROSUtils::BuildType cmakeBuildType;
};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSBUILDCONFIGURATION_H
