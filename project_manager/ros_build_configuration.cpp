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

#include "ros_build_configuration.h"

#include "ros_make_step.h"
#include "ros_project.h"
#include "ros_project_constants.h"
#include "ui_ros_build_configuration.h"

#include <coreplugin/icore.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QFormLayout>
#include <QInputDialog>

using namespace ProjectExplorer;

namespace ROSProjectManager {
namespace Internal {

const char ROS_BC_ID[] = "ROSProjectManager.ROSBuildConfiguration";
const char ROS_BC_INITIAL_ARGUMENTS[] = "ROSProjectManager.ROSBuildConfiguration.InitialArguments";
const char ROS_BC_DISTRIBUTION[] = "ROSProjectManager.ROSBuildConfiguration.Distribution";
const char ROS_BC_DEVEL_DIRECTORY[] = "ROSProjectManager.ROSBuildConfiguration.DevelDirectory";

ROSBuildConfiguration::ROSBuildConfiguration(Target *parent)
    : BuildConfiguration(parent, Core::Id(ROS_BC_ID))
{
}

ROSBuildConfiguration::ROSBuildConfiguration(Target *parent, Core::Id id)
    : BuildConfiguration(parent, id)
{
}

ROSBuildConfiguration::ROSBuildConfiguration(Target *parent, ROSBuildConfiguration *source) :
    BuildConfiguration(parent, source),m_initialArguments(source->m_initialArguments),
    m_rosDistribution(source->m_rosDistribution), m_develDirectory(source->m_develDirectory)
{
    cloneSteps(source);
}

QVariantMap ROSBuildConfiguration::toMap() const
{
  QVariantMap map(BuildConfiguration::toMap());

  map.insert(QLatin1String(ROS_BC_INITIAL_ARGUMENTS), m_initialArguments);
  map.insert(QLatin1String(ROS_BC_DISTRIBUTION), m_rosDistribution);
  map.insert(QLatin1String(ROS_BC_DEVEL_DIRECTORY), m_develDirectory.toString());

  return map;
}

bool ROSBuildConfiguration::fromMap(const QVariantMap &map)
{
  m_initialArguments = map.value(QLatin1String(ROS_BC_INITIAL_ARGUMENTS)).toString();
  m_rosDistribution = map.value(QLatin1String(ROS_BC_DISTRIBUTION)).toString();
  m_develDirectory = Utils::FileName::fromString(map.value(QLatin1String(ROS_BC_DEVEL_DIRECTORY)).toString());
  return BuildConfiguration::fromMap(map);
}

void ROSBuildConfiguration::setInitialArguments(const QString &arguments)
{
  m_initialArguments = arguments;
}

QString ROSBuildConfiguration::initialArguments() const
{
  return m_initialArguments;
}

void ROSBuildConfiguration::setDevelDirectory(const Utils::FileName &dir)
{
  m_develDirectory = dir;
}

Utils::FileName ROSBuildConfiguration::develDirectory() const
{
  return m_develDirectory;
}

void ROSBuildConfiguration::sourceWorkspace()
{
  // Need to source devel directory to setup enviroment variables
  QString cmd = QLatin1String(" source ") + develDirectory().appendPath(tr("setup.bash)")).toString();
  QString ws_dir = target()->project()->projectDirectory().toString();
  QProcess source_devel;
  source_devel.setWorkingDirectory(ws_dir);
  source_devel.start(QLatin1String("bash"), QStringList() << QLatin1String("-c") << cmd);
  source_devel.waitForBytesWritten();
  source_devel.waitForFinished();

  if (source_devel.exitStatus() == QProcess::CrashExit)
  {
    qDebug() << source_devel.errorString();
  }
  else
  {
    QList<Utils::EnvironmentItem> current_env = userEnvironmentChanges();
    Utils::Environment source_env = Utils::Environment(source_devel.systemEnvironment());
    source_env.set(QLatin1String("PWD"), ws_dir);

    // Need to check if additional user changes are not overwritten
    QList<Utils::EnvironmentItem> diff = baseEnvironment().diff(source_env);

    if (!diff.isEmpty())
    {
      foreach(Utils::EnvironmentItem it, current_env)
      {
        source_env.appendOrSet(it.name, it.value);
      }

      diff = baseEnvironment().diff(source_env);
      setUserEnvironmentChanges(diff);
    }
  }

}

NamedWidget *ROSBuildConfiguration::createConfigWidget()
{
    return new ROSBuildSettingsWidget(this);
}

QList<NamedWidget *> ROSBuildConfiguration::createSubConfigWidgets()
{
  return QList<NamedWidget *>() << new ROSBuildEnvironmentWidget(this);
}

/*!
  \class ROSBuildConfigurationFactory
*/

ROSBuildConfigurationFactory::ROSBuildConfigurationFactory(QObject *parent) :
    IBuildConfigurationFactory(parent)
{
}

ROSBuildConfigurationFactory::~ROSBuildConfigurationFactory()
{
}

int ROSBuildConfigurationFactory::priority(const Target *parent) const
{
    return canHandle(parent) ? 0 : -1;
}

QList<BuildInfo *> ROSBuildConfigurationFactory::availableBuilds(const Target *parent) const
{
    QList<BuildInfo *> result;
    QString project_path = parent->project()->projectDirectory().toString();

    for (int type = BuildTypeNone; type != BuildTypeLast; ++type)
    {
      ROSBuildInfo *info = createBuildInfo(parent->kit(), project_path, BuildType(type));
      result << info;
    }

    return result;
}

int ROSBuildConfigurationFactory::priority(const Kit *k, const QString &projectPath) const
{
    Utils::MimeDatabase mdb;
    if (k && mdb.mimeTypeForFile(projectPath).matchesName(QLatin1String(Constants::ROSMIMETYPE)))
        return 0;
    return -1;
}

QList<BuildInfo *> ROSBuildConfigurationFactory::availableSetups(const Kit *k, const QString &projectPath) const
{
    QList<BuildInfo *> result;

    for (int type = BuildTypeNone; type != BuildTypeLast; ++type) {
      ROSBuildInfo *info = createBuildInfo(k, projectPath, BuildType(type));
      result << info;
    }

    //TO DO: Should probably check if the directory that was selected was the workspace
    return result;
}

BuildConfiguration *ROSBuildConfigurationFactory::create(Target *parent, const BuildInfo *info) const
{
    QTC_ASSERT(info->factory() == this, return 0);
    QTC_ASSERT(info->kitId == parent->kit()->id(), return 0);
    QTC_ASSERT(!info->displayName.isEmpty(), return 0);

    ROSBuildInfo ros_info(*static_cast<const ROSBuildInfo *>(info));
    ROSBuildConfiguration *bc = new ROSBuildConfiguration(parent);

    bc->setDisplayName(ros_info.displayName);
    bc->setDefaultDisplayName(ros_info.displayName);
    bc->setBuildDirectory(ros_info.buildDirectory);
    bc->setDevelDirectory(ros_info.develDirectory);
    bc->setInitialArguments(ros_info.arguments);

    BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
    Q_ASSERT(buildSteps);
    Q_ASSERT(cleanSteps);

    ROSMakeStep *makeStep = new ROSMakeStep(buildSteps);
    buildSteps->insertStep(0, makeStep);

    makeStep->setBuildTarget(QLatin1String("all"), /* on = */ true);


    ROSMakeStep *cleanMakeStep = new ROSMakeStep(cleanSteps);
    cleanSteps->insertStep(0, cleanMakeStep);
    cleanMakeStep->setBuildTarget(QLatin1String("clean"), /* on = */ true);
    cleanMakeStep->setClean(true);

    return bc;
}

bool ROSBuildConfigurationFactory::canClone(const Target *parent, BuildConfiguration *source) const
{
    if (!canHandle(parent))
        return false;
    return source->id() == ROS_BC_ID;
}

BuildConfiguration *ROSBuildConfigurationFactory::clone(Target *parent, BuildConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    return new ROSBuildConfiguration(parent, qobject_cast<ROSBuildConfiguration *>(source));
}

bool ROSBuildConfigurationFactory::canRestore(const Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    return ProjectExplorer::idFromMap(map) == ROS_BC_ID;
}

BuildConfiguration *ROSBuildConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    ROSBuildConfiguration *bc(new ROSBuildConfiguration(parent));
    if (bc->fromMap(map))
        return bc;
    delete bc;
    return 0;
}

bool ROSBuildConfigurationFactory::canHandle(const Target *t) const
{
    if (!t->project()->supportsKit(t->kit()))
        return false;
    return qobject_cast<ROSProject *>(t->project());
}

ROSBuildInfo *ROSBuildConfigurationFactory::createBuildInfo(const Kit *k,
                                                          const QString &projectPath,
                                                          BuildType type) const
{
    ROSBuildInfo *info = new ROSBuildInfo(this);
    info->kitId = k->id();
    info->buildDirectory = Project::projectDirectory(Utils::FileName::fromString(projectPath)).appendPath(tr("build"));
    info->develDirectory = Project::projectDirectory(Utils::FileName::fromString(projectPath)).appendPath(tr("devel"));
    switch (type) {
    case BuildTypeNone:
        info->displayName = tr("Default");
        info->typeName = tr("Build");
        break;
    case BuildTypeDebug:
        info->arguments = QLatin1String("-DCMAKE_BUILD_TYPE=Debug");
        info->typeName = tr("Debug");
        info->displayName = info->typeName;
        info->buildType = BuildConfiguration::Debug;
        break;
    case BuildTypeRelease:
        info->arguments = QLatin1String("-DCMAKE_BUILD_TYPE=Release");
        info->typeName = tr("Release");
        info->displayName = info->typeName;
        info->buildType = BuildConfiguration::Release;
        break;
    case BuildTypeMinSizeRel:
        info->arguments = QLatin1String("-DCMAKE_BUILD_TYPE=MinSizeRel");
        info->typeName = tr("Minimum Size Release");
        info->displayName = info->typeName;
        info->buildType = BuildConfiguration::Release;
        break;
    case BuildTypeRelWithDebInfo:
        info->arguments = QLatin1String("-DCMAKE_BUILD_TYPE=RelWithDebInfo");
        info->typeName = tr("Release with Debug Information");
        info->displayName = info->typeName;
        info->buildType = BuildConfiguration::Profile;
        break;
    default:
        QTC_CHECK(false);
        break;
    }

    return info;
}

BuildConfiguration::BuildType ROSBuildConfiguration::buildType() const
{
  if (m_initialArguments == QLatin1String("-DCMAKE_BUILD_TYPE=Debug"))
  {
    return Debug;
  }
  else if ((m_initialArguments == QLatin1String("-DCMAKE_BUILD_TYPE=Debug")) || (m_initialArguments == QLatin1String("-DCMAKE_BUILD_TYPE=MinSizeRel")))
  {
    return Release;
  }
  else if (m_initialArguments == QLatin1String("-DCMAKE_BUILD_TYPE=RelWithDebInfo"))
  {
    return Profile;
  }
  else
  {
    return Unknown;
  }
}

////////////////////////////////////////////////////////////////////////////////////
// ROSBuildSettingsWidget
////////////////////////////////////////////////////////////////////////////////////

ROSBuildSettingsWidget::ROSBuildSettingsWidget(ROSBuildConfiguration *bc)
    : m_buildConfiguration(bc)
{
    m_ui = new Ui::ROSBuildConfiguration;
    m_ui->setupUi(this);

    connect(m_ui->source_pushButton, SIGNAL(clicked()),
                this, SLOT(on_source_pushButton_clicked()));

    setDisplayName(tr("ROS Manager"));
}

void ROSBuildSettingsWidget::on_source_pushButton_clicked()
{
    m_buildConfiguration->sourceWorkspace();
}

////////////////////////////////////////////////////////////////////////////////////
// ROSBuildEnvironmentWidget
////////////////////////////////////////////////////////////////////////////////////

ROSBuildEnvironmentWidget::ROSBuildEnvironmentWidget(BuildConfiguration *bc)
    : m_buildConfiguration(0)
{
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(0);
    m_clearSystemEnvironmentCheckBox = new QCheckBox(this);
    m_clearSystemEnvironmentCheckBox->setText(tr("Clear system environment"));

    m_buildEnvironmentWidget = new EnvironmentWidget(this, m_clearSystemEnvironmentCheckBox);
    vbox->addWidget(m_buildEnvironmentWidget);

    connect(m_buildEnvironmentWidget, SIGNAL(userChangesChanged()),
            this, SLOT(environmentModelUserChangesChanged()));
    connect(m_clearSystemEnvironmentCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(clearSystemEnvironmentCheckBoxClicked(bool)));

    m_buildConfiguration = bc;

    connect(m_buildConfiguration->target(), SIGNAL(environmentChanged()),
            this, SLOT(environmentChanged()));

    m_clearSystemEnvironmentCheckBox->setChecked(!m_buildConfiguration->useSystemEnvironment());
    m_buildEnvironmentWidget->setBaseEnvironment(m_buildConfiguration->baseEnvironment());
    m_buildEnvironmentWidget->setBaseEnvironmentText(m_buildConfiguration->baseEnvironmentText());
    m_buildEnvironmentWidget->setUserChanges(m_buildConfiguration->userEnvironmentChanges());

    setDisplayName(tr("Build Environment"));
}

void ROSBuildEnvironmentWidget::environmentModelUserChangesChanged()
{
    m_buildConfiguration->setUserEnvironmentChanges(m_buildEnvironmentWidget->userChanges());
}

void ROSBuildEnvironmentWidget::clearSystemEnvironmentCheckBoxClicked(bool checked)
{
    m_buildConfiguration->setUseSystemEnvironment(!checked);
    m_buildEnvironmentWidget->setBaseEnvironment(m_buildConfiguration->baseEnvironment());
    m_buildEnvironmentWidget->setBaseEnvironmentText(m_buildConfiguration->baseEnvironmentText());
}

void ROSBuildEnvironmentWidget::environmentChanged()
{
    m_buildEnvironmentWidget->setBaseEnvironment(m_buildConfiguration->baseEnvironment());
    m_buildEnvironmentWidget->setBaseEnvironmentText(m_buildConfiguration->baseEnvironmentText());
    m_buildEnvironmentWidget->setUserChanges(m_buildConfiguration->userEnvironmentChanges());
}

} // namespace Internal
} // namespace GenericProjectManager

