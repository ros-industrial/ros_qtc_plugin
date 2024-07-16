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
#include <coreplugin/messagemanager.h>
#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppmodelmanager.h>
#include <cppeditor/projectinfo.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <qtsupport/baseqtversion.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <qtsupport/qtcppkitinfo.h>
#include <qtsupport/qtkitaspect.h>
#include <utils/async.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/algorithm.h>

#include <QDir>
#include <QtWidgets>
#include <QProcessEnvironment>
#include <QtXml/QDomDocument>
#include <QtConcurrentRun>
#include <QRegularExpression>

#include <cppeditor/cppprojectupdater.h>

using namespace Core;
using namespace ProjectExplorer;

namespace ROSProjectManager {
namespace Internal {

static FolderNode *folderNode(const FolderNode *folder, const Utils::FilePath &directory)
{
    return static_cast<FolderNode *>(folder->findChildFolderNode([&](FolderNode *fn) {
        return fn && fn->filePath() == directory;
    }));
}

static FolderNode *recursiveFindOrCreateFolderNode(FolderNode *folder,
                                                   const Utils::FilePath &directory,
                                                   const Utils::FilePath &overrideBaseDir,
                                                   const FolderNode::FolderNodeFactory &factory)
{
    Utils::FilePath path = overrideBaseDir.isEmpty() ? folder->filePath() : overrideBaseDir;

    Utils::FilePath directoryWithoutPrefix;
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
    QStringList parts = directoryWithoutPrefix.toString().split('/', Qt::SkipEmptyParts);
    if (!Utils::HostOsInfo::isWindowsHost() && !isRelative && parts.count() > 0)
        parts[0].prepend('/');

    ProjectExplorer::FolderNode *parent = folder;
    for (const QString &part : parts) {
        path = path.pathAppended(part);
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
const int UPDATE_INTERVAL = 300;

ROSProject::ROSProject(const Utils::FilePath &fileName) :
    ProjectExplorer::Project(Constants::ROS_MIME_TYPE, fileName),
    m_cppCodeModelUpdater(ProjectUpdaterFactory::createCppProjectUpdater()),
    m_project_loaded(false),
    m_asyncUpdateFutureInterface(nullptr),
    m_asyncBuildCodeModelFutureInterface(nullptr)
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

    connect(&m_futureWatcher, &QFutureWatcher<FutureWatcherResults>::finished, this, &ROSProject::updateProjectTree);

    connect(&m_futureBuildCodeModelWatcher, &QFutureWatcher<CppToolsFutureResults>::finished, this, &ROSProject::updateCppCodeModel);

    connect(&m_watcher, SIGNAL(directoryChanged(QString)),
            this, SLOT(fileSystemChanged(QString)));
}

ROSProject::~ROSProject()
{
    m_futureWatcher.cancel();
    m_futureWatcher.waitForFinished();

    m_futureBuildCodeModelWatcher.cancel();
    m_futureBuildCodeModelWatcher.waitForFinished();

    delete m_cppCodeModelUpdater;
    m_cppCodeModelUpdater = nullptr;

    if (m_asyncUpdateFutureInterface) {
        m_asyncUpdateFutureInterface->reportCanceled();
        m_asyncUpdateFutureInterface->reportFinished();
        delete m_asyncUpdateFutureInterface;
    }

    if (m_asyncBuildCodeModelFutureInterface) {
        m_asyncBuildCodeModelFutureInterface->reportCanceled();
        m_asyncBuildCodeModelFutureInterface->reportFinished();
        delete m_asyncBuildCodeModelFutureInterface;
    }
}

bool ROSProject::saveProjectFile()
{
    DocumentManager::expectFileChange(projectFilePath());
    // Make sure we can open the file for writing

    Utils::FileSaver saver(projectFilePath(), QIODevice::Text);
    if (!saver.hasError())
    {
      QXmlStreamWriter workspaceXml(saver.file());
      ROSUtils::generateQtCreatorWorkspaceFile(workspaceXml, m_projectFileContent);
      saver.setResult(&workspaceXml);
    }
    bool result = saver.finalize(ICore::mainWindow());
    DocumentManager::unexpectFileChange(projectFilePath());
    return result;
}

Utils::FilePath ROSProject::distribution() const
{
    return m_projectFileContent.distribution;
}

ROSUtils::BuildSystem ROSProject::defaultBuildSystem() const
{
    return m_projectFileContent.defaultBuildSystem;
}

ROSBuildConfiguration* ROSProject::rosBuildConfiguration() const
{
    if (activeTarget() != nullptr)
      return static_cast<ROSBuildConfiguration *>(activeTarget()->activeBuildConfiguration());
    else
      return nullptr;
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
    std::unique_ptr<ProjectExplorer::ProjectNode> pn(m_futureWatcher.result().node);
    if (!pn)
    {
        Core::MessageManager::writeSilently("[ROS Warning] Update Project Tree Failed, Results returned null pointer.");
        return;
    }
    setRootProjectNode(std::move(pn));

    if (!m_asyncUpdateFutureInterface)
    {
        Core::MessageManager::writeSilently("[ROS Warning] Update Project Tree Failed, Async Futur Interface was null.");
        // TODO Need to print warning
        return;
    }

    m_workspaceContent = std::move(m_futureWatcher.result().workspaceContent);
    m_workspaceFiles = std::move(m_futureWatcher.result().files);
    m_workspaceDirectories = std::move(m_futureWatcher.result().directories);

    m_asyncUpdateFutureInterface->reportFinished();
    delete m_asyncUpdateFutureInterface;
    m_asyncUpdateFutureInterface = nullptr;
    m_watcher.addPaths(m_workspaceDirectories);

    connect(&m_watcher, SIGNAL(directoryChanged(QString)),
            this, SLOT(fileSystemChanged(QString)));

    if (!m_project_loaded)
    {
      m_project_loaded = true;
      asyncUpdateCppCodeModel(true);
    }
  }
}

void ROSProject::buildProjectTree(const Utils::FilePath projectFilePath, const Utils::FilePath& sourcePath, QFutureInterface<FutureWatcherResults> &fi)
{
    fi.reportStarted();

    FutureWatcherResults results;

    results.workspaceContent = ROSUtils::getFolderContentRecursive(sourcePath, results.files, results.directories);

    ROSProjectNode* project_node(new ROSProjectNode(projectFilePath.parentDir()));
    std::unique_ptr<FileNode> root_node(new FileNode(projectFilePath, ProjectExplorer::FileType::Project));
    project_node->addNode(std::move(root_node));

    const ProjectExplorer::FolderNode::FolderNodeFactory &factory = [](const Utils::FilePath &fn) { return std::make_unique<ROSFolderNode>(fn); };

    std::vector<std::unique_ptr<ProjectExplorer::FileNode>> childNodes;
    QHash<QString, ROSUtils::FolderContent>::const_iterator item = results.workspaceContent.constBegin();
    int cnt = 0;
    double max = results.workspaceContent.size();
    while(item != results.workspaceContent.constEnd())
    {
      if (item.value().files.empty()) {
        // This is required so empty directories show up in project tree
        Utils::FilePath empty_directory = Utils::FilePath::fromString(item.key());
        recursiveFindOrCreateFolderNode(project_node, empty_directory, Utils::FilePath(), factory);
      }
      else {
        // Add all files in the directory node
        for (const QString& file : item.value().files)
        {
          QFileInfo fileInfo(QDir(item.key()), file);

          ProjectExplorer::FileType fileType = ProjectExplorer::FileType::Source;

          if (Constants::HEADER_FILE_EXTENSIONS.contains(fileInfo.suffix()))
            fileType = ProjectExplorer::FileType::Header;

          std::unique_ptr<ProjectExplorer::FileNode> fileNode(new ProjectExplorer::FileNode(Utils::FilePath::fromString(fileInfo.absoluteFilePath()), fileType));
          childNodes.emplace_back(std::move(fileNode));
        }
      }

      cnt += 1;
      fi.setProgressValue(static_cast<int>(100.0 * static_cast<double>(cnt) / max));
      ++item;
    }

    project_node->addNestedNodes(std::move(childNodes), Utils::FilePath(), factory);
    results.node = project_node;

    fi.setProgressValue(fi.progressMaximum());
    fi.reportResult(std::move(results));
    fi.reportFinished();
}

void ROSProject::updateEnvironment()
{
  if (ROSBuildConfiguration *bc = rosBuildConfiguration())
  {
    ROSUtils::WorkspaceInfo workspaceInfo = ROSUtils::getWorkspaceInfo(projectDirectory(), bc->rosBuildSystem(), distribution());
    bc->updateQtEnvironment(Utils::Environment(ROSUtils::getWorkspaceEnvironment(workspaceInfo, bc->environment()).toStringList()));
  }
}

void ROSProject::fileSystemChanged(const QString &path)
{
  const ROSUtils::FolderContent& pre_content = m_workspaceContent[path];

  QStringList folderNameFilters, fileNameFilters;
  ROSUtils::getDefaultFolderContentFilters(folderNameFilters, fileNameFilters);
  const ROSUtils::FolderContent& cur_content = ROSUtils::getFolderContent(path, folderNameFilters, fileNameFilters);

  QSet<QString> pre_content_files(pre_content.files.begin(), pre_content.files.end());
  QSet<QString> pre_content_dirs(pre_content.directories.begin(), pre_content.directories.end());

  QSet<QString> cur_content_files(cur_content.files.begin(), cur_content.files.end());
  QSet<QString> cur_content_dirs(cur_content.directories.begin(), cur_content.directories.end());

  // Content that has been added
  QSet<QString> files_added = cur_content_files - pre_content_files;
  QSet<QString> dirs_added = cur_content_dirs - pre_content_dirs;

  // Content that has been removed
  QSet<QString> files_removed = pre_content_files - cur_content_files;
  QSet<QString> dirs_removed = pre_content_dirs - cur_content_dirs;

  // This is to check if untracked dirs or files were added. If so do not start timer.
  if (files_added.empty() && dirs_added.empty() && files_removed.empty() && dirs_removed.empty())
    return;

  m_asyncUpdateTimer.setInterval(UPDATE_INTERVAL);
  m_asyncUpdateTimer.start();
}

void ROSProject::asyncUpdate()
{
  ROSBuildConfiguration *bc = rosBuildConfiguration();
  if (!bc)
    return;

  bc->buildSystem()->requestParse();

  m_futureWatcher.waitForFinished();

  Q_ASSERT(!m_asyncUpdateFutureInterface);

  m_asyncUpdateFutureInterface = new QFutureInterface<FutureWatcherResults>();

  disconnect(&m_watcher, SIGNAL(directoryChanged(QString)),
             this, SLOT(fileSystemChanged(QString)));

  if (!m_watcher.directories().empty())
    m_watcher.removePaths(m_watcher.directories());

  m_asyncUpdateFutureInterface->setProgressRange(0, 100);
  Core::ProgressManager::addTask(m_asyncUpdateFutureInterface->future(),
                                 tr("Reading Project \"%1\"").arg(displayName()),
                                 Constants::ROS_READING_PROJECT);

  m_futureWatcher.setFuture(m_asyncUpdateFutureInterface->future());

  Utils::asyncRun(ProjectExplorer::ProjectExplorerPlugin::sharedThreadPool(), QThread::LowestPriority,
    [this, bc]() {
      Utils::FilePath sourcePath = ROSUtils::getWorkspaceInfo(projectDirectory(), bc->rosBuildSystem(), distribution()).sourcePath;
      ROSProject::buildProjectTree(projectFilePath(), sourcePath, *m_asyncUpdateFutureInterface);
    });
}

void ROSProject::asyncUpdateCppCodeModel(bool success)
{
    if (success && !m_workspaceFiles.empty() && (rosBuildConfiguration() != nullptr))
    {
        bool async = false;

        m_cppCodeModelUpdater->cancel();

        m_futureBuildCodeModelWatcher.waitForFinished();

        Q_ASSERT(!m_asyncBuildCodeModelFutureInterface);
        m_asyncBuildCodeModelFutureInterface = new QFutureInterface<CppToolsFutureResults>();

        m_asyncBuildCodeModelFutureInterface->setProgressRange(0, 100);
        Core::ProgressManager::addTask(m_asyncBuildCodeModelFutureInterface->future(),
                                       tr("Parsing Build Files for Project \"%1\"").arg(displayName()),
                                       Constants::ROS_RELOADING_BUILD_INFO);

        ROSUtils::WorkspaceInfo workspaceInfo = ROSUtils::getWorkspaceInfo(projectDirectory(), rosBuildConfiguration()->rosBuildSystem(), distribution());
        Utils::Environment current_environment = rosBuildConfiguration()->environment();

        const Kit *k = nullptr;

        if (Target *target = activeTarget())
            k = target->kit();
        else
            k = KitManager::defaultKit();

        QTC_ASSERT(k, return);

        m_futureBuildCodeModelWatcher.setFuture(m_asyncBuildCodeModelFutureInterface->future());

        // TODO: Figure out why running this async causes segfaults
        if (async)
        {
          Utils::asyncRun(ProjectExplorer::ProjectExplorerPlugin::sharedThreadPool(), QThread::LowestPriority, [this, workspaceInfo, k, current_environment]() { ROSProject::buildCppCodeModel(workspaceInfo, projectFilePath(), m_workspaceFiles, k, current_environment, m_wsPackageInfo, m_wsPackageBuildInfo, *m_asyncBuildCodeModelFutureInterface); });
        }
        else
        {
          ROSProject::buildCppCodeModel(workspaceInfo, projectFilePath(), m_workspaceFiles, k, current_environment, m_wsPackageInfo, m_wsPackageBuildInfo, *m_asyncBuildCodeModelFutureInterface);
        }
    }
}

void ROSProject::buildCppCodeModel(const ROSUtils::WorkspaceInfo workspaceInfo,
                                   const Utils::FilePath projectFilePath,
                                   const QStringList /*workspaceFiles*/,
                                   const Kit *k,
                                   const Utils::Environment &env,
                                   const ROSUtils::PackageInfoMap wsPackageInfo,
                                   const ROSUtils::PackageBuildInfoMap wsPackageBuildInfo,
                                   QFutureInterface<CppToolsFutureResults> &fi)
{
    CppToolsFutureResults results;
    results.wsPackageInfo = ROSUtils::getWorkspacePackageInfo(workspaceInfo, &wsPackageInfo);
    results.wsPackageBuildInfo = ROSUtils::getWorkspacePackageBuildInfo(workspaceInfo, results.wsPackageInfo, &wsPackageBuildInfo);

    const Utils::FilePath sysRoot = SysRootKitAspect::sysRoot(k);

    Utils::QtMajorVersion activeQtVersion = Utils::QtMajorVersion::None;
    if (QtSupport::QtVersion *qtVersion = QtSupport::QtKitAspect::qtVersion(k)) {
        if (qtVersion->qtVersion() < QVersionNumber(5,0,0))
            activeQtVersion = Utils::QtMajorVersion::Qt4;
        else if (qtVersion->qtVersion() < QVersionNumber(6,0,0))
            activeQtVersion = Utils::QtMajorVersion::Qt5;
        else
            activeQtVersion = Utils::QtMajorVersion::Qt6;
    }

    // Get all of the workspace includes directories
    QStringList workspace_includes; // This should be the same as workspace_header_paths used for checking for duplicates
    ProjectExplorer::HeaderPaths workspace_header_paths;
    for (const auto& package : qAsConst(results.wsPackageInfo)) {
      Utils::FilePath include_path = Utils::FilePath::fromString(package.path.toString());
      include_path = include_path.pathAppended("include");
      if (!workspace_includes.contains(include_path.toString())) {
        workspace_includes.append(include_path.toString());
        workspace_header_paths.append(ProjectExplorer::HeaderPath(include_path.toString(), ProjectExplorer::HeaderPathType::User));
      }
    }

    ProjectExplorer::RawProjectParts rpps;

    const Toolchain *cxxToolChain = ToolchainKitAspect::cxxToolchain(k);

    if (cxxToolChain)
    {
        int cnt = 0;
        double max = results.wsPackageBuildInfo.size();
        for (const ROSUtils::PackageBuildInfo& buildInfo : qAsConst(results.wsPackageBuildInfo))
        {
            ProjectExplorer::HeaderPaths packageHeaderPaths = workspace_header_paths;
            QStringList package_includes = workspace_includes; // This should be the same as packageHeaderPaths and is used to check for duplicates

            for (const ROSUtils::PackageTargetInfoPtr& targetInfo : buildInfo.targets)
            {
                ProjectExplorer::RawProjectPart rpp;
                const QString defineArg
                        = Utils::transform(targetInfo->defines, [](const QString &s) -> QString {
                            QString result = QString::fromLatin1("#define ") + s;
                            int assignIndex = result.indexOf('=');
                            if (assignIndex != -1)
                                result[assignIndex] = ' ';
                            return result;
                        }).join('\n');

                rpp.setProjectFileLocation(projectFilePath.toString());
                rpp.setBuildSystemTarget(buildInfo.parent.name + '|' + targetInfo->name + '|' + projectFilePath.toString());
                rpp.setDisplayName(buildInfo.parent.name + '|' + targetInfo->name);
                rpp.setQtVersion(activeQtVersion);
                rpp.setMacros(ProjectExplorer::Macro::toMacros(defineArg.toUtf8()));

                QSet<QString> toolChainIncludes;
                const HeaderPaths header_paths = \
                        cxxToolChain->createBuiltInHeaderPathsRunner(env)\
                        (targetInfo->flags, sysRoot, QString());
                for (const HeaderPath &hp : header_paths) {
                    toolChainIncludes.insert(hp.path);
                }

                for (const QString &i : targetInfo->includes) {
                    if (!toolChainIncludes.contains(i) && !package_includes.contains(i)) {
                        packageHeaderPaths.append(ProjectExplorer::HeaderPath(i, ProjectExplorer::HeaderPathType::System));
                        package_includes.append(i);
                    }
                }

                rpp.setFlagsForCxx({cxxToolChain, targetInfo->flags, sysRoot});
                rpp.setFiles(targetInfo->source_files);
                rpp.setHeaderPaths(packageHeaderPaths);
                rpps.append(rpp);
            }
            cnt += 1;
            fi.setProgressValue(static_cast<int>(100.0 * static_cast<double>(cnt) / max));
        }
    }

    results.parts = std::move(rpps);
    fi.setProgressValue(fi.progressMaximum());
    fi.reportResult(results);
    fi.reportFinished();
}

void ROSProject::updateCppCodeModel()
{
  if (m_futureBuildCodeModelWatcher.isFinished())
  {
    m_wsPackageInfo = std::move(m_futureBuildCodeModelWatcher.result().wsPackageInfo);
    m_wsPackageBuildInfo = std::move(m_futureBuildCodeModelWatcher.result().wsPackageBuildInfo);

    QtSupport::CppKitInfo kitInfo(this->activeTarget()->kit());
    QTC_ASSERT(kitInfo.isValid(), return);

    m_cppCodeModelUpdater->update({this, kitInfo, rosBuildConfiguration()->environment(), m_futureBuildCodeModelWatcher.result().parts}, {});

    m_asyncBuildCodeModelFutureInterface->reportFinished();
    delete m_asyncBuildCodeModelFutureInterface;
    m_asyncBuildCodeModelFutureInterface = nullptr;

    updateEnvironment();
  }

}

Project::RestoreResult ROSProject::fromMap(const Utils::Store &map, QString *errorMessage)
{
      RestoreResult result = Project::fromMap(map, errorMessage);
      if (result != RestoreResult::Ok)
          return result;

      Kit *defaultKit = KitManager::defaultKit();
      if (!activeTarget() && defaultKit)
          addTargetForKit(defaultKit);

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

      asyncUpdateCppCodeModel(true);
      return RestoreResult::Ok;
}

void ROSProject::buildQueueFinished(bool success)
{
    asyncUpdateCppCodeModel(success);
}

} // namespace Internal
} // namespace GenericProjectManager
