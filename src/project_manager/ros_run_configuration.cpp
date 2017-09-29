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
#include "ros_project.h"
#include "ros_run_steps_page.h"
#include "ros_run_steps.h"
#include "ui_ros_run_configuration.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/coreicons.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <projectexplorer/target.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/buildstepspage.h>
#include <projectexplorer/runnables.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtoutputformatter.h>
#include <qtsupport/qtsupportconstants.h>
 
#include <utils/fileutils.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcprocess.h>
#include <utils/winutils.h>
#include <qmljstools/qmljstoolsconstants.h>
#include <utils/detailswidget.h>
#include <utils/utilsicons.h>

#include <QLineEdit>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QDebug>
#include <QMessageBox>

using namespace Core;
using namespace ProjectExplorer;

namespace ROSProjectManager {
namespace Internal {

const char ROS_RC_ID[] = "ROSProjectManager.ROSRunConfiguration";
const char ROS_RUN_STEP_LIST_ID[] = "ROSProjectManager.ROSRunConfiguration.RunStepList";

bool operator==(const ROSRunnable &r1, const ROSRunnable &r2)
{
    Q_UNUSED(r1);
    Q_UNUSED(r2);
    return true;
}

void *ROSRunnable::staticTypeId = &ROSRunnable::staticTypeId;


ROSRunConfiguration::ROSRunConfiguration(Target *parent):
    ROSRunConfiguration(parent, Core::Id(ROS_RC_ID))
{
}

ROSRunConfiguration::ROSRunConfiguration(Target *parent, Id id) :
    RunConfiguration(parent, id),
    m_stepList(new RunStepList(this, Core::Id(ROS_RUN_STEP_LIST_ID))),
    m_isEnabled(false)
{
    m_stepList->setDefaultDisplayName(tr("Run"));   
    ctor();
}

ROSRunConfiguration::ROSRunConfiguration(Target *parent, ROSRunConfiguration *source) :
    RunConfiguration(parent, source),
    m_stepList(source->m_stepList),
    m_isEnabled(source->m_isEnabled)
{
    ctor();
}

bool ROSRunConfiguration::isEnabled() const
{
    return m_stepList->enabled();
}

QString ROSRunConfiguration::disabledReason() const
{
    if (!m_isEnabled)
        return tr("No ROS run step for active project.");
    return QString();
}

void ROSRunConfiguration::ctor()
{
//    // reset default settings in constructor
//    connect(EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
//            this, SLOT(changeCurrentFile(Core::IEditor*)));
//    connect(EditorManager::instance(), SIGNAL(currentDocumentStateChanged()),
//            this, SLOT(changeCurrentFile()));

//    connect(target(), SIGNAL(kitChanged()),
//            this, SLOT(updateEnabled()));
setDisplayName(tr("ROS Run Configuration", "ROS run configuration display name."));

//    if (id() == Constants::QML_SCENE_RC_ID)
//        setDisplayName(tr("QML Scene", "QMLRunConfiguration display name."));
//    else
//        setDisplayName(tr("QML Viewer", "QMLRunConfiguration display name."));
//    updateEnabled();
}

QWidget *ROSRunConfiguration::createConfigurationWidget()
{
    return new RunStepsPage(this);
}

Utils::OutputFormatter *ROSRunConfiguration::createOutputFormatter() const
{
    return new QtSupport::QtOutputFormatter(target()->project());
}

Abi ROSRunConfiguration::abi() const
{
    Abi hostAbi = Abi::hostAbi();
    return Abi(hostAbi.architecture(), hostAbi.os(), hostAbi.osFlavor(),
               Abi::RuntimeQmlFormat, hostAbi.wordWidth());
}

ProjectExplorer::Runnable ROSRunConfiguration::runnable() const
{
    ROSRunnable r;
    return r;
}

QVariantMap ROSRunConfiguration::toMap() const
{
    QVariantMap map(RunConfiguration::toMap());
    map.insert(QLatin1String(ROS_RUN_STEP_LIST_ID), m_stepList->toMap());

    return map;
}

bool ROSRunConfiguration::fromMap(const QVariantMap &map)
{
  QVariantMap data = map.value(QLatin1String(ROS_RUN_STEP_LIST_ID)).toMap();
          if (data.isEmpty()) {
              qWarning() << "No data for ROS run step list found!";
              return false;
          }
          RunStepList *list = new RunStepList(this, data);
          if (list->isNull()) {
              qWarning() << "Failed to restore ROS run step list!";
              delete list;
              return false;
          }
          list->setDefaultDisplayName(tr("Run"));
          m_stepList = list;
    return RunConfiguration::fromMap(map);
}

RunStepList* ROSRunConfiguration::stepList() const
{
  return m_stepList;
}

/*!
  \class ROSRunConfigurationFactory
*/

ROSRunConfigurationFactory::ROSRunConfigurationFactory(QObject *parent) :
    ProjectExplorer::IRunConfigurationFactory(parent)
{
    setObjectName(QLatin1String("ROSRunConfigurationFactory"));
}

ROSRunConfigurationFactory::~ROSRunConfigurationFactory()
{
}

QList<Core::Id> ROSRunConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent, CreationMode mode) const
{
    Q_UNUSED(mode);

    if (!canHandle(parent))
        return QList<Core::Id>();

    // First id will be the default run configuration
    QList<Core::Id> list;
    list << Core::Id(ROS_RC_ID);

    return list;
}

QString ROSRunConfigurationFactory::displayNameForId(Core::Id id) const
{
    if (id == ROS_RC_ID)
        return tr("ROS Run Configuration");

    return QString();
}

bool ROSRunConfigurationFactory::canCreate(ProjectExplorer::Target *parent,
                                                  const Core::Id id) const
{
    if (!canHandle(parent))
        return false;

    if (id == ROS_RC_ID)
        return true;

    return false;
}

ProjectExplorer::RunConfiguration *ROSRunConfigurationFactory::doCreate(ProjectExplorer::Target *parent, Core::Id id)
{
  return new ROSRunConfiguration(parent, id);
}

bool ROSRunConfigurationFactory::canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    return parent && canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::RunConfiguration *ROSRunConfigurationFactory::doRestore(ProjectExplorer::Target *parent,
                                                                                const QVariantMap &map)
{
    return new ROSRunConfiguration(parent, ProjectExplorer::idFromMap(map));
}

bool ROSRunConfigurationFactory::canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

ProjectExplorer::RunConfiguration *ROSRunConfigurationFactory::clone(ProjectExplorer::Target *parent,
                                                                     ProjectExplorer::RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    return new ROSRunConfiguration(parent, qobject_cast<ROSRunConfiguration *>(source));
}

bool ROSRunConfigurationFactory::canHandle(ProjectExplorer::Target *parent) const
{
    if (!parent->project()->supportsKit(parent->kit()))
        return false;
    if (!qobject_cast<ROSProject *>(parent->project()))
        return false;
    Core::Id deviceType = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(parent->kit());
    return deviceType == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
}

////////////////////////////////////
/// ROSRunWorker
////////////////////////////////////
ROSRunWorker::ROSRunWorker(RunControl *runControl) : RunWorker(runControl)
{
    setDisplayName("RosRunWorker");
}

void ROSRunWorker::start()
{
    if (runControl()->runMode() == ProjectExplorer::Constants::NORMAL_RUN_MODE)
    {
        foreach(RunStep *rs, qobject_cast<ROSRunConfiguration *>(runControl()->runConfiguration())->stepList()->steps())
        {
            if (rs->enabled() == true)
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
        msg.setText("Debug is only supported using the following methods:"
                    "<ul>"
                    "  <li>Attach to a Running Process</li>"
                    "  <li>Attach to a Unstarted Process</li>"
                    "</ul>"
                    "Note: See <a href=https://github.com/ros-industrial/ros_qtc_plugin/wiki/4.-Debugging-Catkin-Workspace>GitHub wiki</a> for help.");
        msg.exec();
    }

}

} // namespace Internal
} // namespace ROSProjectManager


