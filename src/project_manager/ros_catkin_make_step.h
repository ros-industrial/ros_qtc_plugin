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
#ifndef ROSCATKINMAKESTEP_H
#define ROSCATKINMAKESTEP_H

#include <projectexplorer/abstractprocessstep.h>
#include "ros_build_configuration.h"

QT_BEGIN_NAMESPACE
class QListWidgetItem;
QT_END_NAMESPACE

namespace ROSProjectManager {
namespace Internal {

class ROSCatkinMakeStepWidget;
class ROSCatkinMakeStepFactory;
namespace Ui { class ROSCatkinMakeStep; }

class ROSCatkinMakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT

    friend class ROSCatkinMakeStepWidget;
    friend class ROSCatkinMakeStepFactory;

public:
    enum BuildTargets {BUILD = 0, CLEAN = 1};

    ROSCatkinMakeStep(ProjectExplorer::BuildStepList *parent);
    ~ROSCatkinMakeStep();

    bool init(QList<const BuildStep *> &earlierSteps) override;
    void run(QFutureInterface<bool> &fi) override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    bool immutable() const override;

    ROSBuildConfiguration *rosBuildConfiguration() const;
    BuildTargets buildTarget() const;
    void setBuildTarget(const BuildTargets &target);
    QString allArguments(ROSUtils::BuildType buildType, bool includeDefault = true) const;
    QString makeCommand() const;

    QVariantMap toMap() const;

protected:
    ROSCatkinMakeStep(ProjectExplorer::BuildStepList *parent, ROSCatkinMakeStep *bs);
    ROSCatkinMakeStep(ProjectExplorer::BuildStepList *parent, Core::Id id);
    QStringList automaticallyAddedArguments() const;
    bool fromMap(const QVariantMap &map) override;

    void stdOutput(const QString &line) override;
    void processStarted() override;
    void processFinished(int exitCode, QProcess::ExitStatus status) override;

private:
    void ctor();
    ROSBuildConfiguration *targetsActiveBuildConfiguration() const;

    BuildTargets m_target;
    QString m_catkinMakeArguments;
    QString m_cmakeArguments;
    QString m_makeArguments;
    QRegExp m_percentProgress;
};

class ROSCatkinMakeStepWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    ROSCatkinMakeStepWidget(ROSCatkinMakeStep *makeStep);
    ~ROSCatkinMakeStepWidget();
    QString displayName() const;
    QString summaryText() const;

private slots:
    void updateDetails();
    void updateBuildSystem(const ROSUtils::BuildSystem &buildSystem);
    void enabledChanged();

private:
    Ui::ROSCatkinMakeStep *m_ui;
    ROSCatkinMakeStep *m_makeStep;
    QString m_summaryText;
};

class ROSCatkinMakeStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    explicit ROSCatkinMakeStepFactory(QObject *parent = 0);
    QList<ProjectExplorer::BuildStepInfo> availableSteps(ProjectExplorer::BuildStepList *parent) const override;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, Core::Id id) override;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) override;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) override;
};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSCATKINMAKESTEP_H
