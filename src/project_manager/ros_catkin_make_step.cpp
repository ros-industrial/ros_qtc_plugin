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
#include "ros_catkin_make_step.h"
#include "ros_project_constants.h"
#include "ros_project.h"
#include "ui_ros_catkin_make_step.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/qtparser.h>
#include <utils/stringutils.h>
#include <utils/qtcassert.h>
#include <cmakeprojectmanager/cmakeoutputparser.h>

#include <QDir>
#include <QComboBox>
#include <QLabel>

using namespace ProjectExplorer;

namespace ROSProjectManager {
namespace Internal {

const char ROS_CMS_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("ROSProjectManager::Internal::ROSCatkinMakeStep",
                                                     "CatkinMake Step");

const char ROS_CMS_TARGET_KEY[] = "ROSProjectManager.ROSCatkinMakeStep.Target";
const char ROS_CMS_CATKIN_MAKE_ARGUMENTS_KEY[] = "ROSProjectManager.ROSCatkinMakeStep.CatkinMakeArguments";
const char ROS_CMS_CMAKE_ARGUMENTS_KEY[] = "ROSProjectManager.ROSCatkinMakeStep.CMakeArguments";
const char ROS_CMS_MAKE_ARGUMENTS_KEY[] = "ROSProjectManager.ROSCatkinMakeStep.MakeArguments";

ROSCatkinMakeStep::ROSCatkinMakeStep(BuildStepList *parent, const Utils::Id id) :
    AbstractProcessStep(parent, id)
{
    setDefaultDisplayName(QCoreApplication::translate("ROSProjectManager::Internal::ROSCatkinMakeStep",
                                                      ROS_CMS_DISPLAY_NAME));

    m_percentProgress = QRegularExpression(QLatin1String("\\[\\s{0,2}(\\d{1,3})%\\]")); // Example: [ 82%] [ 82%] [ 87%]

    ROSBuildConfiguration *bc = rosBuildConfiguration();
    if (bc->rosBuildSystem() != ROSUtils::CatkinMake)
        setEnabled(false);

    connect(this, &BuildStep::addOutput, this, [this](const QString &string, OutputFormat format) {
        if (format == OutputFormat::Stdout)
            stdOutput(string);
    });
}

ROSBuildConfiguration *ROSCatkinMakeStep::rosBuildConfiguration() const
{
    return static_cast<ROSBuildConfiguration *>(buildConfiguration());
}

ROSBuildConfiguration *ROSCatkinMakeStep::targetsActiveBuildConfiguration() const
{
    return static_cast<ROSBuildConfiguration *>(target()->activeBuildConfiguration());
}

ROSCatkinMakeStep::~ROSCatkinMakeStep()
{
}

bool ROSCatkinMakeStep::init()
{
    ROSBuildConfiguration *bc = rosBuildConfiguration();
    if (!bc)
        bc = targetsActiveBuildConfiguration();

    Toolchain *tc = ToolchainKitAspect::toolchain(target()->kit(), ProjectExplorer::Constants::CXX_LANGUAGE_ID);

    if (!tc)
        emit addTask(Task::compilerMissingTask());

    if (!bc || !tc) {
        emitFaultyConfigurationMessage();
        return false;
    }

    // TODO: Need to get build data (build directory, environment, etc.) based on build System
    ROSUtils::WorkspaceInfo workspaceInfo = ROSUtils::getWorkspaceInfo(bc->project()->projectDirectory(), bc->rosBuildSystem(), bc->project()->distribution());

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setWorkingDirectory(bc->project()->projectDirectory());
    Utils::Environment env(ROSUtils::getWorkspaceEnvironment(workspaceInfo, bc->environment()).toStringList());

    bc->updateQtEnvironment(env); // TODO: Not sure if this is required here

    // Force output to english for the parsers. Do this here and not in the toolchain's
    // addToEnvironment() to not screw up the users run environment.
    env.set(QLatin1String("LC_ALL"), QLatin1String("C"));
    pp->setEnvironment(env);
    pp->setCommandLine(makeCommand(allArguments(bc->cmakeBuildType())));

    // If we are cleaning, then make can fail with an error code, but that doesn't mean
    // we should stop the clean queue
    // That is mostly so that rebuild works on an already clean project
    setIgnoreReturnValue(m_target == CLEAN);

    return AbstractProcessStep::init();
}

void ROSCatkinMakeStep::setupOutputFormatter(Utils::OutputFormatter *formatter)
{
    formatter->addLineParser(new GnuMakeParser);
    formatter->addLineParser(new CMakeProjectManager::CMakeOutputParser);

    QList<Utils::OutputLineParser *> parsers = target()->kit()->createOutputParsers();

    if (!parsers.empty())
        formatter->addLineParsers(parsers);

    formatter->addSearchDir(processParameters()->effectiveWorkingDirectory());

    AbstractProcessStep::setupOutputFormatter(formatter);
}

void ROSCatkinMakeStep::toMap(Utils::Store &map) const
{
    AbstractProcessStep::toMap(map);

    map.insert(ROS_CMS_TARGET_KEY, m_target);
    map.insert(ROS_CMS_CATKIN_MAKE_ARGUMENTS_KEY, m_catkinMakeArguments);
    map.insert(ROS_CMS_CMAKE_ARGUMENTS_KEY, m_cmakeArguments);
    map.insert(ROS_CMS_MAKE_ARGUMENTS_KEY, m_makeArguments);
}

void ROSCatkinMakeStep::fromMap(const Utils::Store &map)
{
    m_target = (BuildTargets)map.value(ROS_CMS_TARGET_KEY).toInt();
    m_catkinMakeArguments = map.value(ROS_CMS_CATKIN_MAKE_ARGUMENTS_KEY).toString();
    m_cmakeArguments = map.value(ROS_CMS_CMAKE_ARGUMENTS_KEY).toString();
    m_makeArguments = map.value(ROS_CMS_MAKE_ARGUMENTS_KEY).toString();

    BuildStep::fromMap(map);
}

QWidget *ROSCatkinMakeStep::createConfigWidget()
{
    return new ROSCatkinMakeStepWidget(this);
}

QString ROSCatkinMakeStep::allArguments(ROSUtils::BuildType buildType, bool includeDefault) const
{
    QStringList args;

    switch(m_target) {
    case BUILD:
        args << m_catkinMakeArguments;
        if (includeDefault)
            if (buildType == ROSUtils::BuildTypeUserDefined)
                args << QString("--cmake-args -G \"CodeBlocks - Unix Makefiles\" %1").arg(m_cmakeArguments);
            else
                args << QString("--cmake-args -G \"CodeBlocks - Unix Makefiles\" %1 %2").arg(ROSUtils::getCMakeBuildTypeArgument(buildType), m_cmakeArguments);
        else
            if (!m_cmakeArguments.isEmpty())
                args << QString("--cmake-args %1").arg(m_cmakeArguments);

        break;
    case CLEAN:
        args << QLatin1String("clean");
        args << m_catkinMakeArguments;
        if (!m_cmakeArguments.isEmpty())
            args << QString("--cmake-args %1").arg(m_cmakeArguments);

        break;
    }

    if (!m_makeArguments.isEmpty())
        args << QString("--make-args %1").arg(m_makeArguments);

    return args.join(" ");
}

Utils::CommandLine ROSCatkinMakeStep::makeCommand(const QString &args) const
{
    Utils::CommandLine cmd("catkin_make");
    cmd.addArgs(args, Utils::CommandLine::RawType::Raw);
    return cmd;
}

void ROSCatkinMakeStep::stdOutput(const QString &line)
{
    QRegularExpressionMatchIterator i = m_percentProgress.globalMatch(line);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        bool ok = false;
        const int percent = match.captured(1).toInt(&ok);
        if (ok)
          emit progress(percent, QString());
    }
}

ROSCatkinMakeStep::BuildTargets ROSCatkinMakeStep::buildTarget() const
{
    return m_target;
}

void ROSCatkinMakeStep::setBuildTarget(const BuildTargets &target)
{
    m_target = target;
}

//
// ROSCatkinMakeStepConfigWidget
//

ROSCatkinMakeStepWidget::ROSCatkinMakeStepWidget(ROSCatkinMakeStep *makeStep)
    : m_makeStep(makeStep)
{
    m_ui = new Ui::ROSCatkinMakeStep;
    m_ui->setupUi(this);

    m_ui->catkinMakeArgumentsLineEdit->setText(m_makeStep->m_catkinMakeArguments);
    m_ui->cmakeArgumentsLineEdit->setText(m_makeStep->m_cmakeArguments);
    m_ui->makeArgumentsLineEdit->setText(m_makeStep->m_makeArguments);

    updateDetails();

    connect(m_ui->catkinMakeArgumentsLineEdit, &QLineEdit::textEdited,
            this, &ROSCatkinMakeStepWidget::updateDetails);

    connect(m_ui->cmakeArgumentsLineEdit, &QLineEdit::textEdited,
            this, &ROSCatkinMakeStepWidget::updateDetails);

    connect(m_ui->makeArgumentsLineEdit, &QLineEdit::textEdited,
            this, &ROSCatkinMakeStepWidget::updateDetails);

    connect(m_makeStep, SIGNAL(enabledChanged()),
            this, SLOT(enabledChanged()));

    ROSBuildConfiguration *bc = m_makeStep->rosBuildConfiguration();
    connect(bc, SIGNAL(buildSystemChanged(ROSUtils::BuildSystem)),
            this, SLOT(updateBuildSystem(ROSUtils::BuildSystem)));

    connect(bc, &ROSBuildConfiguration::cmakeBuildTypeChanged,
            this, &ROSCatkinMakeStepWidget::updateDetails);

    connect(bc, &ROSBuildConfiguration::environmentChanged,
            this, &ROSCatkinMakeStepWidget::updateDetails);

    connect(ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateDetails()));
}

ROSCatkinMakeStepWidget::~ROSCatkinMakeStepWidget()
{
    delete m_ui;
}

QString ROSCatkinMakeStepWidget::displayName() const
{
    return tr("CatkinMake", "CatkinMake display name.");
}

void ROSCatkinMakeStepWidget::updateDetails()
{
    m_makeStep->m_catkinMakeArguments = m_ui->catkinMakeArgumentsLineEdit->text();
    m_makeStep->m_cmakeArguments = m_ui->cmakeArgumentsLineEdit->text();
    m_makeStep->m_makeArguments = m_ui->makeArgumentsLineEdit->text();

    ROSBuildConfiguration *bc = m_makeStep->rosBuildConfiguration();
    ROSUtils::WorkspaceInfo workspaceInfo = ROSUtils::getWorkspaceInfo(bc->project()->projectDirectory(), bc->rosBuildSystem(), bc->project()->distribution());

    ProcessParameters param;
    param.setMacroExpander(bc->macroExpander());
    param.setWorkingDirectory(workspaceInfo.buildPath);
    param.setEnvironment(bc->environment());
    param.setCommandLine(m_makeStep->makeCommand(m_makeStep->allArguments(bc->cmakeBuildType(), false)));
    m_summaryText = param.summary(displayName());
}

void ROSCatkinMakeStepWidget::updateBuildSystem(const ROSUtils::BuildSystem &buildSystem)
{
    m_makeStep->setEnabled((buildSystem == ROSUtils::CatkinMake));
}

void ROSCatkinMakeStepWidget::enabledChanged()
{
    ROSBuildConfiguration *bc = m_makeStep->rosBuildConfiguration();
    if(m_makeStep->stepEnabled() && (bc->rosBuildSystem() != ROSUtils::CatkinMake))
        m_makeStep->setEnabled(false);
}

QString ROSCatkinMakeStepWidget::summaryText() const
{
    return m_summaryText;
}

//
// ROSCatkinMakeStepFactory
//

ROSCatkinMakeStepFactory::ROSCatkinMakeStepFactory() : BuildStepFactory()
{
  registerStep<ROSCatkinMakeStep>(ROS_CMS_ID);
  setFlags(BuildStep::Flags::UniqueStep);
  setDisplayName(QCoreApplication::translate("ROSProjectManager::Internal::ROSCatkinMakeStep", ROS_CMS_DISPLAY_NAME));
  setSupportedProjectType(Constants::ROS_PROJECT_ID);
  setSupportedStepLists(QList<Utils::Id>({ProjectExplorer::Constants::BUILDSTEPS_BUILD, ProjectExplorer::Constants::BUILDSTEPS_CLEAN}));
}

} // namespace Internal
} // namespace ROSProjectManager
