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

class ROSCatkinMakeStepConfigWidget;
class ROSCatkinMakeStepFactory;
namespace Ui { class ROSMakeStep; }

class ROSCatkinMakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT

    friend class ROSCatkinMakeStepConfigWidget;
    friend class ROSCatkinMakeStepFactory;

public:
    ROSCatkinMakeStep(ProjectExplorer::BuildStepList *parent);
    ~ROSCatkinMakeStep();

    bool init(QList<const BuildStep *> &earlierSteps) override;
    void run(QFutureInterface<bool> &fi) override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    bool immutable() const override;

    ROSBuildConfiguration *rosBuildConfiguration() const;
    bool buildsTarget(const QString &target) const;
    void setBuildTarget(const QString &target, bool on);
    QString allArguments(QString initial_arguments) const;
    QString makeCommand() const;

    void setClean(bool clean);
    bool isClean() const;

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

    QStringList m_buildTargets;
    QString m_makeArguments;
    QString m_makeCommand;
    QRegExp m_percentProgress;
    bool m_clean;
};

class ROSCatkinMakeStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    ROSCatkinMakeStepConfigWidget(ROSCatkinMakeStep *makeStep);
    ~ROSCatkinMakeStepConfigWidget();
    QString displayName() const;
    QString summaryText() const;

private slots:
    void itemChanged(QListWidgetItem *item);
    void makeLineEditTextEdited();
    void makeArgumentsLineEditTextEdited();
    void updateMakeOverrrideLabel();
    void updateDetails();

private:
    Ui::ROSMakeStep *m_ui;
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
