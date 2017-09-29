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
#include "ros_project.h"

#include "ros_build_configuration.h"
#include "ros_catkin_make_step.h"
#include "ros_project_constants.h"
#include "ros_utils.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/projectinfo.h>
#include <cpptools/projectpartheaderpath.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/buildmanager.h>
#include <qtsupport/baseqtversion.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/algorithm.h>

#include <QDir>
#include <QProcessEnvironment>
#include <QtXml/QDomDocument>

#include <cpptools/cpprawprojectpart.h>
#include <cpptools/cppprojectupdater.h>

using namespace Core;
using namespace ProjectExplorer;

namespace ROSProjectManager {
namespace Internal {

////////////////////////////////////////////////////////////////////////////////////
//
// ROSProject
//
////////////////////////////////////////////////////////////////////////////////////

ROSProject::ROSProject(const Utils::FileName &fileName) :
    ProjectExplorer::Project(Constants::ROS_MIME_TYPE, fileName, [this]() { refresh(); }),
    m_cppCodeModelUpdater(new CppTools::CppProjectUpdater(this)),
    m_workspaceWatcher(new ROSWorkspaceWatcher(this))
{
    setId(Constants::ROS_PROJECT_ID);
    setProjectContext(Context(Constants::ROS_PROJECT_CONTEXT));
    setProjectLanguages(Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setDisplayName(projectFilePath().toFileInfo().completeBaseName());

    ROSProjectNode *project_node = new ROSProjectNode(this->projectFilePath());
    project_node->addNode(new FileNode(projectFilePath(), FileType::Project, false));
    setRootProjectNode(project_node);

    refresh();

    // Setup signal/slot connections
    connect(Core::VcsManager::instance(), &Core::VcsManager::repositoryChanged,
            this, &ROSProject::repositoryChanged);

    connect(ProjectExplorer::BuildManager::instance(), SIGNAL(buildQueueFinished(bool)),
            this, SLOT(buildQueueFinished(bool)));

    connect(m_workspaceWatcher, SIGNAL(fileListChanged()),
            this, SIGNAL(fileListChanged()));
}

ROSProject::~ROSProject()
{
    delete m_cppCodeModelUpdater;
    m_cppCodeModelUpdater = nullptr;

    m_projectFutureInterface->cancel();
}

bool ROSProject::saveProjectFile()
{
    DocumentManager::expectFileChange(projectFilePath().toString());
    // Make sure we can open the file for writing

    Utils::FileSaver saver(projectFilePath().toString(), QIODevice::Text);
    if (!saver.hasError())
    {
      QXmlStreamWriter workspaceXml(saver.file());
      ROSUtils::gererateQtCreatorWorkspaceFile(workspaceXml, m_projectFileContent);
      saver.setResult(&workspaceXml);
    }
    bool result = saver.finalize(ICore::mainWindow());
    DocumentManager::unexpectFileChange(projectFilePath().toString());
    return result;
}

QString ROSProject::distribution() const
{
    return m_projectFileContent.distribution;
}

ROSUtils::BuildSystem ROSProject::defaultBuildSystem() const
{
    return m_projectFileContent.defaultBuildSystem;
}

ROSBuildConfiguration* ROSProject::rosBuildConfiguration() const
{
    return static_cast<ROSBuildConfiguration *>(activeTarget()->activeBuildConfiguration());
}

void ROSProject::parseProjectFile()
{
    ROSUtils::parseQtCreatorWorkspaceFile(projectFilePath(), m_projectFileContent);
}

void ROSProject::update()
{
    ROSUtils::WorkspaceInfo workspaceInfo = ROSUtils::getWorkspaceInfo(projectDirectory(), rosBuildConfiguration()->buildSystem(), distribution());
    ROSBuildConfiguration *bc = rosBuildConfiguration();
    m_wsPackageInfo = ROSUtils::getWorkspacePackageInfo(workspaceInfo, &m_wsPackageInfo);
    m_wsPackageBuildInfo = ROSUtils::getWorkspacePackageBuildInfo(workspaceInfo, m_wsPackageInfo, &m_wsPackageBuildInfo);

    if (m_wsPackageBuildInfo.isEmpty())
        m_wsEnvironment = Utils::Environment(ROSUtils::getWorkspaceEnvironment(workspaceInfo).toStringList());
    else
        m_wsEnvironment = Utils::Environment(m_wsPackageBuildInfo.first().environment);

    if (bc)
        bc->updateQtEnvironment(m_wsEnvironment);

}

ROSUtils::PackageInfoMap ROSProject::getPackageInfo() const
{
    return m_wsPackageInfo;
}

ROSUtils::PackageBuildInfoMap ROSProject::getPackageBuildInfo() const
{
    return m_wsPackageBuildInfo;
}

void ROSProject::refresh()
{
    m_projectFutureInterface = new QFutureInterface<void>();

    m_projectFutureInterface->setProgressRange(0, 100);
    Core::ProgressManager::addTask(m_projectFutureInterface->future(),
                                   tr("Reading Project \"%1\"").arg(displayName()),
                                   Constants::ROS_READING_PROJECT);


    m_projectFutureInterface->reportStarted();

    QSet<QString> oldWatchDirectories = m_projectFileContent.watchDirectories.toSet();
    parseProjectFile();
    QSet<QString> newWatchDirectories = m_projectFileContent.watchDirectories.toSet();

    // Make sure the workspace is initialized on refresh.
    ROSUtils::WorkspaceInfo workspaceInfo = ROSUtils::getWorkspaceInfo(this->projectDirectory(), m_projectFileContent.defaultBuildSystem, m_projectFileContent.distribution);
    if(!ROSUtils::isWorkspaceInitialized(workspaceInfo))
    {
        QProcess process;
        ROSUtils::initializeWorkspace(&process, workspaceInfo);
    }

    QStringList addedDirectories = (newWatchDirectories - oldWatchDirectories).toList();
    QStringList removedDirectories = (oldWatchDirectories - newWatchDirectories).toList();

    int cnt = 0;
    int max = removedDirectories.size() + addedDirectories.size();

    foreach (QString dir, removedDirectories) {
      cnt += 1;
      Utils::FileName removedDir = projectDirectory().appendPath(dir);
      if (removedDir.isChildOf(projectDirectory()))
        m_workspaceWatcher->unwatchFolder(removedDir.parentDir().toString(), dir);

      m_projectFutureInterface->setProgressValue(floor(100.0 * (float)cnt / (float)max));
    }

    foreach (QString dir, addedDirectories) {
      cnt += 1;
      Utils::FileName addedDir = projectDirectory().appendPath(dir);
      if (addedDir.isChildOf(projectDirectory()) && addedDir.exists())
        m_workspaceWatcher->watchFolder(addedDir.parentDir().toString(), dir);

      m_projectFutureInterface->setProgressValue(floor(100.0 * (float)cnt / (float)max));
    }

    m_projectFutureInterface->setProgressValue(100);
    m_projectFutureInterface->reportFinished();

//    m_workspaceWatcher->print();
}

void ROSProject::refreshCppCodeModel()
{
    update();

    const Kit *k = nullptr;

    if (Target *target = activeTarget())
        k = target->kit();
    else
        k = KitManager::defaultKit();

    QTC_ASSERT(k, return);

    const Utils::FileName sysRoot = SysRootKitInformation::sysRoot(k);

    m_cppCodeModelUpdater->cancel();

    CppTools::ProjectPart::QtVersion activeQtVersion = CppTools::ProjectPart::NoQt;
    if (QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(k)) {
        if (qtVersion->qtVersion() <= QtSupport::QtVersionNumber(4,8,6))
            activeQtVersion = CppTools::ProjectPart::Qt4_8_6AndOlder;
        else if (qtVersion->qtVersion() < QtSupport::QtVersionNumber(5,0,0))
            activeQtVersion = CppTools::ProjectPart::Qt4Latest;
        else
            activeQtVersion = CppTools::ProjectPart::Qt5;
    }

    CppTools::RawProjectParts rpps;

    QStringList workspaceFiles = m_workspaceWatcher->getWorkspaceFiles();

    ToolChain *cxxToolChain = ToolChainKitInformation::toolChain(k, ProjectExplorer::Constants::CXX_LANGUAGE_ID);

    foreach(ROSUtils::PackageBuildInfo buildInfo, m_wsPackageBuildInfo)
    {
        QStringList packgeFiles = workspaceFiles.filter(buildInfo.parent.path.toString() + QDir::separator());

        foreach(ROSUtils::PackageTargetInfo targetInfo, buildInfo.targets)
        {
            CppTools::RawProjectPart rpp;
            const QString defineArg
                    = Utils::transform(targetInfo.defines, [](const QString &s) -> QString {
                        QString result = QString::fromLatin1("#define ") + s;
                        int assignIndex = result.indexOf('=');
                        if (assignIndex != -1)
                            result[assignIndex] = ' ';
                        return result;
                    }).join('\n');

            rpp.setProjectFileLocation(projectFilePath().toString());
            rpp.setBuildSystemTarget(buildInfo.parent.name + '|' + targetInfo.name + '|' + projectFilePath().toString());
            rpp.setDisplayName(buildInfo.parent.name + '|' + targetInfo.name);
            rpp.setQtVersion(activeQtVersion);
            rpp.setDefines(defineArg.toUtf8());

            QSet<QString> toolChainIncludes;
            foreach (const HeaderPath &hp, cxxToolChain->systemHeaderPaths(targetInfo.flags, sysRoot))
                toolChainIncludes.insert(hp.path());

            QStringList includePaths;
            foreach (const QString &i, targetInfo.includes) {
                if (!toolChainIncludes.contains(i))
                    includePaths.append(i);
            }

            rpp.setIncludePaths(includePaths);
            rpp.setFlagsForCxx({cxxToolChain, targetInfo.flags});
            rpp.setFiles(packgeFiles);
            rpps.append(rpp);
        }
    }

    m_cppCodeModelUpdater->update({this, nullptr, cxxToolChain, k, rpps});
}

QStringList ROSProject::files(FilesMode fileMode) const
{
    Q_UNUSED(fileMode);
    return m_workspaceWatcher->getWorkspaceFiles();
}

Project::RestoreResult ROSProject::fromMap(const QVariantMap &map, QString *errorMessage)
{
      RestoreResult result = Project::fromMap(map, errorMessage);
      if (result != RestoreResult::Ok)
          return result;

      Kit *defaultKit = KitManager::defaultKit();
      if (!activeTarget() && defaultKit)
          addTarget(createTarget(defaultKit));

      // Sanity check: We need both a buildconfiguration and a runconfiguration!
      QList<Target *> targetList = targets();
      if (targetList.isEmpty())
          return RestoreResult::Error;

      foreach (Target *t, targetList) {
          if (!t->activeBuildConfiguration()) {
              removeTarget(t);
              continue;
          }
          if (!t->activeRunConfiguration())
              t->addRunConfiguration(new ProjectExplorer::CustomExecutableRunConfiguration(t));
      }

      refreshCppCodeModel();
      return RestoreResult::Ok;
}

void ROSProject::repositoryChanged(const QString &repository)
{
  static_cast<ROSProjectNode *>(rootProjectNode())->updateVersionControlInfo(repository);
}

void ROSProject::buildQueueFinished(bool success)
{
    Q_UNUSED(success);
    refreshCppCodeModel();
}

} // namespace Internal
} // namespace GenericProjectManager
