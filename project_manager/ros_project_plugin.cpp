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

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>

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

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/invoker.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/mimetypes/mimedatabase.h>

#include <QtPlugin>
#include <QDebug>

using namespace Core;
using namespace ProjectExplorer;

namespace ROSProjectManager {
namespace Internal {

bool ROSProjectPlugin::initialize(const QStringList &, QString *errorMessage)
{
    Q_UNUSED(errorMessage)
    Utils::MimeDatabase::addMimeTypes(QLatin1String(":rosproject/ROSProjectManager.mimetypes.xml"));

    addAutoReleasedObject(new Manager);
    addAutoReleasedObject(new ROSMakeStepFactory);
    addAutoReleasedObject(new ROSBuildConfigurationFactory);
    addAutoReleasedObject(new ROSRunConfigurationFactory);
    addAutoReleasedObject(new ROSRunControlFactory);
    addAutoReleasedObject(new ROSRunStepFactory);

    IWizardFactory::registerFactoryCreator([]() { return QList<IWizardFactory *>() << new ROSProjectWizard; });

    ActionContainer *mproject =
            ActionManager::actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);

    auto reloadWorkspaceAction = new QAction(tr("Reload Workspace..."), this);
    Command *command = ActionManager::registerAction(reloadWorkspaceAction,
        "ROSProjectManager.ReloadWorkspace", Context(Constants::PROJECTCONTEXT));
    command->setAttribute(Command::CA_Hide);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_FILES);

    connect(reloadWorkspaceAction, &QAction::triggered, this, &ROSProjectPlugin::reloadWorkspace);

    return true;
}

void ROSProjectPlugin::reloadWorkspace()
{
    ROSProject *rosProject = qobject_cast<ROSProject *>(ProjectTree::currentProject());
    if (!rosProject)
        return;

    QProcess *runCmake = new QProcess();
    ROSBuildConfiguration *bc = qobject_cast<ROSBuildConfiguration *>(rosProject->activeTarget()->activeBuildConfiguration());

//    connect(runCmake, SIGNAL(readyReadStandardOutput()),this, SLOT(parser->stdOutput()));
//    connect(runCmake, SIGNAL(readyReadStandardError()),this, SLOT(slotUpdateStdError()));

//    Need to figure out how to output to the Compile Output Window
//    QObject *app = ExtensionSystem::PluginManager::getObjectByClassName(QLatin1String("ProjectExplorer::Internal::CompileOutputWindow"));
//    ProjectExplorer::Internal::CompileOutputWindow *capp = qobject_cast<ProjectExplorer::Internal::CompileOutputWindow *>(app);

    // Get Workspace Files
    QStringList projectFiles = ROSUtils::getWorkspaceFiles(rosProject->projectDirectory());
    rosProject->setFiles(projectFiles);

    // Generate CodeBlocks Project File
    if (ROSUtils::sourceWorkspace(runCmake, rosProject->projectDirectory(), bc->rosDistribution()))
    {
      if (ROSUtils::generateCodeBlocksProjectFile(runCmake, rosProject->sourceDirectory(), rosProject->buildDirectory()))
      {
        QStringList projectIncludes = ROSUtils::getWorkspaceIncludes(rosProject->projectDirectory());
        rosProject->addIncludes(projectIncludes);
      }
    }

    delete runCmake;
}

} // namespace Internal
} // namespace ROSProjectManager
