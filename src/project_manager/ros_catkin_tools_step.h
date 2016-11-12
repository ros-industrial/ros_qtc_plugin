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
#ifndef ROSCATKINTOOLSSTEP_H
#define ROSCATKINTOOLSSTEP_H

#include <projectexplorer/abstractprocessstep.h>

#include "ros_build_configuration.h"

#include <QDialog>
#include <yaml-cpp/yaml.h>

QT_BEGIN_NAMESPACE
class QListWidgetItem;
class QAction;
QT_END_NAMESPACE

namespace ROSProjectManager {
namespace Internal {

class ROSCatkinToolsStepWidget;
class ROSCatkinToolsStepFactory;
namespace Ui {class ROSCatkinToolsStep; class ROSCatkinToolsConfigEditor;}

class ROSCatkinToolsStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT

    friend class ROSCatkinToolsStepWidget;
    friend class ROSCatkinToolsStepFactory;

public:
    enum BuildTargets {BUILD = 0, CLEAN = 1};

    ROSCatkinToolsStep(ProjectExplorer::BuildStepList *parent);
    ~ROSCatkinToolsStep();

    bool init(QList<const BuildStep *> &earlierSteps) override;
    void run(QFutureInterface<bool> &fi) override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    bool immutable() const override;

    ROSBuildConfiguration *rosBuildConfiguration() const;
    BuildTargets buildsTarget() const;
    void setBuildTarget(const BuildTargets &target);

    QString activeProfile() const;
    void setActiveProfile(const QString &profileName);

    QString allArguments(ROSUtils::BuildType buildType) const;
    QString makeCommand() const;

    QVariantMap toMap() const;



protected:
    ROSCatkinToolsStep(ProjectExplorer::BuildStepList *parent, ROSCatkinToolsStep *bs);
    ROSCatkinToolsStep(ProjectExplorer::BuildStepList *parent, Core::Id id);
    QStringList automaticallyAddedArguments() const;
    bool fromMap(const QVariantMap &map) override;

    void stdOutput(const QString &line) override;
    void processStarted() override;
    void processFinished(int exitCode, QProcess::ExitStatus status) override;

private:
    void ctor();
    ROSBuildConfiguration *targetsActiveBuildConfiguration() const;

    BuildTargets m_target;
    QString m_activeProfile;
    QString m_catkinToolsArguments;
    QString m_catkinMakeArguments;
    QString m_cmakeArguments;
    QString m_makeArguments;
    QRegExp m_percentProgress;
};


class ROSCatkinToolsStepWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    ROSCatkinToolsStepWidget(ROSCatkinToolsStep *makeStep);
    ~ROSCatkinToolsStepWidget();
    QString displayName() const;
    QString summaryText() const;

private slots:
    void updateDetails();

private:
    Ui::ROSCatkinToolsStep *m_ui;
    ROSCatkinToolsStep *m_makeStep;
    QString m_summaryText;
    QMenu *m_addButtonMenu;
    QMenu *m_profileMenu;

    void updateAddProfileButtonMenu();
    void updateProfileButtonMenu();
    void setProfile(const QString profileName);
    void cloneProfile(const QString profileName);
    void renameProfile(const QString profileName);
    void deleteProfile(const QString profileName);
    void editProfile(const QString profileName);

    QString uniqueName(const QString &name);
};

class ROSCatkinToolsProfileEditorDialog : public QDialog
{
    Q_OBJECT
public:
    ROSCatkinToolsProfileEditorDialog(Utils::FileName filePath);

};

class ROSCatkinToolsConfigEditorWidget : public QWidget
{
    Q_OBJECT
public:
    ROSCatkinToolsConfigEditorWidget();
    ~ROSCatkinToolsConfigEditorWidget();

    bool parseProfileConfig(Utils::FileName filePath);
    bool saveProfileConfig();
    bool isModified() const;

private slots:
    void onActionEditFilePathListTriggered();
    void onActionEditPackageListTriggered();
    void onActionEditStingListTriggered();

private:
    Ui::ROSCatkinToolsConfigEditor *m_ui;
    QAction *m_editor;
    bool m_modified;
    Utils::FileName m_profileConfigPath;
    YAML::Node m_profile_original;
    YAML::Node m_profile_current;

};

class ROSCatkinToolsStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    explicit ROSCatkinToolsStepFactory(QObject *parent = 0);
    QList<ProjectExplorer::BuildStepInfo> availableSteps(ProjectExplorer::BuildStepList *parent) const override;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, Core::Id id) override;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) override;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) override;
};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSCATKINTOOLSSTEP_H
