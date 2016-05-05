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

#ifndef ROSPROJECTCONSTANTS_H
#define ROSPROJECTCONSTANTS_H
#include <QStringList>

namespace ROSProjectManager {
namespace Constants {

// Project
const char ROSPROJECT_ID[]  = "ROSProjectManager.ROSProject";
const char PROJECTCONTEXT[]     = "ROSProject.ProjectContext";
const char ROSMIMETYPE[]    = "application/ros.project";

// ROS default install directory
const char ROS_INSTALL_DIRECTORY[] = "/opt/ros";

// Context menu actions
const char ROS_RELOAD_INCLUDE_DIRS[] = "ROSProjectManager.reloadProjectIncludeDirectories";
const char ROS_REMOVE_DIR[] = "ROSProjectManager.removeDirectory";

// ROS wizards constants
const char ROS_WIZARD_CATEGORY[] = "A.ROS";
const char ROS_WIZARD_CATEGORY_DISPLAY[] = "ROS";

// Project Exclude Extension
const QStringList ROS_EXCLUDE_FILE_EXTENSION = QStringList() << QLatin1Literal("*.autosave");

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
