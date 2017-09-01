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

const char ROS_BC_ID[] = "ROSProjectManager.ROSBuildConfiguration";
const char ROS_BC_BUILD_SYSTEM[] = "ROSProjectManager.ROSBuildConfiguration.BuildSystem";
const char ROS_BC_CMAKE_BUILD_TYPE[] = "ROSProjectManager.ROSBuildConfiguration.CMakeBuildType";

ROSBuildConfiguration::ROSBuildConfiguration(Target *parent)
    : BuildConfiguration(parent, Core::Id(ROS_BC_ID))
{
}

ROSBuildConfiguration::ROSBuildConfiguration(Target *parent, Core::Id id)
    : BuildConfiguration(parent, id)
{
}

ROSBuildConfiguration::ROSBuildConfiguration(Target *parent, ROSBuildConfiguration *source) :
    BuildConfiguration(parent, source),
    m_buildSystem(source->m_buildSystem),
    m_cmakeBuildType(source->m_cmakeBuildType)
{
    cloneSteps(source);
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

ROSUtils::BuildSystem ROSBuildConfiguration::buildSystem() const
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
    QList<Utils::EnvironmentItem> diff = baseEnvironment().diff(env);
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

ROSBuildConfigurationFactory::ROSBuildConfigurationFactory(QObject *parent) :
    IBuildConfigurationFactory(parent)
{
}

ROSBuildConfigurationFactory::~ROSBuildConfigurationFactory()
{
}

int ROSBuildConfigurationFactory::priority(const Target *parent) const
{
    return canHandle(parent) ? 0 : -1;
}

QList<BuildInfo *> ROSBuildConfigurationFactory::availableBuilds(const Target *parent) const
{
    QList<BuildInfo *> result;

    // Need to create a ROS Setting widget where the user sets the default build system to use here.
    ROSUtils::BuildSystem buildSystem = static_cast<ROSProject *>(parent->project())->defaultBuildSystem();

    for (int type = ROSUtils::BuildTypeDebug; type <= ROSUtils::BuildTypeUserDefined; ++type)
    {
      ROSBuildInfo *info = createBuildInfo(parent->kit(), buildSystem, ROSUtils::BuildType(type));
      result << info;
    }

    return result;
}

int ROSBuildConfigurationFactory::priority(const Kit *k, const QString &projectPath) const
{
#if QT_CREATOR_VER < QT_CREATOR_VER_CHECK(4,3,0)
    Utils::MimeDatabase mdb;

    if (k && mdb.mimeTypeForFile(projectPath).matchesName(QLatin1String(Constants::ROS_MIME_TYPE)))
        return 0;
#else
    if (k && Utils::mimeTypeForFile(projectPath).matchesName(QLatin1String(Constants::ROS_MIME_TYPE) ))
        return 0;
#endif

    return -1;
}

QList<BuildInfo *> ROSBuildConfigurationFactory::availableSetups(const Kit *k, const QString &projectPath) const
{
    QList<BuildInfo *> result;

    // Need to create a ROS Setting widget where the user sets the default build system to use here.
    ROSUtils::ROSProjectFileContent projectFileContent;
    ROSUtils::parseQtCreatorWorkspaceFile(Utils::FileName::fromString(projectPath), projectFileContent);

    for (int type = ROSUtils::BuildTypeDebug; type <= ROSUtils::BuildTypeUserDefined; ++type) {
      ROSBuildInfo *info = createBuildInfo(k, projectFileContent.defaultBuildSystem, ROSUtils::BuildType(type));
      result << info;
    }

    //TO DO: Should probably check if the directory that was selected was the workspace
    return result;
}

BuildConfiguration *ROSBuildConfigurationFactory::create(Target *parent, const BuildInfo *info) const
{
    QTC_ASSERT(info->factory() == this, return 0);
    QTC_ASSERT(info->kitId == parent->kit()->id(), return 0);
    QTC_ASSERT(!info->displayName.isEmpty(), return 0);

    ROSBuildInfo ros_info(*static_cast<const ROSBuildInfo *>(info));
    ROSBuildConfiguration *bc = new ROSBuildConfiguration(parent);

    bc->setDisplayName(ros_info.displayName);
    bc->setDefaultDisplayName(ros_info.displayName);
    bc->setBuildDirectory(ros_info.buildDirectory);
    bc->setBuildSystem(ros_info.buildSystem);
    bc->setCMakeBuildType(ros_info.cmakeBuildType);

    BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
    Q_ASSERT(buildSteps);
    Q_ASSERT(cleanSteps);

    switch (bc->buildSystem())
    {
    case ROSUtils::CatkinMake:
    {
        ROSCatkinMakeStep *makeStep = new ROSCatkinMakeStep(buildSteps);
        buildSteps->insertStep(0, makeStep);
        makeStep->setBuildTarget(ROSCatkinMakeStep::BUILD);

        ROSCatkinMakeStep *cleanMakeStep = new ROSCatkinMakeStep(cleanSteps);
        cleanSteps->insertStep(0, cleanMakeStep);
        cleanMakeStep->setBuildTarget(ROSCatkinMakeStep::CLEAN);
        break;
    }
    case ROSUtils::CatkinTools:
    {
        ROSCatkinToolsStep *makeStep = new ROSCatkinToolsStep(buildSteps);
        buildSteps->insertStep(0, makeStep);
        makeStep->setBuildTarget(ROSCatkinToolsStep::BUILD);

        ROSCatkinToolsStep *cleanMakeStep = new ROSCatkinToolsStep(cleanSteps);
        cleanSteps->insertStep(0, cleanMakeStep);
        cleanMakeStep->setBuildTarget(ROSCatkinToolsStep::CLEAN);
        break;
    }
    }

    return bc;
}

bool ROSBuildConfigurationFactory::canClone(const Target *parent, BuildConfiguration *source) const
{
    if (!canHandle(parent))
        return false;
    return source->id() == ROS_BC_ID;
}

BuildConfiguration *ROSBuildConfigurationFactory::clone(Target *parent, BuildConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    return new ROSBuildConfiguration(parent, qobject_cast<ROSBuildConfiguration *>(source));
}

bool ROSBuildConfigurationFactory::canRestore(const Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    return ProjectExplorer::idFromMap(map) == ROS_BC_ID;
}

BuildConfiguration *ROSBuildConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    ROSBuildConfiguration *bc(new ROSBuildConfiguration(parent));
    if (bc->fromMap(map))
        return bc;
    delete bc;
    return 0;
}

