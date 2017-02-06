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

// Tasks
const char ROS_READING_PROJECT[] = "ROSProjectManager.ReadingProject";
const char ROS_RELOADING_BUILD_INFO[] = "ROSProjectManager.ReloadingBuildInfo";


// ROS default install directory
const char ROS_INSTALL_DIRECTORY[] = "/opt/ros";

// Context menu actions
const char ROS_RELOAD_BUILD_INFO[] = "ROSProjectManager.reloadProjectBuildInfo";
const char ROS_REMOVE_DIR[] = "ROSProjectManager.removeDirectory";

// ROS wizards constants
const char ROS_WIZARD_CATEGORY[] = "A.ROS";
const char ROS_WIZARD_CATEGORY_DISPLAY[] = "ROS";

// Project Exclude Extension
const QStringList ROS_EXCLUDE_FILE_EXTENSION = QStringList() << QLatin1Literal("*.autosave");

// ROS Cpp Code Style ID
const char ROS_CPP_CODE_STYLE_ID[] = "ROSProject.CppCodeStyle";

// ROS C++ constants
const char SOURCE_HEADER_FILE_FILTER[] = "*.c; *.cc; *.cpp; *.c++; *.cp; *.cxx; *.h; *.hh; *.hpp; *.h++; *.hp; *.hxx;";

const QStringList SOURCE_FILE_EXTENSIONS = QStringList() << QLatin1Literal("c")
                                                         << QLatin1Literal("cc")
                                                         << QLatin1Literal("cpp")
                                                         << QLatin1Literal("c++")
                                                         << QLatin1Literal("cp")
                                                         << QLatin1Literal("cxx");

const QStringList HEADER_FILE_EXTENSIONS = QStringList() << QLatin1Literal("h")
                                                         << QLatin1Literal("hh")
                                                         << QLatin1Literal("hpp")
                                                         << QLatin1Literal("h++")
                                                         << QLatin1Literal("hp")
                                                         << QLatin1Literal("hxx");

} // namespace Constants
} // namespace ROSProjectManager

#endif // ROSPROJECTCONSTANTS_H
