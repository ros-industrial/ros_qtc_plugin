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

namespace ROSProjectManager {
namespace Constants {

// Project
const char ROS_PROJECT_ID[]  = "ROSProjectManager.ROSProject";
const char ROS_PROJECT_CONTEXT[] = "ROSProject.ProjectContext";
const char ROS_PROJECT_FILE_ID[] = "ROSProject.ProjectFile";
const char ROS_MIME_TYPE[]    = "application/ros.project";
const char ROS_DEFAULT_WORKING_DIR[] = "%{CurrentProject:Path}";

// Tasks
const char ROS_READING_PROJECT[] = "ROSProjectManager.ReadingProject";
const char ROS_RELOADING_BUILD_INFO[] = "ROSProjectManager.ReloadingBuildInfo";

// ROS default install directory
const char ROS_INSTALL_DIRECTORY[] = "/opt/ros";

// Context menu actions
const char ROS_RELOAD_BUILD_INFO[] = "ROSProjectManager.reloadProjectBuildInfo";
const char ROS_REMOVE_DIR[] = "ROSProjectManager.removeDirectory";
const char ROS_RENAME_FILE[] = "ROSProjectManager.renameFile";

// ROS wizards constants
const char ROS_WIZARD_CATEGORY[] = "A.ROS";
const char ROS_WIZARD_CATEGORY_DISPLAY[] = "ROS";

// ROS run step constants
const char ROS_RUN_STEP_LIST_ID[] = "ROSProjectManager.RunStepList";
const char ROS_RUN_STEPS_PAGE_ID[] = "ROSProjectManager.RunSteps";
const char ROS_LAUNCH_ID[] = "ROSProjectManager.ROSLaunchStep";
const char ROS_RUN_ID[] = "ROSProjectManager.ROSRunStep";
const char ROS_ATTACH_TO_NODE_ID[] = "ROSProjectManager.ROSAttachToNode";
const char ROS_TEST_ID[] = "ROSProjectManager.ROSTestStep";
const char ROS_CATKIN_TEST_RESULTS_ID[] = "ROSProjectManager.ROSCatkinTestResultsStep";

// Project Exclude Extension
const QStringList ROS_EXCLUDE_FILE_EXTENSION = QStringList() << QLatin1String("*.autosave") << QLatin1String("*.workspace");

// ROS Cpp Code Style ID
const char ROS_CPP_CODE_STYLE_ID[] = "ROSProject.CppCodeStyle";

// ROS C++ constants
const char SOURCE_HEADER_FILE_FILTER[] = "*.c; *.cc; *.cpp; *.c++; *.cp; *.cxx; *.h; *.hh; *.hpp; *.h++; *.hp; *.hxx;";

const QStringList SOURCE_FILE_EXTENSIONS = QStringList() << QLatin1String("c")
                                                         << QLatin1String("cc")
                                                         << QLatin1String("cpp")
                                                         << QLatin1String("c++")
                                                         << QLatin1String("cp")
                                                         << QLatin1String("cxx");

const QStringList HEADER_FILE_EXTENSIONS = QStringList() << QLatin1String("h")
                                                         << QLatin1String("hh")
                                                         << QLatin1String("hpp")
                                                         << QLatin1String("h++")
                                                         << QLatin1String("hp")
                                                         << QLatin1String("hxx");

// ROS Settings Widgets
const char ROS_SETTINGS_GROUP_ID[] = "ROSProjectManager.ROSSettingsGroup";
const char ROS_SETTINGS_CATEGORY_ID[] = "ROSProjectManager.ROSSettingsCategory";
const char ROS_SETTINGS_CATEGORY_NAME_ID[] = "ROSProjectManager.ROSSettingsCategoryName";
const char ROS_SETTINGS_MAIN_PAGE_ID[] = "ROSProjectManager.ROSSettingsMainPage";
const char ROS_SETTINGS_MAIN_PAGE_NAME_ID[] = "ROSProjectManager.ROSSettingsMainPageName";

} // namespace Constants
} // namespace ROSProjectManager

#endif // ROSPROJECTCONSTANTS_H
