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
#include "ros_run_configuration.h"
#include "ros_generic_run_step.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/coreicons.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/messagemanager.h>
#include <projectexplorer/target.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/buildstepspage.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <debugger/debuggerengine.h>

#include <qtsupport/qtoutputformatter.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/fileutils.h>
#include <utils/winutils.h>
#include <qmljstools/qmljstoolsconstants.h>
#include <utils/detailswidget.h>
#include <utils/utilsicons.h>
#include <utils/processinfo.h>

#include <QLineEdit>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QDebug>
#include <QMessageBox>

using namespace ProjectExplorer;

namespace ROSProjectManager {
namespace Internal {

const char ROS_RC_ID[] = "ROSProjectManager.ROSRunConfiguration";

ROSRunConfiguration::ROSRunConfiguration(Target *target, Utils::Id id) :
    RunConfiguration(target, id),
    m_stepList(new RunStepList(target, Constants::ROS_RUN_STEP_LIST_ID))
{
}


QString ROSRunConfiguration::disabledReason(Utils::Id runMode) const
{
  QString output;
  output = RunConfiguration::disabledReason(runMode);

  if (output.isEmpty())
  {
    if (!isEnabled(runMode))
        output = tr("No ROS run step for active project.");
  }

  return output;
}

RunStepList* ROSRunConfiguration::stepList() const
{
  return m_stepList;
}

/*!
  \class ROSRunConfigurationFactory
*/

ROSRunConfigurationFactory::ROSRunConfigurationFactory() :
    ProjectExplorer::RunConfigurationFactory()
{
  registerRunConfiguration<ROSRunConfiguration>(ROS_RC_ID);
  addSupportedProjectType(Constants::ROS_PROJECT_ID);
  addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
}

ROSRunConfigurationFactory::~ROSRunConfigurationFactory()
{
}

QList<ProjectExplorer::RunConfigurationCreationInfo>
ROSRunConfigurationFactory::availableCreators(ProjectExplorer::Target */*parent*/) const
{
  RunConfigurationCreationInfo rci;
  rci.factory = this;
  rci.buildKey = "This is a test";
  rci.displayName = "ROS Run Configuration";
  rci.creationMode = RunConfigurationCreationInfo::ManualCreationOnly;
  rci.useTerminal = false;

  return {rci};
}

} // namespace Internal
} // namespace ROSProjectManager


