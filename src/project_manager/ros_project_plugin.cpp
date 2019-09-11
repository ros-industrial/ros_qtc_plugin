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
#include "ros_project_plugin.h"
#include "ros_terminal_pane.h"
#include "ros_build_configuration.h"
#include "ros_run_configuration.h"
#include "ros_rosrun_step.h"
#include "ros_roslaunch_step.h"
#include "ros_rosattach_step.h"
#include "ros_rostest_step.h"
#include "ros_catkin_test_results_step.h"
#include "ros_project_wizard.h"
#include "ros_project_constants.h"
#include "ros_catkin_make_step.h"
#include "ros_catkin_tools_step.h"
#include "ros_colcon_step.h"
#include "ros_project.h"
#include "ros_utils.h"
#include "ros_project_constants.h"
#include "ros_package_wizard.h"
#include "ros_settings_page.h"
#include "remove_directory_dialog.h"

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <cpptools/cppcodestylepreferences.h>
#include <cpptools/cpptoolssettings.h>
#include <cpptools/cpptoolsconstants.h>

#include <texteditor/codestylepool.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorsettings.h>

#include <projectexplorer/projectmanager.h>
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
#include <utils/removefiledialog.h>

#include <QtPlugin>
#include <QDebug>
#include <QApplication>
#include <QMessageBox>

using namespace Core;
using namespace ProjectExplorer;

namespace ROSProjectManager {
namespace Internal {

static ROSProjectPlugin *m_instance = nullptr;

class ROSProjectPluginPrivate
{
public:
    ROSProjectPluginPrivate() :
      settings(new ROSSettings()),
      settingsPage(new ROSSettingsPage(settings))
    {
      settings->fromSettings(ICore::settings());

      runDebugWorkerFactory.addSupportedRunMode(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    }

    ROSRunConfigurationFactory runConfigFactory;
    ROSRunStepFactory rosRunStepFactory;
    ROSLaunchStepFactory rosLaunchStepFactory;
    ROSAttachStepFactory rosAttachStepFactory;
    ROSTestStepFactory rosTestStepFactory;
    ROSCatkinTestResultsStepFactory rosCatkinTestResultsStepFactory;

    SimpleRunWorkerFactory<ROSRunWorker, ROSRunConfiguration> runWorkerFactory;
    SimpleRunWorkerFactory<ROSDebugRunWorker, ROSRunConfiguration> runDebugWorkerFactory;

    ROSBuildConfigurationFactory buildConfigFactory;
    ROSCatkinMakeStepFactory catkinMakeStepFactory;
    ROSCatkinToolsStepFactory catkinToolsStepFactory;
    ROSColconStepFactory colconStepFactory;

    ROSTerminalPane terminalPane;

    QSharedPointer<ROSSettings> settings;
    QSharedPointer<ROSSettingsPage> settingsPage;
};

ROSProjectPlugin::ROSProjectPlugin() : ExtensionSystem::IPlugin()
{
  m_instance = this;
}

ROSProjectPlugin::~ROSProjectPlugin()
{
  ExtensionSystem::PluginManager::removeObject(&(d->terminalPane));
  delete d;
  m_instance = nullptr;
}

ROSProjectPlugin *ROSProjectPlugin::instance()
{
    return m_instance;
}

bool ROSProjectPlugin::initialize(const QStringList &, QString *errorMessage)
{
    Q_UNUSED(errorMessage);

    d = new ROSProjectPluginPrivate();
    ExtensionSystem::PluginManager::addObject(&(d->terminalPane));

    QFile mimeFilePath(":rosproject/ROSProjectManager.mimetypes.xml");

    if (mimeFilePath.open(QIODevice::ReadOnly)) {
        QByteArray mimeByteArray = mimeFilePath.readAll();

        if( ! mimeByteArray.isEmpty() )
            Utils::addMimeTypes(Constants::ROS_MIME_TYPE, mimeByteArray);
        else { Q_ASSERT(false); }
    }

    ProjectManager::registerProjectType<ROSProject>(Constants::ROS_MIME_TYPE);

    IWizardFactory::registerFactoryCreator([]() { return QList<IWizardFactory *>() << new ROSProjectWizard << new ROSPackageWizard; });

    ActionContainer *mproject = ActionManager::actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);

    auto reloadProjectBuildInfoAction = new QAction(tr("Reload Project Build Info..."), this);
    Command *reloadCommand = ActionManager::registerAction(reloadProjectBuildInfoAction,
                                                           Constants::ROS_RELOAD_BUILD_INFO,
                                                           Context(Constants::ROS_PROJECT_CONTEXT));

    reloadCommand->setAttribute(Command::CA_Hide);
    mproject->addAction(reloadCommand, ProjectExplorer::Constants::G_PROJECT_FILES);
    connect(reloadProjectBuildInfoAction, &QAction::triggered, this, &ROSProjectPlugin::reloadProjectBuildInfo);

    // This will context menu action for deleting and renaming project folders from the ProjectTree.
    ActionContainer *mfolderContextMenu = ActionManager::actionContainer(ProjectExplorer::Constants::M_FOLDERCONTEXT);

