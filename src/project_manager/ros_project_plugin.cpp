/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "ros_project_plugin.h"

#include "ros_build_configuration.h"
#include "ros_run_configuration.h"
#include "ros_run_steps.h"
#include "ros_project_manager.h"
#include "ros_project_wizard.h"
#include "ros_project_constants.h"
#include "ros_make_step.h"
#include "ros_project.h"
#include "ros_utils.h"
#include "ros_project_constants.h"
#include "ros_package_wizard.h"
#include "remove_directory_dialog.h"

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/removefiledialog.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/selectablefilesmodel.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/compileoutputwindow.h>
#include <projectexplorer/appoutputpane.h>
#include <projectexplorer/projecttreewidget.h>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/invoker.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/mimetypes/mimedatabase.h>

#include <QtPlugin>
#include <QDebug>
#include <QApplication>
#include <QMessageBox>

using namespace Core;
using namespace ProjectExplorer;

namespace ROSProjectManager {
namespace Internal {

bool ROSProjectPlugin::initialize(const QStringList &, QString *errorMessage)
{
    Q_UNUSED(errorMessage)
    Utils::MimeDatabase::addMimeTypes(QLatin1String(":rosproject/ROSProjectManager.mimetypes.xml"));

    addAutoReleasedObject(new ROSManager);
    addAutoReleasedObject(new ROSMakeStepFactory);
    addAutoReleasedObject(new ROSBuildConfigurationFactory);
    addAutoReleasedObject(new ROSRunConfigurationFactory);
    addAutoReleasedObject(new ROSRunControlFactory);
    addAutoReleasedObject(new ROSRunStepFactory);

    IWizardFactory::registerFactoryCreator([]() { return QList<IWizardFactory *>() << new ROSProjectWizard << new ROSPackageWizard; });

    ActionContainer *mproject =
            ActionManager::actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);

    auto reloadProjectIncludeDirectoriesAction = new QAction(tr("Reload Project Include Directories..."), this);
    Command *reloadCommand = ActionManager::registerAction(reloadProjectIncludeDirectoriesAction,
        Constants::ROS_RELOAD_INCLUDE_DIRS, Context(Constants::PROJECTCONTEXT));
    reloadCommand->setAttribute(Command::CA_Hide);
    mproject->addAction(reloadCommand, ProjectExplorer::Constants::G_PROJECT_FILES);
    connect(reloadProjectIncludeDirectoriesAction, &QAction::triggered, this, &ROSProjectPlugin::reloadProjectIncludeDirectories);

    // This will context menu action for deleting and renaming project folders from the ProjectTree.
    ActionContainer *mfolderContextMenu = ActionManager::actionContainer(ProjectExplorer::Constants::M_FOLDERCONTEXT);

    auto removeProjectDirectoryAction = new QAction(tr("Remove Directory..."), this);
    Command *removeCommand = ActionManager::registerAction(removeProjectDirectoryAction,
        Constants::ROS_REMOVE_DIR, Context(Constants::PROJECTCONTEXT));
    removeCommand->setAttribute(Command::CA_Hide);
    mfolderContextMenu->addAction(removeCommand, ProjectExplorer::Constants::G_FOLDER_FILES);
    connect(removeProjectDirectoryAction, &QAction::triggered, this, &ROSProjectPlugin::removeProjectDirectory);

    Command *renameCommand = ActionManager::command(ProjectExplorer::Constants::RENAMEFILE);
    mfolderContextMenu->addAction(renameCommand, ProjectExplorer::Constants::G_FOLDER_FILES);

    return true;
}

void ROSProjectPlugin::reloadProjectIncludeDirectories()
{
    ROSProject *rosProject = qobject_cast<ROSProject *>(ProjectTree::currentProject());
    if (!rosProject)
        return;

    QProcess *runCmake = new QProcess();
    ROSBuildConfiguration *bc = qobject_cast<ROSBuildConfiguration *>(rosProject->activeTarget()->activeBuildConfiguration());

    // Generate CodeBlocks Project File
    if (ROSUtils::sourceWorkspace(runCmake, rosProject->projectDirectory(), bc->rosDistribution()))
    {
      if (ROSUtils::generateCodeBlocksProjectFile(runCmake, rosProject->sourceDirectory(), rosProject->buildDirectory()))
      {
        QStringList projectIncludes = ROSUtils::getWorkspaceIncludes(rosProject->projectDirectory(), bc->rosDistribution());
        rosProject->addIncludes(projectIncludes);
      }
    }

    delete runCmake;
}

void ROSProjectPlugin::removeProjectDirectory()
{
  ProjectExplorer::Node *currentNode = ProjectExplorer::ProjectTree::currentNode();
  QTC_ASSERT(currentNode && currentNode->nodeType() == ProjectExplorer::FolderNodeType, return);

  QString filePath = currentNode->filePath().toString();
  RemoveDirectoryDialog removeDirectoryDialog(filePath, ICore::mainWindow());

  if (removeDirectoryDialog.exec() == QDialog::Accepted)
  {
      const bool deleteDirectory = removeDirectoryDialog.isDeleteDirectoryChecked();
      if (deleteDirectory)
          QDir(filePath).removeRecursively();
  }
}

} // namespace Internal
} // namespace ROSProjectManager
