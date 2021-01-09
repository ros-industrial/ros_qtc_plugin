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
#ifndef ROS_RUN_CONFIGURATION_H
#define ROS_RUN_CONFIGURATION_H

#include "ros_run_step.h"
#include "ros_project_constants.h"
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/devicesupport/deviceprocesslist.h>
#include <debugger/debuggerruncontrol.h>

#include <QPointer>
#include <QMenu>
#include <QFutureInterface>
#include <QFutureWatcher>
#include <QTimer>

QT_FORWARD_DECLARE_CLASS(QStringListModel)


namespace ROSProjectManager {
namespace Internal {

class ROSRunConfigurationFactory;

namespace Ui { class ROSRunConfiguration; class ROSLaunchConfiguration;}

class ROSRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    friend class ROSRunConfigurationFactory;

public:
    ROSRunConfiguration(ProjectExplorer::Target *parent, Core::Id id);

    // RunConfiguration
    QString disabledReason() const override;

    QVariantMap toMap() const override;

    RunStepList *stepList() const;

protected:

    virtual bool fromMap(const QVariantMap &map) override;

private:

    RunStepList *m_stepList;
};

class ROSRunConfigurationFactory : public ProjectExplorer::RunConfigurationFactory
{
public:
    explicit ROSRunConfigurationFactory();
    ~ROSRunConfigurationFactory() override;

protected:
    QList<ProjectExplorer::RunConfigurationCreationInfo>
    availableCreators(ProjectExplorer::Target *parent) const override;
};

class ROSRunWorker : public ProjectExplorer::RunWorker
{
    Q_OBJECT

public:
    explicit ROSRunWorker(ProjectExplorer::RunControl *runControl);
    void start() override;
};

class ROSDebugRunWorker : public Debugger::DebuggerRunTool
{
    Q_OBJECT

public:
    explicit ROSDebugRunWorker(ProjectExplorer::RunControl *runControl);
    void start() override;

private:
    void findProcess();
    void pidFound(ProjectExplorer::DeviceProcessItem process);
    QTimer m_timer;
    int m_timeElapsed;
    QString m_debugTargetPath;
    bool m_debugContinueOnAttach;
};

} // namespace Internal

} // namespace ROSProjectManager

#endif // ROS_RUN_CONFIGURATION_H
