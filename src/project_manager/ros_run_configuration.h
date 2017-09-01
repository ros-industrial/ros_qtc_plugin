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

#include <QPointer>
#include <QMenu>
#include <QFutureInterface>
#include <QFutureWatcher>

QT_FORWARD_DECLARE_CLASS(QStringListModel)


namespace ROSProjectManager {
namespace Internal {

class ROSRunConfigurationFactory;
class ROSRunControlFactory;

namespace Ui { class ROSRunConfiguration; class ROSLaunchConfiguration;}

class ROSRunnable
{
public:
    static void *staticTypeId;
};
bool operator==(const ROSRunnable &r1, const ROSRunnable &r2);


class ROSRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    friend class ROSRunConfigurationFactory;
    friend class ROSRunConfigurationWidget;

public:
    explicit ROSRunConfiguration(ProjectExplorer::Target *parent);

    // RunConfiguration
    bool isEnabled() const override;
    QString disabledReason() const override;
    virtual QWidget *createConfigurationWidget() override;
    Utils::OutputFormatter *createOutputFormatter() const override;
    QVariantMap toMap() const override;

    ProjectExplorer::Abi abi() const override;

    ProjectExplorer::Runnable runnable() const override;

    RunStepList *stepList() const;

protected:
    ROSRunConfiguration(ProjectExplorer::Target *parent, Core::Id id);
    ROSRunConfiguration(ProjectExplorer::Target *parent,
                               ROSRunConfiguration *source);
    virtual bool fromMap(const QVariantMap &map) override;

private:
    void ctor();

    RunStepList *m_stepList;
    bool m_isEnabled;
};

class ROSRunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit ROSRunConfigurationFactory(QObject *parent = 0);
    ~ROSRunConfigurationFactory();

    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent, CreationMode mode) const override;
    QString displayNameForId(Core::Id id) const override;

    bool canCreate(ProjectExplorer::Target *parent, Core::Id id) const override;
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const override;
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) const override;
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) override;

private:
    bool canHandle(ProjectExplorer::Target *parent) const;

    ProjectExplorer::RunConfiguration *doCreate(ProjectExplorer::Target *parent, Core::Id id) override;
    ProjectExplorer::RunConfiguration *doRestore(ProjectExplorer::Target *parent,
                                                 const QVariantMap &map) override;
};

class ROSRunControl : public ProjectExplorer::RunControl
{
  Q_OBJECT
public:
  explicit ROSRunControl(ProjectExplorer::RunConfiguration *rc);

  void start() override;
  StopResult stop() override;

protected:
  ROSRunControl(ProjectExplorer::RunConfiguration *rc, Core::Id id);

private slots:
  void handleErrorMessage(const QString &error);
  void handleRunnerFinished();
  void handleRemoteOutput(const QByteArray &output);
  void handleRemoteErrorOutput(const QByteArray &output);
  void handleProgressReport(const QString &progressString);

private:

  class ROSRunControlPrivate;
  ROSRunControlPrivate * const d;
  ROSRunConfiguration *m_rc;
  QFutureWatcher<bool> m_watcher;
  QFutureInterface<bool> m_futureInterfaceForAysnc;
};

class ROSRunControlFactory : public ProjectExplorer::IRunControlFactory
{
  Q_OBJECT
public:
  explicit ROSRunControlFactory(QObject * parent = 0);

  bool canRun(ProjectExplorer::RunConfiguration *rc, Core::Id mode) const override;
  ProjectExplorer::RunControl *create(ProjectExplorer::RunConfiguration *rc, Core::Id mode, QString *errorMessage) override;
};

} // namespace Internal

} // namespace ROSProjectManager

#endif // ROS_RUN_CONFIGURATION_H
