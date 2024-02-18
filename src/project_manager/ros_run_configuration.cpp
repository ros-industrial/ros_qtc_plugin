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

////////////////////////////////////
/// ROSRunWorker
////////////////////////////////////
ROSRunWorker::ROSRunWorker(RunControl *runControl) : RunWorker(runControl)
{
    setId("RosRunWorker");
}

void ROSRunWorker::start()
{
    for (RunStep *rs : qobject_cast<ROSRunConfiguration *>(runControl()->target()->activeRunConfiguration())->stepList()->steps())
    {
        if (rs->enabled() == true && rs->id() != ROSProjectManager::Constants::ROS_ATTACH_TO_NODE_ID)
        {
            rs->run();
        }
    }
}

////////////////////////////////////
/// ROSDebugRunWorker
////////////////////////////////////

ROSDebugRunWorker::ROSDebugRunWorker(RunControl *runControl) : Debugger::DebuggerRunTool(runControl)
{
    setId("RosDebugRunWorker");

    connect(&m_timer, &QTimer::timeout,
    this, &ROSDebugRunWorker::findProcess);

    connect(this, &ROSDebugRunWorker::stopped,
            &m_timer, &QTimer::stop);
}

void ROSDebugRunWorker::start()
{
    bool found = false;
    for (RunStep *rs : qobject_cast<ROSRunConfiguration *>(runControl()->target()->activeRunConfiguration())->stepList()->steps())
    {
        if (rs->enabled() == true && rs->id() == Constants::ROS_ATTACH_TO_NODE_ID)
        {
            found = true;
            m_debugContinueOnAttach = qobject_cast<ROSGenericRunStep *>(rs)->getDebugContinueOnAttach();
            m_debugTargetPath = qobject_cast<ROSGenericRunStep *>(rs)->getTargetPath();
            if (QFileInfo(m_debugTargetPath).exists())
            {
                m_timeElapsed = 0;
                m_timer.start(10);
            }
            else
            {
                QMessageBox msg;
                msg.setWindowTitle("Debugging Catkin Workspace");
                msg.setTextFormat(Qt::RichText);
                msg.setWindowFlags(Qt::WindowStaysOnTopHint);
                msg.setText("The <b><i>ROS Attach to Node</b></i> Run Step is not complete! Please verify information and try again.");
                msg.exec();
                return;
            }

        }
    }

    if (found)
    {
        // Now that the watcher is started run all of the other steps
        for (RunStep *rs : qobject_cast<ROSRunConfiguration *>(runControl()->target()->activeRunConfiguration())->stepList()->steps())
        {
            if (rs->enabled() == true && rs->id() != ROSProjectManager::Constants::ROS_ATTACH_TO_NODE_ID)
            {
                rs->run();
            }
        }
    }
    else
    {
        QMessageBox msg;
        msg.setWindowTitle("Debugging Catkin Workspace");
        msg.setTextFormat(Qt::RichText);
        msg.setWindowFlags(Qt::WindowStaysOnTopHint);
        msg.setText("In order to debug a ROS Node the <b><i>ROS Attach to Node</b></i> Run Step must be added and enabled!");
        msg.exec();
        return;
    }
}

void ROSDebugRunWorker::pidFound(const Utils::ProcessInfo &process)
{
    m_timer.stop();
    setAttachPid(Utils::ProcessHandle(process.processId));
    setId(tr("Process %1").arg(process.processId));
    setInferiorExecutable(Utils::FilePath::fromString(process.executable));
    setStartMode(Debugger::StartExternal);
    setCloseMode(Debugger::DetachAtClose);
    setContinueAfterAttach(m_debugContinueOnAttach);
    DebuggerRunTool::start();
}

void ROSDebugRunWorker::findProcess()
{
    m_timeElapsed += 10;
    const QString &appName = Utils::FileUtils::normalizedPathName(m_debugTargetPath);
    Utils::ProcessInfo fallback;
    for (const Utils::ProcessInfo &p : Utils::ProcessInfo::processInfoList()) {
        if (Utils::FileUtils::normalizedPathName(p.executable) == appName) {
            Core::MessageManager::writeSilently(tr("[ROS] Attaching to process: %1.").arg(appName));
            pidFound(p);
            return;
        }
        if (p.commandLine.startsWith(appName))
            fallback = p;
    }
    if (fallback.processId != 0)
        pidFound(fallback);

    // Make sure this does not run indefinitely. Allow 30sec to start the process.
    if (m_timeElapsed >= 30000)
    {
        m_timer.stop();
        Core::MessageManager::writeFlashing(tr("[ROS Error] Unable to find process: %1.").arg(appName));
    }
}

} // namespace Internal
} // namespace ROSProjectManager


