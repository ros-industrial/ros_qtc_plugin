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
#ifndef ROSPROJECTCONSTANTS_H
#define ROSPROJECTCONSTANTS_H
#include <QStringList>

// clazy:excludeall=non-pod-global-static

namespace ROSProjectManager {
namespace Constants {

// Project
static constexpr char ROS_PROJECT_ID[]  = "ROSProjectManager.ROSProject";
static constexpr char ROS_PROJECT_CONTEXT[] = "ROSProject.ProjectContext";
static constexpr char ROS_PROJECT_FILE_ID[] = "ROSProject.ProjectFile";
static constexpr char ROS_MIME_TYPE[]    = "application/ros.project";
static constexpr char ROS_DEFAULT_WORKING_DIR[] = "%{CurrentProject:Path}";

// Tasks
static constexpr char ROS_READING_PROJECT[] = "ROSProjectManager.ReadingProject";
static constexpr char ROS_RELOADING_BUILD_INFO[] = "ROSProjectManager.ReloadingBuildInfo";

// ROS default install directory
static const QString ROS_SOURCE_FILE_SHELL = "setup.sh";
static const QString ROS_SOURCE_FILE_BASH = "setup.bash";
static const QString ROS_SOURCE_FILE_ZSH = "setup.zsh";
static const QString ROS_SOURCE_FILE_BAT = "setup.bat";
#if defined(Q_OS_WIN)
static constexpr char ROS_INSTALL_DIRECTORY[] = "C:/ros";
static const QString ROS_SOURCE_FILE = ROS_SOURCE_FILE_BAT;
#elif defined(Q_OS_MAC)
static constexpr char ROS_INSTALL_DIRECTORY[] = "/usr/local/opt/ros";
static const QString ROS_SOURCE_FILE = ROS_SOURCE_FILE_ZSH;
#else
static constexpr char ROS_INSTALL_DIRECTORY[] = "/opt/ros";
static const QString ROS_SOURCE_FILE = ROS_SOURCE_FILE_BASH;
#endif

// Context menu actions
static constexpr char ROS_RELOAD_BUILD_INFO[] = "ROSProjectManager.reloadProjectBuildInfo";
static constexpr char ROS_REMOVE_DIR[] = "ROSProjectManager.removeDirectory";
static constexpr char ROS_RENAME_FILE[] = "ROSProjectManager.renameFile";

// ROS wizards constants
static constexpr char ROS_WIZARD_CATEGORY[] = "A.ROS";
static constexpr char ROS_WIZARD_CATEGORY_DISPLAY[] = "ROS";

// ROS run step constants
static constexpr char ROS_RUN_STEP_LIST_ID[] = "ROSProjectManager.RunStepList";
static constexpr char ROS_RUN_STEPS_PAGE_ID[] = "ROSProjectManager.RunSteps";
static constexpr char ROS_LAUNCH_ID[] = "ROSProjectManager.ROSLaunchStep";
static constexpr char ROS_RUN_ID[] = "ROSProjectManager.ROSRunStep";
static constexpr char ROS_ATTACH_TO_NODE_ID[] = "ROSProjectManager.ROSAttachToNode";
static constexpr char ROS_TEST_ID[] = "ROSProjectManager.ROSTestStep";
static constexpr char ROS_CATKIN_TEST_RESULTS_ID[] = "ROSProjectManager.ROSCatkinTestResultsStep";

// Project Exclude Extension
static const QStringList ROS_EXCLUDE_FILE_EXTENSION = QStringList() << QStringLiteral("*.autosave") << QStringLiteral("*.workspace");

// ROS Cpp Code Style ID
static constexpr char ROS_CPP_CODE_STYLE_ID[] = "ROSProject.CppCodeStyle";

// ROS C++ constants
static constexpr char SOURCE_HEADER_FILE_FILTER[] = "*.c; *.cc; *.cpp; *.c++; *.cp; *.cxx; *.h; *.hh; *.hpp; *.h++; *.hp; *.hxx;";

static const QStringList SOURCE_FILE_EXTENSIONS = QStringList()
                                                  << QStringLiteral("c")
                                                  << QStringLiteral("cc")
                                                  << QStringLiteral("cpp")
                                                  << QStringLiteral("c++")
                                                  << QStringLiteral("cp")
                                                  << QStringLiteral("cxx");

static const QStringList HEADER_FILE_EXTENSIONS = QStringList()
                                                  << QStringLiteral("h")
                                                  << QStringLiteral("hh")
                                                  << QStringLiteral("hpp")
                                                  << QStringLiteral("h++")
                                                  << QStringLiteral("hp")
                                                  << QStringLiteral("hxx");

// ROS Settings Widgets
static constexpr char ROS_SETTINGS_GROUP_ID[] = "ROSProjectManager.ROSSettingsGroup";
static constexpr char ROS_SETTINGS_CATEGORY_ID[] = "ROSProjectManager.ROSSettingsCategory";
static constexpr char ROS_SETTINGS_CATEGORY_NAME_ID[] = "ROSProjectManager.ROSSettingsCategoryName";
static constexpr char ROS_SETTINGS_MAIN_PAGE_ID[] = "ROSProjectManager.ROSSettingsMainPage";
static constexpr char ROS_SETTINGS_MAIN_PAGE_NAME_ID[] = "ROSProjectManager.ROSSettingsMainPageName";

} // namespace Constants
} // namespace ROSProjectManager

#endif // ROSPROJECTCONSTANTS_H