    auto removeProjectDirectoryAction = new QAction(tr("Remove Directory..."), this);
    Command *removeCommand = ActionManager::registerAction(removeProjectDirectoryAction,
                                                           Constants::ROS_REMOVE_DIR,
                                                           Context(Constants::ROS_PROJECT_CONTEXT));
    removeCommand->setAttribute(Command::CA_Hide);
    mfolderContextMenu->addAction(removeCommand, ProjectExplorer::Constants::G_FOLDER_FILES);
    connect(removeProjectDirectoryAction, &QAction::triggered, this, &ROSProjectPlugin::removeProjectDirectory);

    auto renameFileAction = new QAction(tr("Rename File..."), this);
    Command *renameCommand = ActionManager::registerAction(renameFileAction,
                                                           Constants::ROS_RENAME_FILE,
                                                           Context(Constants::ROS_PROJECT_CONTEXT));

    renameCommand->setAttribute(Command::CA_Hide);

    mfolderContextMenu->addAction(renameCommand, ProjectExplorer::Constants::G_FOLDER_FILES);

    createCppCodeStyle();

    return true;
}

QTermWidget &ROSProjectPlugin::startTerminal(int startnow, const QString name)
{
  return d->terminalPane.startTerminal(startnow, name);
}

QSharedPointer<ROSSettings> ROSProjectPlugin::settings() const
{
    return d->settings;
}

void ROSProjectPlugin::createCppCodeStyle()
{
  TextEditor::CodeStylePool *pool = TextEditor::TextEditorSettings::codeStylePool(CppTools::Constants::CPP_SETTINGS_ID);

  // ROS style
  CppTools::CppCodeStylePreferences *rosCodeStyle = new CppTools::CppCodeStylePreferences();
  rosCodeStyle->setId(Constants::ROS_CPP_CODE_STYLE_ID);
  rosCodeStyle->setDisplayName(tr("ROS"));
  rosCodeStyle->setReadOnly(true);

  TextEditor::TabSettings rosTabSettings;
  rosTabSettings.m_tabPolicy = TextEditor::TabSettings::SpacesOnlyTabPolicy;
  rosTabSettings.m_tabSize = 2;
  rosTabSettings.m_indentSize = 2;
  rosTabSettings.m_continuationAlignBehavior = TextEditor::TabSettings::ContinuationAlignWithIndent;
  rosCodeStyle->setTabSettings(rosTabSettings);

  CppTools::CppCodeStyleSettings rosCodeStyleSettings;
  rosCodeStyleSettings.alignAssignments = false;
  rosCodeStyleSettings.bindStarToIdentifier = true;
  rosCodeStyleSettings.bindStarToLeftSpecifier = false;
  rosCodeStyleSettings.bindStarToRightSpecifier = false;
  rosCodeStyleSettings.bindStarToTypeName = false;
  rosCodeStyleSettings.extraPaddingForConditionsIfConfusingAlign = true;
  rosCodeStyleSettings.indentAccessSpecifiers = false;
  rosCodeStyleSettings.indentBlockBody = true;
  rosCodeStyleSettings.indentBlockBraces = false;
  rosCodeStyleSettings.indentBlocksRelativeToSwitchLabels = false;
  rosCodeStyleSettings.indentClassBraces = false;
  rosCodeStyleSettings.indentControlFlowRelativeToSwitchLabels = true;
  rosCodeStyleSettings.indentDeclarationsRelativeToAccessSpecifiers = true;
  rosCodeStyleSettings.indentEnumBraces = false;
  rosCodeStyleSettings.indentFunctionBody = true;
  rosCodeStyleSettings.indentFunctionBraces = false;
  rosCodeStyleSettings.indentNamespaceBody = false;
  rosCodeStyleSettings.indentNamespaceBraces = false;
  rosCodeStyleSettings.indentStatementsRelativeToSwitchLabels = true;
  rosCodeStyleSettings.indentSwitchLabels = false;
  rosCodeStyle->setCodeStyleSettings(rosCodeStyleSettings);

  pool->addCodeStyle(rosCodeStyle);

  // Since the ROS Cpp code style can not be added until after the CppToolsSettings instance is create
  // the Cpp code style must be reloaded from settings to capture if it is set ROS Cpp code style.
  QSettings *s = Core::ICore::settings();
  CppTools::CppCodeStylePreferences *originalCppCodeStylePreferences = CppTools::CppToolsSettings::instance()->cppCodeStyle();
  originalCppCodeStylePreferences->fromSettings(QLatin1String(CppTools::Constants::CPP_SETTINGS_ID), s);
}

void ROSProjectPlugin::reloadProjectBuildInfo()
{
    if (ROSProject *rosProject = qobject_cast<ROSProject *>(ProjectTree::currentProject()))
        rosProject->asyncUpdateCppCodeModel(true);
}

void ROSProjectPlugin::removeProjectDirectory()
{
  ProjectExplorer::Node *currentNode = ProjectExplorer::ProjectTree::currentNode();

  QTC_ASSERT(currentNode && currentNode->isFolderNodeType(), return);

  QString filePath = currentNode->filePath().toString();
  RemoveDirectoryDialog removeDirectoryDialog(filePath, ICore::mainWindow());

  if (removeDirectoryDialog.exec() == QDialog::Accepted)
  {
      const bool deleteDirectory = removeDirectoryDialog.isDeleteDirectoryChecked();
      if (deleteDirectory)
          QDir(filePath).removeRecursively();
  }
}

void ROSProjectPlugin::renameFile()
{
    ///TODO: Rename File here
    // ProjectNode::renameFile();
}

} // namespace Internal
} // namespace ROSProjectManager
