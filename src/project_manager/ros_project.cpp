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
#include "ros_project_nodes.h"
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
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <qtsupport/baseqtversion.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/algorithm.h>
#include <utils/runextensions.h>

#include <QDir>
#include <QProcessEnvironment>
#include <QtXml/QDomDocument>
#include <QtConcurrentRun>

#include <cpptools/cpprawprojectpart.h>
#include <cpptools/cppprojectupdater.h>

using namespace Core;
using namespace ProjectExplorer;

namespace ROSProjectManager {
namespace Internal {

static FolderNode *folderNode(const FolderNode *folder, const Utils::FileName &directory)
{
    return static_cast<FolderNode *>(Utils::findOrDefault(folder->folderNodes(),
                                                          [&directory](const FolderNode *fn) {
        return fn && fn->filePath() == directory;
    }));
}

static FolderNode *recursiveFindOrCreateFolderNode(FolderNode *folder,
                                                   const Utils::FileName &directory,
                                                   const Utils::FileName &overrideBaseDir,
                                                   const FolderNode::FolderNodeFactory &factory)
{
    Utils::FileName path = overrideBaseDir.isEmpty() ? folder->filePath() : overrideBaseDir;

    Utils::FileName directoryWithoutPrefix;
    bool isRelative = false;

    if (path.isEmpty() || path.toFileInfo().isRoot()) {
        directoryWithoutPrefix = directory;
        isRelative = false;
    } else {
        if (directory.isChildOf(path) || directory == path) {
            isRelative = true;
            directoryWithoutPrefix = directory.relativeChildPath(path);
        } else {
            isRelative = false;
            path.clear();
            directoryWithoutPrefix = directory;
        }
    }
    QStringList parts = directoryWithoutPrefix.toString().split('/', QString::SkipEmptyParts);
    if (!Utils::HostOsInfo::isWindowsHost() && !isRelative && parts.count() > 0)
        parts[0].prepend('/');

    ProjectExplorer::FolderNode *parent = folder;
    foreach (const QString &part, parts) {
        path.appendPath(part);
        // Find folder in subFolders
        FolderNode *next = folderNode(parent, path);
        if (!next) {
            // No FolderNode yet, so create it
            auto tmp = factory(path);
            tmp->setDisplayName(part);
            next = tmp.get();
            parent->addNode(std::move(tmp));
        }
        parent = next;
    }
    return parent;
}

////////////////////////////////////////////////////////////////////////////////////
//
// ROSProject
//
////////////////////////////////////////////////////////////////////////////////////
const int UPDATE_INTERVAL = 3000;

ROSProject::ROSProject(const Utils::FileName &fileName) :
    ProjectExplorer::Project(Constants::ROS_MIME_TYPE, fileName, [this]() { refresh(); }),
    m_cppCodeModelUpdater(new CppTools::CppProjectUpdater(this))
{
    setId(Constants::ROS_PROJECT_ID);
    setProjectLanguages(Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setDisplayName(projectFilePath().toFileInfo().completeBaseName());

    m_asyncUpdateTimer.setSingleShot(true);
    m_asyncUpdateTimer.setInterval(UPDATE_INTERVAL);
    connect(&m_asyncUpdateTimer, &QTimer::timeout, this, &ROSProject::asyncUpdate);

    refresh();

    // Setup signal/slot connections
    connect(ProjectExplorer::BuildManager::instance(), SIGNAL(buildQueueFinished(bool)),
            this, SLOT(buildQueueFinished(bool)));

    connect(&m_futureWatcher, &QFutureWatcher<ProjectExplorer::ProjectNode*>::finished, this, &ROSProject::updateProjectTree);

    connect(&m_watcher, SIGNAL(directoryChanged(QString)),
            this, SLOT(fileSystemChanged()));
}

ROSProject::~ROSProject()
{
    m_futureWatcher.cancel();
    m_futureWatcher.waitForFinished();

    delete m_cppCodeModelUpdater;
    m_cppCodeModelUpdater = nullptr;

    if (m_asyncUpdateFutureInterface) {
        m_asyncUpdateFutureInterface->reportCanceled();
        m_asyncUpdateFutureInterface->reportFinished();
        delete m_asyncUpdateFutureInterface;
    }
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
    // Parse project file and then update project
    ROSUtils::parseQtCreatorWorkspaceFile(projectFilePath(), m_projectFileContent);

    m_asyncUpdateTimer.setInterval(UPDATE_INTERVAL / 2);
    m_asyncUpdateTimer.start();
}

void ROSProject::updateProjectTree()
{
  if (m_futureWatcher.isFinished())
  {
    std::unique_ptr<ProjectExplorer::ProjectNode> pn(m_futureWatcher.result());
    setRootProjectNode(std::move(pn));
    m_asyncUpdateFutureInterface->reportFinished();
    delete m_asyncUpdateFutureInterface;
    m_asyncUpdateFutureInterface = nullptr;
    m_watcher.addPaths(m_workspaceDirectories);
  }

  emitParsingFinished(true);
}

void ROSProject::buildProjectTree(const Utils::FileName& projectFilePath, const QStringList& watchDirectories, QFutureInterface<ProjectExplorer::ProjectNode*> &fi, QHash<QString, ROSUtils::FolderContent>& workspaceContent, QStringList &files, QStringList& directories)
{
    fi.reportStarted();

    for (const QString& dir : watchDirectories) {
        Utils::FileName addedDir = projectFilePath.parentDir().appendPath(dir);
        QHash<QString, ROSUtils::FolderContent> newDirContent = ROSUtils::getFolderContent(addedDir, files, directories);
        workspaceContent.unite(newDirContent);
    }

    ROSProjectNode* project_node(new ROSProjectNode(projectFilePath.parentDir()));
    std::unique_ptr<FileNode> root_node(new FileNode(projectFilePath, FileType::Project, false));
    project_node->addNode(std::move(root_node));

    const ProjectExplorer::FolderNode::FolderNodeFactory &factory = [](const Utils::FileName &fn) { return std::make_unique<ROSFolderNode>(fn, fn.fileName()); };

    std::vector<std::unique_ptr<ProjectExplorer::FileNode>> childNodes;
    QHashIterator<QString, ROSUtils::FolderContent> item(workspaceContent);
    int cnt = 0;
    double max = workspaceContent.size();
    while (item.hasNext())
    {
      item.next();

      if (item.value().files.empty()) {
        // This is required so empty directories show up in project tree
        Utils::FileName empty_directory = Utils::FileName::fromString(item.key());
        recursiveFindOrCreateFolderNode(project_node, empty_directory, Utils::FileName(), factory);
      }
      else {
        // Add all files in the directory node
        for (const QString& file : item.value().files)
        {
          QFileInfo fileInfo(QDir(item.key()), file);

          ProjectExplorer::FileType fileType = ProjectExplorer::FileType::Source;

          if (Constants::HEADER_FILE_EXTENSIONS.contains(fileInfo.suffix()))
            fileType = ProjectExplorer::FileType::Header;

          std::unique_ptr<ProjectExplorer::FileNode> fileNode(new ProjectExplorer::FileNode(Utils::FileName::fromString(fileInfo.absoluteFilePath()), fileType, /*generated = */ false));
          childNodes.emplace_back(std::move(fileNode));
        }
      }

      cnt += 1;
      fi.setProgressValue(static_cast<int>(100.0 * static_cast<double>(cnt) / max));
    }

    project_node->addNestedNodes(std::move(childNodes), Utils::FileName(), factory);

    fi.setProgressValue(fi.progressMaximum());
    fi.reportResult(project_node);
    fi.reportFinished();
}



void ROSProject::fileSystemChanged()
{
  m_asyncUpdateTimer.setInterval(UPDATE_INTERVAL);
  m_asyncUpdateTimer.start();
}

void ROSProject::asyncUpdate()
{
  if (!isParsing())
      emitParsingStarted();

  m_futureWatcher.waitForFinished();

  Q_ASSERT(!m_asyncUpdateFutureInterface);
  m_asyncUpdateFutureInterface = new QFutureInterface<ProjectExplorer::ProjectNode*>();

  m_watcher.removePaths(m_watcher.directories());
  m_workspaceContent.clear();
  m_workspaceFiles.clear();
  m_workspaceDirectories.clear();

  m_asyncUpdateFutureInterface->setProgressRange(0, 100);
  Core::ProgressManager::addTask(m_asyncUpdateFutureInterface->future(),
                                 tr("Reading Project \"%1\"").arg(displayName()),
                                 Constants::ROS_READING_PROJECT);

  Utils::runAsync(ProjectExplorer::ProjectExplorerPlugin::sharedThreadPool(), QThread::LowestPriority, [this]() { ROSProject::buildProjectTree(projectFilePath(), m_projectFileContent.watchDirectories, *m_asyncUpdateFutureInterface, m_workspaceContent, m_workspaceFiles, m_workspaceDirectories); });

  m_futureWatcher.setFuture(m_asyncUpdateFutureInterface->future());
}

void ROSProject::refreshCppCodeModel(bool success)
{
    if (success)
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

        const Kit *k = nullptr;

        if (Target *target = activeTarget())
            k = target->kit();
        else
            k = KitManager::defaultKit();

        QTC_ASSERT(k, return);

        const Utils::FileName sysRoot = SysRootKitInformation::sysRoot(k);

        m_cppCodeModelUpdater->cancel();

        CppTools::ProjectPart::QtVersion activeQtVersion = CppTools::ProjectPart::NoQt;
        if (QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(activeTarget()->kit()))
        {
            if (qtVersion->qtVersion() < QtSupport::QtVersionNumber(5,0,0))
                activeQtVersion = CppTools::ProjectPart::Qt4;
            else
                activeQtVersion = CppTools::ProjectPart::Qt5;
        }

        CppTools::RawProjectParts rpps;

        QStringList workspaceFiles = m_workspaceFiles;

        ToolChain *cxxToolChain = ToolChainKitInformation::toolChain(k, ProjectExplorer::Constants::CXX_LANGUAGE_ID);

        for (const ROSUtils::PackageBuildInfo& buildInfo : m_wsPackageBuildInfo)
        {
            QStringList packgeFiles = workspaceFiles.filter(buildInfo.parent.path.toString() + QDir::separator());

            for (const ROSUtils::PackageTargetInfo& targetInfo : buildInfo.targets)
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
                rpp.setMacros(ProjectExplorer::Macro::toMacros(defineArg.toUtf8()));

                QSet<QString> toolChainIncludes;
                for (const HeaderPath &hp : cxxToolChain->builtInHeaderPaths(targetInfo.flags, sysRoot)) {
                    toolChainIncludes.insert(hp.path);
                }

                QStringList includePaths;
                for (const QString &i : targetInfo.includes) {
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

      for (Target *t : targetList) {
          if (!t->activeBuildConfiguration()) {
              removeTarget(t);
              continue;
          }
          if (!t->activeRunConfiguration())
              t->addRunConfiguration(new ProjectExplorer::CustomExecutableRunConfiguration(t));
      }

      refreshCppCodeModel(true);
      return RestoreResult::Ok;
}

void ROSProject::buildQueueFinished(bool success)
{
    refreshCppCodeModel(success);
}

} // namespace Internal
} // namespace GenericProjectManager