bool ROSBuildConfigurationFactory::canHandle(const Target *t) const
{
    if (!t->project()->supportsKit(t->kit()))
        return false;
    return qobject_cast<ROSProject *>(t->project());
}

ROSBuildInfo *ROSBuildConfigurationFactory::createBuildInfo(const Kit *k, const ROSUtils::BuildSystem &build_system, const ROSUtils::BuildType &type) const
{
    ROSBuildInfo *info = new ROSBuildInfo(this);
    info->kitId = k->id();
    info->buildSystem = build_system;
    info->cmakeBuildType = type;
    info->typeName = ROSUtils::buildTypeName(type);
    info->displayName = info->typeName;

    switch (type) {
    case ROSUtils::BuildTypeDebug:
        info->buildType = BuildConfiguration::Debug;
        break;
    case ROSUtils::BuildTypeMinSizeRel:
        info->buildType = BuildConfiguration::Release;
        break;
    case ROSUtils::BuildTypeRelWithDebInfo:
        info->buildType = BuildConfiguration::Profile;
        break;
    case ROSUtils::BuildTypeRelease:
        info->buildType = BuildConfiguration::Release;
        break;
    default:
        info->buildType = BuildConfiguration::Unknown;
        break;
    }

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
    : m_buildConfiguration(bc)
{
    m_ui = new Ui::ROSBuildConfiguration;
    m_ui->setupUi(this);
    m_ui->buildSystemComboBox->setCurrentIndex(bc->buildSystem());
    m_ui->buildTypeComboBox->setCurrentIndex(bc->cmakeBuildType());

    connect(m_ui->buildSystemComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(buildSystemChanged(int)));

    connect(m_ui->buildTypeComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(buildTypeChanged(int)));

    setDisplayName(tr("ROS Manager"));
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
{
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(0);
    m_clearSystemEnvironmentCheckBox = new QCheckBox(this);
    m_clearSystemEnvironmentCheckBox->setText(tr("Clear system environment"));

    m_buildEnvironmentWidget = new EnvironmentWidget(this, m_clearSystemEnvironmentCheckBox);
    vbox->addWidget(m_buildEnvironmentWidget);

    connect(m_buildEnvironmentWidget, SIGNAL(userChangesChanged()),
            this, SLOT(environmentModelUserChangesChanged()));
    connect(m_clearSystemEnvironmentCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(clearSystemEnvironmentCheckBoxClicked(bool)));

    m_buildConfiguration = bc;

    connect(m_buildConfiguration->target(), SIGNAL(environmentChanged()),
            this, SLOT(environmentChanged()));

    m_clearSystemEnvironmentCheckBox->setChecked(!m_buildConfiguration->useSystemEnvironment());
    m_buildEnvironmentWidget->setBaseEnvironment(m_buildConfiguration->baseEnvironment());
    m_buildEnvironmentWidget->setBaseEnvironmentText(m_buildConfiguration->baseEnvironmentText());
    m_buildEnvironmentWidget->setUserChanges(m_buildConfiguration->userEnvironmentChanges());

    setDisplayName(tr("Build Environment"));
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

} // namespace Internal
} // namespace GenericProjectManager


