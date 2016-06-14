/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtoutputformatter.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/fileutils.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcprocess.h>
#include <utils/winutils.h>
#include <qmljstools/qmljstoolsconstants.h>
#include <utils/detailswidget.h>

#include <QLineEdit>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QStandardItemModel>
#include <QVBoxLayout>

using namespace Core;
using namespace ProjectExplorer;

namespace ROSProjectManager {
namespace Internal {

const char ROS_RC_ID[] = "ROSProjectManager.ROSRunConfiguration";
const char ROS_RUN_STEP_LIST_ID[] = "ROSProjectManager.ROSRunConfiguration.RunStepList";
//const char ROS_RUN_CONTROL_ID[] = "ROSProjectManager.ROSRunControl";

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
    m_isEnabled = true; //TODO: Need to create a way to ask the ros project if it is ok.
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
    return m_isEnabled;
}

QString ROSRunConfiguration::disabledReason() const
{
    if (!m_isEnabled)
        return tr("No qmlviewer or qmlscene found.");
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

QString ROSRunConfiguration::executable() const
{
//    QtSupport::BaseQtVersion *version = qtVersion();
//    if (!version)
//        return QString();

//    if (id() == Constants::QML_SCENE_RC_ID)
//        return version->qmlsceneCommand();
//    return version->qmlviewerCommand();
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
    Q_UNUSED(mode)

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

/*!
  \class ROSRunControlFactory https://github.com/qtproject/qt-creator/blob/66bdd60947b946b8aa30141e3871f33226f0cc37/src/plugins/remotelinux/remotelinuxruncontrol.cpp
*/
ROSRunControlFactory::ROSRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
}

bool ROSRunControlFactory::canRun(RunConfiguration *rc, Core::Id mode) const
{
    if (mode != ProjectExplorer::Constants::NORMAL_RUN_MODE
            && mode != ProjectExplorer::Constants::DEBUG_RUN_MODE
            && mode != ProjectExplorer::Constants::DEBUG_RUN_MODE_WITH_BREAK_ON_MAIN
            && mode != ProjectExplorer::Constants::QML_PROFILER_RUN_MODE) {
        return false;
    }

    return rc->isEnabled() && (rc->id() == ROS_RC_ID);
}

RunControl *ROSRunControlFactory::create(RunConfiguration *rc, Core::Id mode,
                                                 QString *errorMessage)
{
    QTC_ASSERT(canRun(rc, mode), return 0);

    if (mode == ProjectExplorer::Constants::NORMAL_RUN_MODE)
        return new ROSRunControl(rc);

    QTC_CHECK(false);
    return 0;
}


/*!
  \class ROSRunControl https://github.com/qtproject/qt-creator/blob/66bdd60947b946b8aa30141e3871f33226f0cc37/src/plugins/remotelinux/remotelinuxruncontrol.cpp
*/

class ROSRunControl::ROSRunControlPrivate
{
public:
    bool running;
    DeviceApplicationRunner runner;
    IDevice::ConstPtr device;
    QString remoteExecutable;
    QString arguments;
    Utils::Environment environment;
    QString workingDir;
};


ROSRunControl::ROSRunControl(ProjectExplorer::RunConfiguration *rc) :
  ROSRunControl(rc, ProjectExplorer::Constants::NORMAL_RUN_MODE)
{
}

ROSRunControl::ROSRunControl(RunConfiguration *rc, Id id):
    RunControl(rc, id),
    d(new ROSRunControlPrivate)
{
  setIcon(Core::Icons::RUN_SMALL);

  m_rc = qobject_cast<ROSRunConfiguration *>(rc);
  d->running = false;
  d->device = DeviceKitInformation::device(m_rc->target()->kit());

  //d->arguments = lrc->arguments();
  d->environment = m_rc->target()->activeBuildConfiguration()->environment();
  d->workingDir = m_rc->target()->activeBuildConfiguration()->buildDirectory().toString();

}


void ROSRunControl::start()
{
  foreach(RunStep *rs, m_rc->stepList()->steps())
  {
    rs->run(m_futureInterfaceForAysnc);
  }

//  d->running = true;
//  emit started();
//  d->runner.disconnect(this);
//  connect(&d->runner, &ProjectExplorer::DeviceApplicationRunner::reportError,
//          this, &ROSRunControl::handleErrorMessage);
//  connect(&d->runner, &ProjectExplorer::DeviceApplicationRunner::remoteStderr,
//          this, &ROSRunControl::handleRemoteErrorOutput);
//  connect(&d->runner, &ProjectExplorer::DeviceApplicationRunner::remoteStdout,
//          this, &ROSRunControl::handleRemoteOutput);
//  connect(&d->runner, &ProjectExplorer::DeviceApplicationRunner::finished,
//          this, &ROSRunControl::handleRunnerFinished);
//  connect(&d->runner, &ProjectExplorer::DeviceApplicationRunner::reportProgress,
//          this, &ROSRunControl::handleProgressReport);

//  d->runner.setEnvironment(d->environment);
//  d->runner.setWorkingDirectory(d->workingDir);



//  d->runner.start(d->device, d->remoteExecutable,
//                  Utils::QtcProcess::splitArgs(d->arguments, Utils::OsTypeLinux));

}


RunControl::StopResult ROSRunControl::stop()
{
  d->runner.stop();
  return RunControl::AsynchronousStop;
}

void ROSRunControl::handleErrorMessage(const QString &error)
{
    appendMessage(error, Utils::ErrorMessageFormat);
}

void ROSRunControl::handleRunnerFinished()
{
    d->runner.disconnect(this);
    d->running = false;
    emit finished();
}

void ROSRunControl::handleRemoteOutput(const QByteArray &output)
{
    appendMessage(QString::fromUtf8(output), Utils::StdOutFormatSameLine);
}

void ROSRunControl::handleRemoteErrorOutput(const QByteArray &output)
{
    appendMessage(QString::fromUtf8(output), Utils::StdErrFormatSameLine);
}

void ROSRunControl::handleProgressReport(const QString &progressString)
{
    appendMessage(progressString + QLatin1Char('\n'), Utils::NormalMessageFormat);
}

bool ROSRunControl::isRunning() const
{
  return d->running;
}


} // namespace Internal
} // namespace ROSProjectManager


