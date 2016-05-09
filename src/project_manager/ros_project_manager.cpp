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

#include "ros_project_manager.h"
#include "ros_project_constants.h"
#include "ros_project.h"

#include <coreplugin/icore.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>

#include <extensionsystem/pluginmanager.h>

#include <QDebug>

namespace ROSProjectManager {
namespace Internal {

static ROSManager *m_instance = 0;

ROSManager::ROSManager()
{
  m_instance = this;
  m_terminalPane = new ROSTerminalPane();
  ExtensionSystem::PluginManager::addObject(m_terminalPane);
}

ROSManager::~ROSManager()
{
  m_instance = 0;
  ExtensionSystem::PluginManager::removeObject(m_terminalPane);
  delete m_terminalPane;
}

ROSManager *ROSManager::instance()
{
  return m_instance;
}

QString ROSManager::mimeType() const
{
    return QLatin1String(Constants::ROSMIMETYPE);
}

ProjectExplorer::Project *ROSManager::openProject(const QString &fileName, QString *errorString)
{
    if (!QFileInfo(fileName).isFile()) {
        if (errorString)
            *errorString = tr("Failed opening project \"%1\": Project is not a file.")
                .arg(fileName);
        return 0;
    }

    return new ROSProject(this, fileName);
}

void ROSManager::registerProject(ROSProject *project)
{
    m_projects.append(project);
}

void ROSManager::unregisterProject(ROSProject *project)
{
    m_projects.removeAll(project);
}

ROSTerminalPane *ROSManager::getTerminalPane()
{
  return m_terminalPane;
}

} // namespace Internal
} // namespace ROSProjectManager
