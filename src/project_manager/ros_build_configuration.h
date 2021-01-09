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
#include "ros_project.h"

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
class PathChooser;
} // namespace Utils

namespace ROSProjectManager {
namespace Internal {

class ROSBuildConfigurationFactory;
class ROSBuildSettingsWidget;
class ROSExtraBuildInfo;
namespace Ui { class ROSBuildConfiguration; }

class ROSBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT
    friend class ROSBuildConfigurationFactory;

public:
    ROSBuildConfiguration(ProjectExplorer::Target *parent, Core::Id id);

    void initialize(const ProjectExplorer::BuildInfo &info);

    ProjectExplorer::NamedWidget *createConfigWidget() override;
    QList<ProjectExplorer::NamedWidget *> createSubConfigWidgets() override;

    BuildType buildType() const override;

    QVariantMap toMap() const override;

    ROSUtils::BuildSystem rosBuildSystem() const;
    void setBuildSystem(const ROSUtils::BuildSystem &buildSystem);

    ROSUtils::BuildType cmakeBuildType() const;
    void setCMakeBuildType(const ROSUtils::BuildType &buildType);

    void updateQtEnvironment(const Utils::Environment &env);

    ROSProject *project();

signals:
    void buildSystemChanged(const ROSUtils::BuildSystem &buildSystem);
    void cmakeBuildTypeChanged(const ROSUtils::BuildType &buildType);

protected:
    bool fromMap(const QVariantMap &map) override;

    friend class ROSBuildSettingsWidget;

private:
    ROSUtils::BuildSystem m_buildSystem;
    ROSUtils::BuildType m_cmakeBuildType;
    ProjectExplorer::NamedWidget *m_buildEnvironmentWidget;

};

class ROSBuildConfigurationFactory : public ProjectExplorer::BuildConfigurationFactory
{
    Q_OBJECT

public:
    explicit ROSBuildConfigurationFactory();
    ~ROSBuildConfigurationFactory();

    QList<ProjectExplorer::BuildInfo> availableBuilds(const ProjectExplorer::Kit *k,
                                                      const Utils::FilePath &projectPath,
                                                      bool forSetup) const override;

private:
    ProjectExplorer::BuildInfo createBuildInfo(const ProjectExplorer::Kit *k, const ROSUtils::BuildSystem &build_system, const ROSUtils::BuildType &type) const;
};

class ROSBuildSettingsWidget : public ProjectExplorer::NamedWidget
{
    Q_OBJECT

public:
    ROSBuildSettingsWidget(ROSBuildConfiguration *bc);
    ~ROSBuildSettingsWidget();

private slots:
    void buildSystemChanged(int index);
    void buildTypeChanged(int index);
    void buildSourceWorkspaceButtonClicked();

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

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSBUILDCONFIGURATION_H
