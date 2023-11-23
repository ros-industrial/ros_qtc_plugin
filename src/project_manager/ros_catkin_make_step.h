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

static const char ROS_CMS_ID[] = "ROSProjectManager.ROSCatkinMakeStep";

class ROSCatkinMakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT

    friend class ROSCatkinMakeStepWidget;
    friend class ROSCatkinMakeStepFactory;

public:
    enum BuildTargets {BUILD = 0, CLEAN = 1};

    ROSCatkinMakeStep(ProjectExplorer::BuildStepList *parent, Utils::Id id);
    ~ROSCatkinMakeStep() override;

    bool init() override;
    void setupOutputFormatter(Utils::OutputFormatter *formatter) override;

    ROSBuildConfiguration *rosBuildConfiguration() const;
    BuildTargets buildTarget() const;
    void setBuildTarget(const BuildTargets &target);
    QString allArguments(ROSUtils::BuildType buildType, bool includeDefault = true) const;
    Utils::CommandLine makeCommand(const QString &args) const;

    void toMap(Utils::Store &map) const override;

protected:
    QStringList automaticallyAddedArguments() const;
    void fromMap(const Utils::Store &map) override;
    QWidget *createConfigWidget() override;

private:
    ROSBuildConfiguration *targetsActiveBuildConfiguration() const;
    void stdOutput(const QString &line);

    BuildTargets m_target;
    QString m_catkinMakeArguments;
    QString m_cmakeArguments;
    QString m_makeArguments;
    QRegularExpression m_percentProgress;
};

class ROSCatkinMakeStepWidget : public QWidget
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

class ROSCatkinMakeStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    explicit ROSCatkinMakeStepFactory();
};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSCATKINMAKESTEP_H
