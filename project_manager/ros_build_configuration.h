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
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/environmentwidget.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>
#include <QCheckBox>

namespace Utils {
class FileName;
class PathChooser;
} // namespace Utils

namespace ROSProjectManager {
namespace Internal {

class ROSBuildConfigurationFactory;
class ROSBuildSettingsWidget;
class ROSBuildInfo;
namespace Ui { class ROSBuildConfiguration; }

class ROSBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT
    friend class ROSBuildConfigurationFactory;

public:
    explicit ROSBuildConfiguration(ProjectExplorer::Target *parent);

    ProjectExplorer::NamedWidget *createConfigWidget();
    QList<ProjectExplorer::NamedWidget *> createSubConfigWidgets();

    BuildType buildType() const;

    QVariantMap toMap() const;

    void setInitialArguments(const QString &arguments);
    QString initialArguments() const;

    void setDevelDirectory(const Utils::FileName &dir);
    Utils::FileName develDirectory() const;

    void sourceWorkspace();

protected:
    ROSBuildConfiguration(ProjectExplorer::Target *parent, ROSBuildConfiguration *source);
    ROSBuildConfiguration(ProjectExplorer::Target *parent, Core::Id id);

    bool fromMap(const QVariantMap &map);

    friend class ROSBuildSettingsWidget;

private:
    QString m_initialArguments;
    QString m_rosDistribution;
    Utils::FileName m_develDirectory;
    ProjectExplorer::NamedWidget *m_buildEnvironmentWidget;

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

    enum BuildType { BuildTypeNone = 0,
                     BuildTypeDebug = 1,
                     BuildTypeRelease = 2,
                     BuildTypeRelWithDebInfo = 3,
                     BuildTypeMinSizeRel = 4,
                     BuildTypeLast = 5 };

    ROSBuildInfo *createBuildInfo(const ProjectExplorer::Kit *k, const QString &projectPath, BuildType type) const;
};

class ROSBuildSettingsWidget : public ProjectExplorer::NamedWidget
{
    Q_OBJECT

public:
    ROSBuildSettingsWidget(ROSBuildConfiguration *bc);

private slots:
    void on_source_pushButton_clicked();

private:
    Ui::ROSBuildConfiguration *m_ui;
    ROSBuildConfiguration *m_buildConfiguration;
};

class ROSBuildEnvironmentWidget : public ProjectExplorer::NamedWidget
{
  Q_OBJECT

public:
  ROSBuildEnvironmentWidget(ProjectExplorer::BuildConfiguration *bc);

private slots:
  void environmentModelUserChangesChanged();
  void clearSystemEnvironmentCheckBoxClicked(bool checked);
  void environmentChanged();

protected:
  ProjectExplorer::EnvironmentWidget *m_buildEnvironmentWidget;
  QCheckBox *m_clearSystemEnvironmentCheckBox;
  ProjectExplorer::BuildConfiguration *m_buildConfiguration;
};

class ROSBuildInfo : public ProjectExplorer::BuildInfo
{
public:
    ROSBuildInfo(const ProjectExplorer::IBuildConfigurationFactory *f) :
        ProjectExplorer::BuildInfo(f) { }

    ROSBuildInfo(const Internal::ROSBuildConfiguration *bc) :
        ProjectExplorer::BuildInfo(ProjectExplorer::IBuildConfigurationFactory::find(bc->target()))
    {
        displayName = bc->displayName();
        buildDirectory = bc->buildDirectory();
        kitId = bc->target()->kit()->id();
        environment = bc->environment();
        develDirectory = bc->develDirectory();
        arguments = bc->initialArguments();
    }

    Utils::Environment environment;
    Utils::FileName develDirectory;
    QString arguments;
};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSBUILDCONFIGURATION_H
