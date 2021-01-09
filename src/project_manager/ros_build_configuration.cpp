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
#include "ros_build_configuration.h"

#include "ros_catkin_make_step.h"
#include "ros_catkin_tools_step.h"
#include "ros_colcon_step.h"
#include "ros_project.h"
#include "ros_project_constants.h"
#include "ros_utils.h"
#include "ui_ros_build_configuration.h"

#include <coreplugin/icore.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QFormLayout>

using namespace ProjectExplorer;

namespace ROSProjectManager {
namespace Internal {

class ROSExtraBuildInfo
{
public:
    ROSUtils::BuildSystem buildSystem;
    ROSUtils::BuildType cmakeBuildType;
};

} // namespace Internal
} // namespace ROSProjectManager

Q_DECLARE_METATYPE(ROSProjectManager::Internal::ROSExtraBuildInfo)

namespace ROSProjectManager {
namespace Internal {

const char ROS_BC_ID[] = "ROSProjectManager.ROSBuildConfiguration";
const char ROS_BC_BUILD_SYSTEM[] = "ROSProjectManager.ROSBuildConfiguration.BuildSystem";
const char ROS_BC_CMAKE_BUILD_TYPE[] = "ROSProjectManager.ROSBuildConfiguration.CMakeBuildType";

ROSBuildConfiguration::ROSBuildConfiguration(Target *parent, Core::Id id)
    : BuildConfiguration(parent, id)
{
    setInitializer(std::bind(&ROSBuildConfiguration::initialize, this, std::placeholders::_1));
}

void ROSBuildConfiguration::initialize(const ProjectExplorer::BuildInfo &info)
{
    auto extraInfo = info.extraInfo.value<ROSExtraBuildInfo>();

    setDisplayName(info.displayName);
    setDefaultDisplayName(info.displayName);
    setBuildDirectory(info.buildDirectory);
    setBuildSystem(extraInfo.buildSystem);
    setCMakeBuildType(extraInfo.cmakeBuildType);

    BuildStepList *bs = buildSteps();
    BuildStepList *cs = cleanSteps();
    Q_ASSERT(bs);
    Q_ASSERT(cs);

    bs->appendStep(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    cs->appendStep(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);

    switch (extraInfo.buildSystem)
    {
        case ROSUtils::CatkinMake:
        {
            ROSCatkinMakeStep *makeStep = new ROSCatkinMakeStep(bs, ROS_CMS_ID);
            bs->insertStep(0, makeStep);
            makeStep->setBuildTarget(ROSCatkinMakeStep::BUILD);

            ROSCatkinMakeStep *cleanMakeStep = new ROSCatkinMakeStep(cs, ROS_CMS_ID);
            cs->insertStep(0, cleanMakeStep);
            cleanMakeStep->setBuildTarget(ROSCatkinMakeStep::CLEAN);
            break;
        }
        case ROSUtils::CatkinTools:
        {
            ROSCatkinToolsStep *makeStep = new ROSCatkinToolsStep(bs, ROS_CTS_ID);
            bs->insertStep(0, makeStep);
            makeStep->setBuildTarget(ROSCatkinToolsStep::BUILD);

            ROSCatkinToolsStep *cleanMakeStep = new ROSCatkinToolsStep(cs, ROS_CTS_ID);
            cs->insertStep(0, cleanMakeStep);
            cleanMakeStep->setBuildTarget(ROSCatkinToolsStep::CLEAN);
            break;
        }
        case ROSUtils::Colcon:
        {
            ROSColconStep *makeStep = new ROSColconStep(bs, ROS_COLCON_STEP_ID);
            bs->insertStep(0, makeStep);
            makeStep->setBuildTarget(ROSColconStep::BUILD);

            ROSColconStep *cleanMakeStep = new ROSColconStep(cs, ROS_COLCON_STEP_ID);
            cs->insertStep(0, cleanMakeStep);
            cleanMakeStep->setBuildTarget(ROSColconStep::CLEAN);
            break;
        }
    }
}

QVariantMap ROSBuildConfiguration::toMap() const
{
  QVariantMap map(BuildConfiguration::toMap());

  map.insert(QLatin1String(ROS_BC_BUILD_SYSTEM), (int)m_buildSystem);
  map.insert(QLatin1String(ROS_BC_CMAKE_BUILD_TYPE), (int)m_cmakeBuildType);
  return map;
}

bool ROSBuildConfiguration::fromMap(const QVariantMap &map)
{
  m_buildSystem = (ROSUtils::BuildSystem)map.value(QLatin1String(ROS_BC_BUILD_SYSTEM)).toInt();
  m_cmakeBuildType = (ROSUtils::BuildType)map.value(QLatin1String(ROS_BC_CMAKE_BUILD_TYPE)).toInt();
  return BuildConfiguration::fromMap(map);
}

ROSUtils::BuildSystem ROSBuildConfiguration::rosBuildSystem() const
{
    return m_buildSystem;
}

void ROSBuildConfiguration::setBuildSystem(const ROSUtils::BuildSystem &buildSystem)
{
    m_buildSystem = buildSystem;
    emit buildSystemChanged(buildSystem);
}

ROSUtils::BuildType ROSBuildConfiguration::cmakeBuildType() const
{
    return m_cmakeBuildType;
}

void ROSBuildConfiguration::setCMakeBuildType(const ROSUtils::BuildType &buildType)
{
    m_cmakeBuildType = buildType;
    emit cmakeBuildTypeChanged(buildType);
}

ROSProject *ROSBuildConfiguration::project()
{
    return qobject_cast<ROSProject *>(target()->project());
}

void ROSBuildConfiguration::updateQtEnvironment(const Utils::Environment &env)
{
    const Utils::NameValueItems diff = baseEnvironment().diff(env);
    if (!diff.isEmpty())
      setUserEnvironmentChanges(diff);
}

NamedWidget *ROSBuildConfiguration::createConfigWidget()
{
    return new ROSBuildSettingsWidget(this);
}

QList<NamedWidget *> ROSBuildConfiguration::createSubConfigWidgets()
{
  return QList<NamedWidget *>() << new ROSBuildEnvironmentWidget(this);
}

/*!
  \class ROSBuildConfigurationFactory
*/

ROSBuildConfigurationFactory::ROSBuildConfigurationFactory() :
    BuildConfigurationFactory()
{
    registerBuildConfiguration<ROSBuildConfiguration>(ROS_BC_ID);

    setSupportedProjectType(Constants::ROS_PROJECT_ID);
    setSupportedProjectMimeTypeName(Constants::ROS_MIME_TYPE);

    setBuildGenerator(std::bind(&ROSBuildConfigurationFactory::availableBuilds, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

ROSBuildConfigurationFactory::~ROSBuildConfigurationFactory()
{
}

QList<BuildInfo> ROSBuildConfigurationFactory::availableBuilds(const Kit *k,
                                                               const Utils::FilePath &projectPath,
                                                               bool forSetup) const
{
    Q_UNUSED(forSetup);

    QList<BuildInfo> result;

    // Need to create a ROS Setting widget where the user sets the default build system to use here.
    ROSUtils::ROSProjectFileContent projectFileContent;
    ROSUtils::parseQtCreatorWorkspaceFile(projectPath, projectFileContent);

    for (int type = ROSUtils::BuildTypeDebug; type <= ROSUtils::BuildTypeUserDefined; ++type) {
      ProjectExplorer::BuildInfo info = createBuildInfo(k, projectFileContent.defaultBuildSystem, ROSUtils::BuildType(type));
      result << info;
    }

    //TO DO: Should probably check if the directory that was selected was the workspace
    return result;
}

ProjectExplorer::BuildInfo ROSBuildConfigurationFactory::createBuildInfo(const Kit *k, const ROSUtils::BuildSystem &build_system, const ROSUtils::BuildType &type) const
{
    ProjectExplorer::BuildInfo info;
    info.kitId = k->id();
    info.typeName = ROSUtils::buildTypeName(type);
    info.displayName = info.typeName;

    switch (type) {
    case ROSUtils::BuildTypeDebug:
        info.buildType = BuildConfiguration::Debug;
        break;
    case ROSUtils::BuildTypeMinSizeRel:
        info.buildType = BuildConfiguration::Release;
        break;
    case ROSUtils::BuildTypeRelWithDebInfo:
        info.buildType = BuildConfiguration::Profile;
        break;
    case ROSUtils::BuildTypeRelease:
        info.buildType = BuildConfiguration::Release;
        break;
    default:
        info.buildType = BuildConfiguration::Unknown;
        break;
    }

    ROSExtraBuildInfo extra;
    extra.buildSystem = build_system;
    extra.cmakeBuildType = type;
    info.extraInfo = QVariant::fromValue(extra);

    return info;
}

BuildConfiguration::BuildType ROSBuildConfiguration::buildType() const
{
    switch (m_cmakeBuildType) {
    case ROSUtils::BuildTypeDebug:
        return Debug;
    case ROSUtils::BuildTypeRelWithDebInfo:
        return Profile;
    case ROSUtils::BuildTypeMinSizeRel:
        return Release;
    case ROSUtils::BuildTypeRelease:
        return Release;
    default:
        return Unknown;
    }
}

////////////////////////////////////////////////////////////////////////////////////
// ROSBuildSettingsWidget
////////////////////////////////////////////////////////////////////////////////////

ROSBuildSettingsWidget::ROSBuildSettingsWidget(ROSBuildConfiguration *bc)
    : NamedWidget(tr("ROS Manager")),
      m_buildConfiguration(bc)
{
    m_ui = new Ui::ROSBuildConfiguration;
    m_ui->setupUi(this);
    m_ui->buildSystemComboBox->setCurrentIndex(bc->rosBuildSystem());
    m_ui->buildTypeComboBox->setCurrentIndex(bc->cmakeBuildType());

    connect(m_ui->buildSystemComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(buildSystemChanged(int)));

    connect(m_ui->buildTypeComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(buildTypeChanged(int)));

    connect(m_ui->buildSourceWorkspaceButton, SIGNAL(clicked()),
            this, SLOT(buildSourceWorkspaceButtonClicked()));
}

ROSBuildSettingsWidget::~ROSBuildSettingsWidget()
{
  delete m_ui;
}

void ROSBuildSettingsWidget::buildSystemChanged(int index)
{
    m_buildConfiguration->setBuildSystem(((ROSUtils::BuildSystem)index));
}

void ROSBuildSettingsWidget::buildTypeChanged(int index)
{
    m_buildConfiguration->setCMakeBuildType(((ROSUtils::BuildType)index));
}

////////////////////////////////////////////////////////////////////////////////////
// ROSBuildEnvironmentWidget
////////////////////////////////////////////////////////////////////////////////////

ROSBuildEnvironmentWidget::ROSBuildEnvironmentWidget(BuildConfiguration *bc)
    : NamedWidget(tr("Build Environment"))
{
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(0);
    m_clearSystemEnvironmentCheckBox = new QCheckBox(this);
    m_clearSystemEnvironmentCheckBox->setText(tr("Clear system environment"));

    m_buildEnvironmentWidget = new EnvironmentWidget(this, ProjectExplorer::EnvironmentWidget::TypeLocal, m_clearSystemEnvironmentCheckBox);
    vbox->addWidget(m_buildEnvironmentWidget);

    connect(m_buildEnvironmentWidget, SIGNAL(userChangesChanged()),
            this, SLOT(environmentModelUserChangesChanged()));
    connect(m_clearSystemEnvironmentCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(clearSystemEnvironmentCheckBoxClicked(bool)));

    m_buildConfiguration = bc;

    connect(m_buildConfiguration, &BuildConfiguration::environmentChanged,
            this, &ROSBuildEnvironmentWidget::environmentChanged);

    m_clearSystemEnvironmentCheckBox->setChecked(!m_buildConfiguration->useSystemEnvironment());
    m_buildEnvironmentWidget->setBaseEnvironment(m_buildConfiguration->baseEnvironment());
    m_buildEnvironmentWidget->setBaseEnvironmentText(m_buildConfiguration->baseEnvironmentText());
    m_buildEnvironmentWidget->setUserChanges(m_buildConfiguration->userEnvironmentChanges());
}

void ROSBuildEnvironmentWidget::environmentModelUserChangesChanged()
{
    m_buildConfiguration->setUserEnvironmentChanges(m_buildEnvironmentWidget->userChanges());
}

void ROSBuildEnvironmentWidget::clearSystemEnvironmentCheckBoxClicked(bool checked)
{
    m_buildConfiguration->setUseSystemEnvironment(!checked);
    m_buildEnvironmentWidget->setBaseEnvironment(m_buildConfiguration->baseEnvironment());
    m_buildEnvironmentWidget->setBaseEnvironmentText(m_buildConfiguration->baseEnvironmentText());
}

void ROSBuildEnvironmentWidget::environmentChanged()
{
    m_buildEnvironmentWidget->setBaseEnvironment(m_buildConfiguration->baseEnvironment());
    m_buildEnvironmentWidget->setBaseEnvironmentText(m_buildConfiguration->baseEnvironmentText());
    m_buildEnvironmentWidget->setUserChanges(m_buildConfiguration->userEnvironmentChanges());
}

void ROSBuildSettingsWidget::buildSourceWorkspaceButtonClicked()
{
  ROSUtils::WorkspaceInfo workspaceInfo = ROSUtils::getWorkspaceInfo(m_buildConfiguration->project()->projectDirectory(),
                                                                     m_buildConfiguration->rosBuildSystem(),
                                                                     m_buildConfiguration->project()->distribution());

  Utils::Environment env(ROSUtils::getWorkspaceEnvironment(workspaceInfo, m_buildConfiguration->environment()).toStringList());

  m_buildConfiguration->updateQtEnvironment(env);
}

} // namespace Internal
} // namespace GenericProjectManager


