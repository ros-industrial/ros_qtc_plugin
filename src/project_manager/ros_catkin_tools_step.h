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
#include <QLineEdit>
#include <yaml-cpp/yaml.h>

QT_BEGIN_NAMESPACE
class QListWidgetItem;
class QAction;
QT_END_NAMESPACE

namespace ROSProjectManager {
namespace Internal {

class ROSCatkinToolsStepWidget;
class ROSCatkinToolsStepFactory;
namespace Ui {class ROSCatkinToolsStep;
              class ROSCatkinToolsListEditor;
              class ROSCatkinToolsConfigEditor;}

static const char ROS_CTS_ID[] = "ROSProjectManager.ROSCatkinToolsStep";

class ROSCatkinToolsStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT

    friend class ROSCatkinToolsStepWidget;
    friend class ROSCatkinToolsStepFactory;

public:
    enum BuildTargets {BUILD = 0, CLEAN = 1};

    ROSCatkinToolsStep(ProjectExplorer::BuildStepList *parent, Core::Id id);
    ~ROSCatkinToolsStep() override;

    bool init() override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;

    ROSBuildConfiguration *rosBuildConfiguration() const;
    BuildTargets buildTarget() const;
    void setBuildTarget(const BuildTargets &target);

    QString activeProfile() const;
    void setActiveProfile(const QString &profileName);

    QString allArguments(ROSUtils::BuildType buildType, bool includeDefault = true) const;
    Utils::CommandLine makeCommand(const QString &args) const;
    void stdOutput(const QString &line) override;

    QVariantMap toMap() const override;

protected:
    QStringList automaticallyAddedArguments() const;
    bool fromMap(const QVariantMap &map) override;

private:
    ROSBuildConfiguration *targetsActiveBuildConfiguration() const;

    BuildTargets m_target;
    QString m_activeProfile;
    QString m_catkinToolsArguments;
    QString m_catkinMakeArguments;
    QString m_cmakeArguments;
    QString m_makeArguments;
    QString m_catkinToolsWorkingDir;
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
    void updateBuildSystem(const ROSUtils::BuildSystem &buildSystem);
    void enabledChanged();

private:
    Ui::ROSCatkinToolsStep *m_ui;
    ROSCatkinToolsStep *m_makeStep;
    QString m_summaryText;
    QMenu *m_addButtonMenu;
    QMenu *m_profileMenu;

    void updateAddProfileButtonMenu();
    void updateProfileButtonMenu();
    void setProfile(const QString profileName);
    void newProfile();
    void cloneProfile(const QString profileName);
    void renameProfile(const QString profileName);
    void removeProfile(const QString profileName);
    void editProfile(const QString profileName);

    QString uniqueName(const QString &name, const bool &isRename);
};

class ROSCatkinToolsProfileEditorDialog : public QDialog
{
    Q_OBJECT
public:
    ROSCatkinToolsProfileEditorDialog(Utils::FilePath filePath);

};

class ROSCatkinToolsListEditorWidget : public QDialog
{
    Q_OBJECT
public:
    ROSCatkinToolsListEditorWidget(QWidget *parent = Q_NULLPTR);
    ~ROSCatkinToolsListEditorWidget();

    void setList(const QStringList &list);
    QStringList list() const;

private slots:
    void listWidget_itemChanged();
    void listWidget_itemSelectionChanged();

    void addPushButton_clicked();
    void removePushButton_clicked();

    void moveUpPushButton_clicked();
    void moveDownPushButton_clicked();

private:
    void addItem(const QString &str);

    Ui::ROSCatkinToolsListEditor *m_ui;
    QStringList m_list;
};

class ROSCatkinToolsListWidget: public QLineEdit
{
    Q_OBJECT
public:
    ROSCatkinToolsListWidget(QWidget *parent = Q_NULLPTR);
    ~ROSCatkinToolsListWidget();

    void setList(const QStringList &list);
    QStringList list() const;

private slots:
    void onActionEditListTriggered();

private:
    ROSCatkinToolsListEditorWidget *m_editor;
    QStringList m_list;
};

class ROSCatkinToolsConfigEditorWidget : public QWidget
{
    Q_OBJECT
public:
    ROSCatkinToolsConfigEditorWidget();
    ~ROSCatkinToolsConfigEditorWidget();

    bool parseProfileConfig(Utils::FilePath filePath);
    bool saveProfileConfig();
    bool isModified() const;
    bool isValid() const;

public slots:
    void propertyChanged();

private:
    QStringList parseList(const std::string &key) const;
    QString parseString(const std::string &key) const;
    bool parseBool(const std::string &key) const;
    std::vector<std::string> toStdVector(const QStringList &list) const;
    QStringList toQStringList(const std::vector<std::string> &list) const;

    Ui::ROSCatkinToolsConfigEditor *m_ui;
    ROSCatkinToolsListEditorWidget *m_editor;
    bool m_modified;
    bool m_parsing;
    Utils::FilePath m_profileConfigPath;
    YAML::Node m_profile_original;
    YAML::Node m_profile_current;
};


class ROSCatkinToolsStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    explicit ROSCatkinToolsStepFactory();
};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSCATKINTOOLSSTEP_H
