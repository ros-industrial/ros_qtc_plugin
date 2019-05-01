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
#include <QRegularExpression>

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
        Core::MessageManager::write("[ROS Warning] Update Project Tree Failed, Results returned null pointer.");
        return;
    }
    setRootProjectNode(std::move(pn));

    if (!m_asyncUpdateFutureInterface)
    {
        Core::MessageManager::write("[ROS Warning] Update Project Tree Failed, Async Futur Interface was null.");
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

  emitParsingFinished(true);
}

void ROSProject::buildProjectTree(const Utils::FileName projectFilePath, const QStringList watchDirectories, QFutureInterface<FutureWatcherResults> &fi)
{
    fi.reportStarted();

    FutureWatcherResults results;

    for (const QString& dir : watchDirectories) {
        Utils::FileName addedDir = projectFilePath.parentDir().appendPath(dir);
        QHash<QString, ROSUtils::FolderContent> newDirContent = ROSUtils::getFolderContentRecurisve(addedDir, results.files, results.directories);
        results.workspaceContent.unite(newDirContent);
    }

    ROSProjectNode* project_node(new ROSProjectNode(projectFilePath.parentDir()));
    std::unique_ptr<FileNode> root_node(new FileNode(projectFilePath, FileType::Project, false));
    project_node->addNode(std::move(root_node));

    const ProjectExplorer::FolderNode::FolderNodeFactory &factory = [](const Utils::FileName &fn) { return std::make_unique<ROSFolderNode>(fn, fn.fileName()); };

    std::vector<std::unique_ptr<ProjectExplorer::FileNode>> childNodes;
    QHashIterator<QString, ROSUtils::FolderContent> item(results.workspaceContent);
    int cnt = 0;
    double max = results.workspaceContent.size();
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
    results.node = project_node;

    fi.setProgressValue(fi.progressMaximum());
    fi.reportResult(std::move(results));
    fi.reportFinished();
}

void ROSProject::updateEnvironment()
{
  if (ROSBuildConfiguration *bc = rosBuildConfiguration())
      bc->updateQtEnvironment(m_wsEnvironment);
}

void ROSProject::fileSystemChanged(const QString &path)
{
  const ROSUtils::FolderContent& pre_content = m_workspaceContent[path];

  QStringList folderNameFilters, fileNameFilters;
  ROSUtils::getDefaultFolderContentFilters(folderNameFilters, fileNameFilters);
  const ROSUtils::FolderContent& cur_content = ROSUtils::getFolderContent(path, folderNameFilters, fileNameFilters);

  QSet<QString> pre_content_files = QSet<QString>::fromList(pre_content.files);
  QSet<QString> pre_content_dirs = QSet<QString>::fromList(pre_content.directories);

  QSet<QString> cur_content_files = QSet<QString>::fromList(cur_content.files);
  QSet<QString> cur_content_dirs = QSet<QString>::fromList(cur_content.directories);

  // Files that have been added
  QSet<QString> files_diff = cur_content_files - pre_content_files;
  QSet<QString> dirs_diff = cur_content_dirs - pre_content_dirs;

  // This is to check if untracked dirs or files were added. If so do not start timer.
  if (files_diff.empty() && dirs_diff.empty())
    return;

  m_asyncUpdateTimer.setInterval(UPDATE_INTERVAL);
  m_asyncUpdateTimer.start();
}

void ROSProject::asyncUpdate()
{
  if (!isParsing())
      emitParsingStarted();

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

  Utils::runAsync(ProjectExplorer::ProjectExplorerPlugin::sharedThreadPool(), QThread::LowestPriority, [this]() { ROSProject::buildProjectTree(projectFilePath(), m_projectFileContent.watchDirectories, *m_asyncUpdateFutureInterface); });
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

        ROSUtils::WorkspaceInfo workspaceInfo = ROSUtils::getWorkspaceInfo(projectDirectory(), rosBuildConfiguration()->buildSystem(), distribution());

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
          Utils::runAsync(ProjectExplorer::ProjectExplorerPlugin::sharedThreadPool(), QThread::LowestPriority, [this, workspaceInfo, k]() { ROSProject::buildCppCodeModel(workspaceInfo, projectFilePath(), m_workspaceFiles, k, m_wsPackageInfo, m_wsPackageBuildInfo, *m_asyncBuildCodeModelFutureInterface); });
        }
        else
        {
          ROSProject::buildCppCodeModel(workspaceInfo, projectFilePath(), m_workspaceFiles, k, m_wsPackageInfo, m_wsPackageBuildInfo, *m_asyncBuildCodeModelFutureInterface);
        }
    }
}

void ROSProject::buildCppCodeModel(const ROSUtils::WorkspaceInfo workspaceInfo,
                                   const Utils::FileName projectFilePath,
                                   const QStringList workspaceFiles,
                                   const Kit *k,
                                   const ROSUtils::PackageInfoMap wsPackageInfo,
                                   const ROSUtils::PackageBuildInfoMap wsPackageBuildInfo,
                                   QFutureInterface<CppToolsFutureResults> &fi)
{
    CppToolsFutureResults results;
    results.wsPackageInfo = ROSUtils::getWorkspacePackageInfo(workspaceInfo, &wsPackageInfo);
    results.wsPackageBuildInfo = ROSUtils::getWorkspacePackageBuildInfo(workspaceInfo, results.wsPackageInfo, &wsPackageBuildInfo);

    if (results.wsPackageBuildInfo.isEmpty())
        results.wsEnvironment = Utils::Environment(ROSUtils::getWorkspaceEnvironment(workspaceInfo).toStringList());
    else
        results.wsEnvironment = Utils::Environment(results.wsPackageBuildInfo.first().environment);

    const Utils::FileName sysRoot = SysRootKitInformation::sysRoot(k);

    CppTools::ProjectPart::QtVersion activeQtVersion = CppTools::ProjectPart::NoQt;
    if (QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(k))
    {
        if (qtVersion->qtVersion() < QtSupport::QtVersionNumber(5,0,0))
            activeQtVersion = CppTools::ProjectPart::Qt4;
        else
            activeQtVersion = CppTools::ProjectPart::Qt5;
    }

    // This assumes package follow the ros package configuration.
    // Could improve to to parse packges for c++ file and add all directories
    QStringList workspace_includes;
    for (const auto& package : results.wsPackageInfo)
    {
      Utils::FileName include_path = Utils::FileName(package.path);
      include_path.appendPath("include");
      if (include_path.exists())
        workspace_includes.push_back(include_path.toString());

    }

    CppTools::RawProjectParts rpps;

    ToolChain *cxxToolChain = ToolChainKitInformation::toolChain(k, ProjectExplorer::Constants::CXX_LANGUAGE_ID);

    QString pattern = "^.*\\.(" + QRegularExpression::escape("c") +
                            "|" + QRegularExpression::escape("cc") +
                            "|" + QRegularExpression::escape("cpp") +
                            "|" + QRegularExpression::escape("c++") +
                            "|" + QRegularExpression::escape("cp") +
                            "|" + QRegularExpression::escape("cxx") +
                            "|" + QRegularExpression::escape("h") +
                            "|" + QRegularExpression::escape("hh") +
                            "|" + QRegularExpression::escape("hpp") +
                            "|" + QRegularExpression::escape("h++") +
                            "|" + QRegularExpression::escape("hp") +
                            "|" + QRegularExpression::escape("hxx") + ")";

    int cnt = 0;
    double max = results.wsPackageBuildInfo.size();
    for (const ROSUtils::PackageBuildInfo& buildInfo : results.wsPackageBuildInfo)
    {
        QStringList packageFiles = workspaceFiles.filter(buildInfo.parent.path.toString() + QDir::separator());
        QStringList packageCppFiles = packageFiles.filter(QRegularExpression(pattern));

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

            rpp.setProjectFileLocation(projectFilePath.toString());
            rpp.setBuildSystemTarget(buildInfo.parent.name + '|' + targetInfo.name + '|' + projectFilePath.toString());
            rpp.setDisplayName(buildInfo.parent.name + '|' + targetInfo.name);
            rpp.setQtVersion(activeQtVersion);
            rpp.setMacros(ProjectExplorer::Macro::toMacros(defineArg.toUtf8()));

            QSet<QString> toolChainIncludes;
            for (const HeaderPath &hp : cxxToolChain->builtInHeaderPaths(targetInfo.flags, sysRoot)) {
                toolChainIncludes.insert(hp.path);
            }

            QStringList includePaths;
            for (const QString &i : targetInfo.includes) {
                if (!toolChainIncludes.contains(i) && !i.startsWith(workspaceInfo.installPath.toString()) && !i.startsWith(workspaceInfo.develPath.toString()))
                    includePaths.append(i);
            }
            includePaths.append(workspace_includes);

            rpp.setIncludePaths(includePaths);
            rpp.setFlagsForCxx({cxxToolChain, targetInfo.flags});
            rpp.setFiles(packageCppFiles);
            rpps.append(rpp);
        }
        cnt += 1;
        fi.setProgressValue(static_cast<int>(100.0 * static_cast<double>(cnt) / max));
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
    updateEnvironment();

    const Kit *k = nullptr;

    if (Target *target = activeTarget())
        k = target->kit();
    else
        k = KitManager::defaultKit();

    QTC_ASSERT(k, return);

    ToolChain *cxxToolChain = ToolChainKitInformation::toolChain(k, ProjectExplorer::Constants::CXX_LANGUAGE_ID);

    m_wsPackageInfo = std::move(m_futureBuildCodeModelWatcher.result().wsPackageInfo);
    m_wsPackageBuildInfo = std::move(m_futureBuildCodeModelWatcher.result().wsPackageBuildInfo);
    m_wsEnvironment = std::move(m_futureBuildCodeModelWatcher.result().wsEnvironment);

    m_cppCodeModelUpdater->update({this, nullptr, cxxToolChain, k, std::move(m_futureBuildCodeModelWatcher.result().parts)});

    m_asyncBuildCodeModelFutureInterface->reportFinished();
    delete m_asyncBuildCodeModelFutureInterface;
    m_asyncBuildCodeModelFutureInterface = nullptr;
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

      asyncUpdateCppCodeModel(true);
      return RestoreResult::Ok;
}

void ROSProject::buildQueueFinished(bool success)
{
    asyncUpdateCppCodeModel(success);
}

} // namespace Internal
} // namespace GenericProjectManager
