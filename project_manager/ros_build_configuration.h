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

#ifndef ROSBUILDCONFIGURATION_H
#define ROSBUILDCONFIGURATION_H

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/namedwidget.h>

namespace Utils {
class FileName;
class PathChooser;
} // namespace Utils

namespace ROSProjectManager {
namespace Internal {

class ROSBuildConfigurationFactory;
class ROSBuildSettingsWidget;

class ROSBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT
    friend class ROSBuildConfigurationFactory;

public:
    explicit ROSBuildConfiguration(ProjectExplorer::Target *parent);

    ProjectExplorer::NamedWidget *createConfigWidget();

    BuildType buildType() const;

protected:
    ROSBuildConfiguration(ProjectExplorer::Target *parent, ROSBuildConfiguration *source);
    ROSBuildConfiguration(ProjectExplorer::Target *parent, Core::Id id);

    friend class ROSBuildSettingsWidget;
};

class ROSBuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
{
    Q_OBJECT

public:
    explicit ROSBuildConfigurationFactory(QObject *parent = 0);
    ~ROSBuildConfigurationFactory();

    int priority(const ProjectExplorer::Target *parent) const;
    QList<ProjectExplorer::BuildInfo *> availableBuilds(const ProjectExplorer::Target *parent) const;
    int priority(const ProjectExplorer::Kit *k, const QString &projectPath) const;
    QList<ProjectExplorer::BuildInfo *> availableSetups(const ProjectExplorer::Kit *k,
                                                        const QString &projectPath) const;
    ProjectExplorer::BuildConfiguration *create(ProjectExplorer::Target *parent,
                                                const ProjectExplorer::BuildInfo *info) const;

    bool canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const;
    ProjectExplorer::BuildConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source);
    bool canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);

private:
    bool canHandle(const ProjectExplorer::Target *t) const;
    ProjectExplorer::BuildInfo *createBuildInfo(const ProjectExplorer::Kit *k, const Utils::FileName &buildDir) const;
};

class ROSBuildSettingsWidget : public ProjectExplorer::NamedWidget
{
    Q_OBJECT

public:
    ROSBuildSettingsWidget(ROSBuildConfiguration *bc);

private slots:
    void buildDirectoryChanged();
    void environmentHasChanged();

private:
    Utils::PathChooser *m_pathChooser;
    ROSBuildConfiguration *m_buildConfiguration;
};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSBUILDCONFIGURATION_H
