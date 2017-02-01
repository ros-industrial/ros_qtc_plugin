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
#include "ros_run_steps.h"
#include "ros_project_constants.h"
#include "ros_project.h"
#include "ui_ros_generic_configuration.h"
#include "ros_build_configuration.h"
#include "ros_utils.h"

#include <projectexplorer/project.h>
#include <projectexplorer/buildmanager.h>
#include <qtermwidget5/qtermwidget.h>

namespace ROSProjectManager {
namespace Internal {

const char ROS_LAUNCH_ID[] = "ROSProjectManager.ROSLaunchStep";
const char ROS_RUN_ID[] = "ROSProjectManager.ROSRunStep";

const char ROS_GENERIC_COMMAND_KEY[] = "ROSProjectManager.ROSGenericStep.Command";
const char ROS_GENERIC_PACKAGE_KEY[] = "ROSProjectManager.ROSGenericStep.Package";
const char ROS_GENERIC_TARGET_KEY[] = "ROSProjectManager.ROSGenericStep.Target";
const char ROS_GENERIC_ARGUMENTS_KEY[] = "ROSProjectManager.ROSGenericStep.Arguments";

ROSRunStep::ROSRunStep(RunStepList *rsl) :
    ROSRunStep(rsl, Core::Id(ROS_RUN_ID))
{
}

ROSRunStep::ROSRunStep(RunStepList *rsl, Core::Id id) : ROSGenericRunStep(rsl, id)
{
}

ROSRunStep::ROSRunStep(RunStepList *rsl, RunStep *rs) : ROSGenericRunStep(rsl, rs)
{
}

ROSLaunchStep::ROSLaunchStep(RunStepList *rsl) :
    ROSLaunchStep(rsl, Core::Id(ROS_LAUNCH_ID))
{
}

ROSLaunchStep::ROSLaunchStep(RunStepList *rsl, Core::Id id) : ROSGenericRunStep(rsl, id)
{
}

ROSLaunchStep::ROSLaunchStep(RunStepList *rsl, RunStep *rs) : ROSGenericRunStep(rsl, rs)
{
}

ROSGenericRunStep::ROSGenericRunStep(RunStepList *rsl, Core::Id id) : RunStep(rsl, id)
{
  if(id == ROS_LAUNCH_ID)
  {
    m_command = QLatin1String("roslaunch");
  }
  else if (id == ROS_RUN_ID)
  {
    m_command = QLatin1String("rosrun");
  }
  else
  {
    //TODO: need to throw error
  }
}

ROSGenericRunStep::ROSGenericRunStep(RunStepList *rsl, RunStep *rs) : RunStep(rsl, rs)
{
}

ROSGenericRunStep::~ROSGenericRunStep()
{
}

bool ROSGenericRunStep::init(QList<const RunStep *> &earlierSteps)
{
    Q_UNUSED(earlierSteps);

    ROSRunConfiguration *rc = rosRunConfiguration();
    if (!rc)
    {
      rc = targetsActiveRunConfiguration();

      if (!rc)
      {
          return false;
      }
    }

    return true;
}
void ROSGenericRunStep::run(QFutureInterface<bool> &fi)
{
  Q_UNUSED(fi);

  ROSProject *rp = qobject_cast<ROSProject *>(target()->project());

  QString command;
  command = QString::fromLatin1("%1 %2 %3 %4\n")
      .arg(m_command,
           m_package,
           m_target,
           m_arguments);

  //create terminal without starting shell
  QTermWidget &terminal = ROSManager::instance()->startTerminal(0, QString::fromLatin1("%1 %2 %3").arg(m_command, m_package, m_target));

  terminal.setWorkingDirectory(rp->projectDirectory().toString());

  //start bash now that everything is setup
  terminal.startShellProgram();

  // source workspace (This is a hack because the setEnvironment is not working as I expected)
  ROSUtils::WorkspaceInfo workspaceInfo = ROSUtils::getWorkspaceInfo(rp->projectDirectory(), rp->rosBuildConfiguration()->buildSystem(), rp->distribution());
  terminal.sendText(QString("source %1\n").arg(workspaceInfo.develPath.appendPath("setup.bash").toString()));

  //send roslaunch command
  terminal.sendText(command);
}

QVariantMap ROSGenericRunStep::toMap() const
{
    QVariantMap map(RunStep::toMap());

    map.insert(QLatin1String(ROS_GENERIC_COMMAND_KEY), m_command);
    map.insert(QLatin1String(ROS_GENERIC_PACKAGE_KEY), m_package);
    map.insert(QLatin1String(ROS_GENERIC_TARGET_KEY), m_target);
    map.insert(QLatin1String(ROS_GENERIC_ARGUMENTS_KEY), m_arguments);
    return map;
}

bool ROSGenericRunStep::fromMap(const QVariantMap &map)
{
    m_command = map.value(QLatin1String(ROS_GENERIC_COMMAND_KEY)).toString();
    m_package = map.value(QLatin1String(ROS_GENERIC_PACKAGE_KEY)).toString();
    m_target = map.value(QLatin1String(ROS_GENERIC_TARGET_KEY)).toString();
    m_arguments = map.value(QLatin1String(ROS_GENERIC_ARGUMENTS_KEY)).toString();

    return RunStep::fromMap(map);
}

ROSRunConfiguration *ROSGenericRunStep::rosRunConfiguration() const
{
    return static_cast<ROSRunConfiguration *>(runConfiguration());
}

ROSRunConfiguration *ROSGenericRunStep::targetsActiveRunConfiguration() const
{
    return static_cast<ROSRunConfiguration *>(target()->activeRunConfiguration());
}

RunStepConfigWidget *ROSGenericRunStep::createConfigWidget()
{
    return new ROSGenericRunStepConfigWidget(this);
}

QString ROSGenericRunStep::getCommand() const
{
  return m_command;
}

QString ROSGenericRunStep::getPackage() const
{
  return m_package;
}

QString ROSGenericRunStep::getTarget() const
{
  return m_target;
}

QString ROSGenericRunStep::getArguments() const
{
  return m_arguments;
}

void ROSGenericRunStep::setPackage(const QString &package)
{
  m_package = package;
}

void ROSGenericRunStep::setTarget(const QString &target)
{
  m_target = target;
}

void ROSGenericRunStep::setArguments(const QString &arguments)
{
  m_arguments = arguments;
}


//
// ROSLaunchStepConfigWidget
//

ROSGenericRunStepConfigWidget::ROSGenericRunStepConfigWidget(ROSGenericRunStep *genericStep)
    : m_rosGenericStep(genericStep),
      m_packageNames(new QStringListModel())
{
    updateAvailablePackages();

    m_ui = new Ui::ROSGenericStep();
    m_ui->setupUi(this);
    m_ui->packageComboBox->setStyleSheet(tr("combobox-popup: 0;"));
    m_ui->targetComboBox->setStyleSheet(tr("combobox-popup: 0;"));
    m_ui->packageComboBox->setModel(m_packageNames);

    int idx;
    idx = m_ui->packageComboBox->findText(genericStep->getPackage(), Qt::MatchExactly);
    m_ui->packageComboBox->setCurrentIndex(idx);

    m_ui->targetComboBox->addItems(getAvailableTargets());
    idx = m_ui->targetComboBox->findText(genericStep->getTarget(), Qt::MatchExactly);
    m_ui->targetComboBox->setCurrentIndex(idx);

    m_ui->argumentsLineEdit->setText(genericStep->getArguments());

    connect(m_ui->packageComboBox, SIGNAL(currentIndexChanged(QString)),
            this, SLOT(packageComboBox_currentIndexChanged(QString)));

    connect(m_ui->targetComboBox, SIGNAL(currentIndexChanged(QString)),
            this, SLOT(targetComboBox_currentIndexChanged(QString)));

    connect(m_ui->argumentsLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(argumentsLineEdit_textChanged(QString)));

    connect(ProjectExplorer::BuildManager::instance(), &ProjectExplorer::BuildManager::buildQueueFinished,
            this, &ROSGenericRunStepConfigWidget::updateAvailablePackages);
}

ROSGenericRunStepConfigWidget::~ROSGenericRunStepConfigWidget()
{
    delete m_ui;
}

void ROSGenericRunStepConfigWidget::updateAvailablePackages()
{
    ROSBuildConfiguration *bc = qobject_cast<ROSBuildConfiguration *>(m_rosGenericStep->target()->activeBuildConfiguration());
    m_availablePackages = ROSUtils::getROSPackages(bc->environment().toStringList());
    m_packageNames->setStringList(m_availablePackages.keys());
}

QString ROSGenericRunStepConfigWidget::displayName() const
{
    return tr("ROS Step", "ROS step display name.");
}

QString ROSGenericRunStepConfigWidget::summaryText() const
{
  //this is causing a the plugin to crash
  return QString::fromLatin1("<b>%1:</b> %2 %3 %4 %5")
          .arg(displayName(),
               m_rosGenericStep->getCommand(),
               m_rosGenericStep->getPackage(),
               m_rosGenericStep->getTarget(),
               m_rosGenericStep->getArguments());
}

void ROSGenericRunStepConfigWidget::packageComboBox_currentIndexChanged(const QString &arg1)
{
  m_rosGenericStep->setPackage(arg1);
  m_ui->targetComboBox->addItems(getAvailableTargets());
  emit updateSummary();
}

void ROSGenericRunStepConfigWidget::targetComboBox_currentIndexChanged(const QString &arg1)
{
  m_rosGenericStep->setTarget(arg1);
  emit updateSummary();
}

void ROSGenericRunStepConfigWidget::argumentsLineEdit_textChanged(const QString &arg1)
{
  m_rosGenericStep->setArguments(arg1);
  emit updateSummary();
}

QStringList ROSGenericRunStepConfigWidget::getAvailableTargets()
{
    if(m_rosGenericStep->id() == ROS_LAUNCH_ID)
    {
      return ROSUtils::getROSPackageLaunchFiles(m_availablePackages[m_rosGenericStep->getPackage()]);
    }
    else if (m_rosGenericStep->id() == ROS_RUN_ID)
    {
      ROSBuildConfiguration *bc = qobject_cast<ROSBuildConfiguration *>(m_rosGenericStep->target()->activeBuildConfiguration());
      return ROSUtils::getROSPackageExecutables(m_rosGenericStep->getPackage(), bc->environment().toStringList());
    }

    return QStringList();
}

//
// ROSRunStepFactory
//

ROSRunStepFactory::ROSRunStepFactory(QObject *parent) :
    IRunStepFactory(parent)
{
}

bool ROSRunStepFactory::canCreate(RunStepList *parent, const Core::Id id) const
{
    if (parent->target()->project()->id() == Constants::ROS_PROJECT_ID)
    {
      if (id == ROS_LAUNCH_ID || id == ROS_RUN_ID)
      {
        return true;
      }
    }
    return false;
}

RunStep *ROSRunStepFactory::create(RunStepList *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;

    if (id == ROS_LAUNCH_ID)
    {
      return new ROSLaunchStep(parent, id);
    }
    else if (id == ROS_RUN_ID)
    {
      return new ROSRunStep(parent, id);
    }

    return 0;
}

bool ROSRunStepFactory::canClone(RunStepList *parent, RunStep *source) const
{
    return canCreate(parent, source->id());
}

RunStep *ROSRunStepFactory::clone(RunStepList *parent, RunStep *source)
{
    if (!canClone(parent, source))
        return 0;

    if (source->id() == ROS_LAUNCH_ID)
    {
      ROSLaunchStep *old(qobject_cast<ROSLaunchStep *>(source));
      Q_ASSERT(old);
      return new ROSLaunchStep(parent, old);
    }
    else if (source->id() == ROS_RUN_ID)
    {
      ROSRunStep *old(qobject_cast<ROSRunStep *>(source));
      Q_ASSERT(old);
      return new ROSRunStep(parent, old);
    }
    else
    {
      return 0;
    }
}

bool ROSRunStepFactory::canRestore(RunStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

RunStep *ROSRunStepFactory::restore(RunStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    if (ProjectExplorer::idFromMap(map) == ROS_LAUNCH_ID)
    {
      ROSLaunchStep *rs(new ROSLaunchStep(parent, ProjectExplorer::idFromMap(map)));
      if (rs->fromMap(map))
          return rs;
      delete rs;
      return 0;
    }
    else if (ProjectExplorer::idFromMap(map) == ROS_RUN_ID)
    {
      ROSRunStep *rs(new ROSRunStep(parent, ProjectExplorer::idFromMap(map)));
      if (rs->fromMap(map))
          return rs;
      delete rs;
      return 0;
    }

    return 0;
}

QList<Core::Id> ROSRunStepFactory::availableCreationIds(RunStepList *parent) const
{
    if (parent->target()->project()->id() == Constants::ROS_PROJECT_ID)
        return QList<Core::Id>() << Core::Id(ROS_LAUNCH_ID) << Core::Id(ROS_RUN_ID);
    return QList<Core::Id>();
}

QString ROSRunStepFactory::displayNameForId(const Core::Id id) const
{
    if (id == ROS_LAUNCH_ID)
        return tr("ROS Launch Step");
    else if (id== ROS_RUN_ID)
        return tr("ROS Run Step");

    return QString();
}


} // namespace Internal
} // namespace ROSProjectManager
