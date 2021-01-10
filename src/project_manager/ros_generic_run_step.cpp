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
#include "ros_generic_run_step.h"
#include "ros_project.h"
#include "ros_build_configuration.h"
#include "ui_ros_generic_configuration.h"

#include <projectexplorer/buildmanager.h>
#include <coreplugin/messagemanager.h>

#include <QStringListModel>

namespace ROSProjectManager {
namespace Internal {

const char ROS_GENERIC_COMMAND_KEY[] = "ROSProjectManager.ROSGenericStep.Command";
const char ROS_GENERIC_PACKAGE_KEY[] = "ROSProjectManager.ROSGenericStep.Package";
const char ROS_GENERIC_PACKAGE_PATH_KEY[] = "ROSProjectManager.ROSGenericStep.PackagePath";
const char ROS_GENERIC_TARGET_KEY[] = "ROSProjectManager.ROSGenericStep.Target";
const char ROS_GENERIC_TARGET_PATH_KEY[] = "ROSProjectManager.ROSGenericStep.TargetPath";
const char ROS_GENERIC_ARGUMENTS_KEY[] = "ROSProjectManager.ROSGenericStep.Arguments";
const char ROS_GENERIC_DEBUG_CONTINUE_ON_ATTACH_KEY[] = "ROSProjectManager.ROSGenericStep.DebugContinueOnAttach";

ROSGenericRunStep::ROSGenericRunStep(RunStepList *rsl, Utils::Id id) : RunStep(rsl, id)
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

void ROSGenericRunStep::run()
{
  ROSProject *rp = qobject_cast<ROSProject *>(target()->project());

  QString command;
  command = QString("%1 %2 %3 %4\n")
      .arg(m_command,
           m_package,
           m_target,
           m_arguments);

  ROSUtils::WorkspaceInfo workspaceInfo = ROSUtils::getWorkspaceInfo(rp->projectDirectory(), rp->rosBuildConfiguration()->rosBuildSystem(), rp->distribution());
  ROSBuildConfiguration *bc = qobject_cast<ROSBuildConfiguration *>(target()->activeBuildConfiguration());
  Utils::Environment env = bc->environment();
  Utils::FilePath shell = Utils::FilePath::fromString(env.value("SHELL"));
  QString source_cmd;

  Utils::FilePath sourcePath(workspaceInfo.develPath);
  if (workspaceInfo.install)
    sourcePath = Utils::FilePath(workspaceInfo.installPath);

  if (shell.fileName() == "bash")
      source_cmd = QString("source %1\n").arg(sourcePath.pathAppended("setup.bash").toString());
  else if (shell.fileName() == "sh")
      source_cmd = QString("source %1\n").arg(sourcePath.pathAppended("setup.sh").toString());
  else if (shell.fileName() == "zsh")
       source_cmd = QString("source %1\n").arg(sourcePath.pathAppended("setup.zsh").toString());
  else
       Core::MessageManager::write(tr("[ROS Error] The shell: %1 is currently not supported (Use bash, sh, or zsh)!").arg(shell.toString()));

  //create terminal without starting shell
  QTermWidget &terminal = ROSProjectPlugin::instance()->startTerminal(0, command);

  terminal.setWorkingDirectory(rp->projectDirectory().toString());

  //start bash now that everything is setup
  terminal.startShellProgram();

  // source workspace (This is a hack because the setEnvironment is not working as I expected)
  terminal.sendText(source_cmd);

  //send roslaunch command
  terminal.sendText(command);
}

QVariantMap ROSGenericRunStep::toMap() const
{
    QVariantMap map(RunStep::toMap());

    map.insert(ROS_GENERIC_COMMAND_KEY, m_command);
    map.insert(ROS_GENERIC_PACKAGE_KEY, m_package);
    map.insert(ROS_GENERIC_PACKAGE_PATH_KEY, m_package_path);
    map.insert(ROS_GENERIC_TARGET_KEY, m_target);
    map.insert(ROS_GENERIC_TARGET_PATH_KEY, m_targetPath);
    map.insert(ROS_GENERIC_ARGUMENTS_KEY, m_arguments);
    map.insert(ROS_GENERIC_DEBUG_CONTINUE_ON_ATTACH_KEY, m_debugContinueOnAttach);
    return map;
}

bool ROSGenericRunStep::fromMap(const QVariantMap &map)
{
    m_command = map.value(ROS_GENERIC_COMMAND_KEY).toString();
    m_package = map.value(ROS_GENERIC_PACKAGE_KEY).toString();
    m_package_path = map.value(ROS_GENERIC_PACKAGE_PATH_KEY).toString();
    m_target = map.value(ROS_GENERIC_TARGET_KEY).toString();
    m_targetPath = map.value(ROS_GENERIC_TARGET_PATH_KEY).toString();
    m_arguments = map.value(ROS_GENERIC_ARGUMENTS_KEY).toString();
    m_debugContinueOnAttach = map.value(ROS_GENERIC_DEBUG_CONTINUE_ON_ATTACH_KEY).toBool();

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
    Q_ASSERT(false);
    return nullptr;
}

QString ROSGenericRunStep::getCommand() const
{
  return m_command;
}

QString ROSGenericRunStep::getPackage() const
{
  return m_package;
}

QString ROSGenericRunStep::getPackagePath() const
{
  return m_package_path;
}

QString ROSGenericRunStep::getTarget() const
{
  return m_target;
}

QString ROSGenericRunStep::getTargetPath() const
{
  return m_targetPath;
}

QString ROSGenericRunStep::getArguments() const
{
  return m_arguments;
}

bool ROSGenericRunStep::getDebugContinueOnAttach() const
{
    return m_debugContinueOnAttach;
}

void ROSGenericRunStep::setCommand(const QString &command)
{
  m_command = command;
}

void ROSGenericRunStep::setPackage(const QString &package)
{
  m_package = package;
}

void ROSGenericRunStep::setPackagePath(const QString &package)
{
  m_package_path = package;
}

void ROSGenericRunStep::setTarget(const QString &target)
{
  m_target = target;
}

void ROSGenericRunStep::setTargetPath(const QString &targetPath)
{
  m_targetPath = targetPath;
}

void ROSGenericRunStep::setArguments(const QString &arguments)
{
  m_arguments = arguments;
}

void ROSGenericRunStep::setDebugContinueOnAttach(const bool &contOnAttach)
{
  m_debugContinueOnAttach = contOnAttach;
}

//
// ROSGenericRunStepConfigWidget
//

ROSGenericRunStepConfigWidget::ROSGenericRunStepConfigWidget(ROSGenericRunStep *genericStep, bool packages_show, bool args_show, bool debug_show)
    : m_rosGenericStep(genericStep),
      m_packageNames(new QStringListModel()),
      m_targetNames(new QStringListModel())
{
    m_ui = new Ui::ROSGenericStep();
    m_ui->setupUi(this);


    if (!packages_show)
    {
        m_ui->packageLabel->hide();
        m_ui->packageComboBox->hide();

        m_ui->targetLabel->hide();
        m_ui->targetComboBox->hide();
    }
    else
    {
      m_ui->packageComboBox->setStyleSheet(tr("combobox-popup: 0;"));
      m_ui->packageComboBox->setModel(m_packageNames);
      m_ui->targetComboBox->setStyleSheet(tr("combobox-popup: 0;"));
      m_ui->targetComboBox->setModel(m_targetNames);

      int idx;
      updateAvailablePackages();
      idx = m_ui->packageComboBox->findText(genericStep->getPackage(), Qt::MatchExactly);
      m_ui->packageComboBox->setCurrentIndex(idx);

      updateAvailableTargets();
      idx = m_ui->targetComboBox->findText(genericStep->getTarget(), Qt::MatchExactly);
      m_ui->targetComboBox->setCurrentIndex(idx);

      connect(m_ui->packageComboBox, SIGNAL(currentIndexChanged(QString)),
              this, SLOT(packageComboBox_currentIndexChanged(QString)));

      connect(m_ui->targetComboBox, SIGNAL(currentIndexChanged(QString)),
              this, SLOT(targetComboBox_currentIndexChanged(QString)));
    }

    if (!args_show)
    {
        m_ui->argumentsLabel->hide();
        m_ui->argumentsLineEdit->hide();
    }
    else
    {
        m_ui->argumentsLineEdit->setText(genericStep->getArguments());

        connect(m_ui->argumentsLineEdit, SIGNAL(textChanged(QString)),
                this, SLOT(argumentsLineEdit_textChanged(QString)));
    }

    if (!debug_show) //Note this only used for Attach to Node Run Step
    {
        m_ui->debugLabel->hide();
        m_ui->debugCheckBox->hide();
    }
    else
    {
        m_ui->debugCheckBox->setChecked(genericStep->getDebugContinueOnAttach());
        connect(m_ui->debugCheckBox, SIGNAL(toggled(bool)),
                this, SLOT(debugCheckBox_toggled(bool)));
    }

    connect(ProjectExplorer::BuildManager::instance(), &ProjectExplorer::BuildManager::buildQueueFinished,
            this, &ROSGenericRunStepConfigWidget::updateAvailablePackages);
}

ROSGenericRunStepConfigWidget::~ROSGenericRunStepConfigWidget()
{
    delete m_ui;
}

void ROSGenericRunStepConfigWidget::updateAvailablePackages()
{
    QString cachePkgName = m_ui->packageComboBox->currentText();
    QString cachePkgTarget = m_ui->targetComboBox->currentText();

    ROSBuildConfiguration *bc = qobject_cast<ROSBuildConfiguration *>(m_rosGenericStep->target()->activeBuildConfiguration());
    m_availablePackages = ROSUtils::getROSPackages(bc->environment().toStringList());
    m_packageNames->setStringList(m_availablePackages.keys());

    m_ui->packageComboBox->setCurrentText(cachePkgName);
    m_ui->targetComboBox->setCurrentText(cachePkgTarget);
}

QString ROSGenericRunStepConfigWidget::displayName() const
{
    return tr("ROS Step", "ROS step display name.");
}

QString ROSGenericRunStepConfigWidget::summaryText() const
{
  //this is causing a the plugin to crash
  return QString("<b>%1:</b> %2 %3 %4 %5")
          .arg(displayName(),
               m_rosGenericStep->getCommand(),
               m_rosGenericStep->getPackage(),
               m_rosGenericStep->getTarget(),
               m_rosGenericStep->getArguments());
}

void ROSGenericRunStepConfigWidget::debugCheckBox_toggled(const bool &arg1)
{
    m_rosGenericStep->setDebugContinueOnAttach(arg1);
}

void ROSGenericRunStepConfigWidget::packageComboBox_currentIndexChanged(const QString &arg1)
{
  m_rosGenericStep->setPackage(arg1);
  m_rosGenericStep->setPackagePath(m_availablePackages[arg1]);
  m_ui->targetComboBox->clear();
  updateAvailableTargets();

  emit updateSummary();
}

void ROSGenericRunStepConfigWidget::targetComboBox_currentIndexChanged(const QString &arg1)
{
  m_rosGenericStep->setTarget(arg1);
  m_rosGenericStep->setTargetPath(m_availableTargets[arg1]);
  emit updateSummary();
}

void ROSGenericRunStepConfigWidget::argumentsLineEdit_textChanged(const QString &arg1)
{
  m_rosGenericStep->setArguments(arg1);
  emit updateSummary();
}

void ROSGenericRunStepConfigWidget::updateAvailableTargets()
{
    m_availableTargets = m_rosGenericStep->getAvailableTargets();
    m_targetNames->setStringList(m_availableTargets.keys());
}

} // namespace Internal
} // namespace ROSProjectManager
