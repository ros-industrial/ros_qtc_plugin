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
#ifndef ROSPROJECT_H
#define ROSPROJECT_H


#include "ros_workspace_watcher.h"
#include "ros_project_manager.h"
#include "ros_project_nodes.h"
#include "ros_project_plugin.h"
#include "ros_utils.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/buildconfiguration.h>
#include <coreplugin/idocument.h>

#include <QFuture>
#include <QFutureInterface>

namespace CppTools {
    class CppProjectUpdater;
}

namespace ROSProjectManager {
namespace Internal {

class ROSProjectFile;
class ROSBuildConfiguration;

class ROSProject : public ProjectExplorer::Project
{
    Q_OBJECT
    friend class ROSProjectPlugin;

public:
    //ROSProject(ROSManager *manager, const QString &filename);
    ROSProject(const Utils::FileName &filename);
    ~ROSProject() override;

#if QT_CREATOR_VER < QT_CREATOR_VER_CHECK(4,3,0)
    QString displayName() const override { return m_projectName; }
    ROSManager *projectManager() const override;
    QStringList files(FilesMode fileMode) const override;
#else
    ROSManager *projectManager() const;
    QStringList files(FilesMode fileMode) const;
#endif

    void settManager(ROSManager* manager);

    void refresh();

    QString distribution() const;
    ROSUtils::BuildSystem defaultBuildSystem() const;
    ROSBuildConfiguration* rosBuildConfiguration() const;

    ROSUtils::PackageInfoMap getPackageInfo() const;
    ROSUtils::PackageBuildInfoMap getPackageBuildInfo() const;

public slots:
    void buildQueueFinished(bool success);

protected:
    Project::RestoreResult fromMap(const QVariantMap &map, QString *errorMessage) override;

private:
    bool saveProjectFile();
    void parseProjectFile();

    void update();
    void refreshCppCodeModel();
    void repositoryChanged(const QString &repository);

    ROSUtils::ROSProjectFileContent m_projectFileContent;
    QFutureInterface<void>     *m_projectFutureInterface = nullptr;
    ROSUtils::PackageInfoMap    m_wsPackageInfo;
    ROSUtils::PackageBuildInfoMap m_wsPackageBuildInfo;
    Utils::Environment          m_wsEnvironment;

#if QT_CREATOR_VER < QT_CREATOR_VER_CHECK(4,3,0)
    QFuture<void>               m_codeModelFuture;
    QString                     m_projectName;
#else
    CppTools::CppProjectUpdater *m_cppCodeModelUpdater;
    /// RJG TODO: Check who will release it
    ROSManager                  *m_manager;
#endif

    // Moved here so ctor init order is correct
    // else on >= 4.3.0 you will see compilation warrning -Wreorder
    ROSWorkspaceWatcher         *m_workspaceWatcher;
};

class ROSProjectFile : public Core::IDocument
{
    Q_OBJECT

public:
    ROSProjectFile(ROSProject *parent, const Utils::FileName& fileName);

    bool save(QString *errorString, const QString &fileName, bool autoSave) override;

    bool isModified() const override;
    bool isSaveAsAllowed() const override;

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const override;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;

private:
    ROSProject *m_project;
};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSPROJECT_H
