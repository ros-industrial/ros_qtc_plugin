/**
 * @author Joseph Schornak
 * @date May 30, 2019
 *
 * @copyright Copyright (c) 2019, Southwest Research Institute
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
#include "ros_colcon_step.h"
#include "ros_project_constants.h"
#include "ros_project.h"
#include "ui_ros_colcon_step.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtparser.h>
#include <utils/stringutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <cmakeprojectmanager/cmakeparser.h>

#include <QDir>
#include <QComboBox>
#include <QLabel>

using namespace ProjectExplorer;

namespace ROSProjectManager {
namespace Internal {

const char ROS_COLCON_STEP_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("ROSProjectManager::Internal::ROSColconStep",
                                                     "Colcon Step");

const char ROS_COLCON_STEP[] = "ROSProjectManager.ROSColconStep.Target";
const char ROS_COLCON_STEP_ARGUMENTS_KEY[] = "ROSProjectManager.ROSColconStep.ColconArguments";
const char ROS_COLCON_STEP_CMAKE_ARGUMENTS_KEY[] = "ROSProjectManager.ROSColconStep.CMakeArguments";
const char ROS_COLCON_STEP_MAKE_ARGUMENTS_KEY[] = "ROSProjectManager.ROSColconStep.MakeArguments";

ROSColconStep::ROSColconStep(BuildStepList *parent, const Utils::Id id) :
    AbstractProcessStep(parent, id)
{
    setDefaultDisplayName(QCoreApplication::translate("ROSProjectManager::Internal::ROSColconStep",
                                                      ROS_COLCON_STEP_DISPLAY_NAME));

    m_percentProgress = QRegExp(QLatin1String(".+\\[(\\d+)/(\\d+) complete\\]")); // Example: [0/24 complete]

    ROSBuildConfiguration *bc = rosBuildConfiguration();
    if (bc->rosBuildSystem() != ROSUtils::Colcon)
        setEnabled(false);
}

ROSBuildConfiguration *ROSColconStep::rosBuildConfiguration() const
{
    return static_cast<ROSBuildConfiguration *>(buildConfiguration());
}

ROSBuildConfiguration *ROSColconStep::targetsActiveBuildConfiguration() const
{
    return static_cast<ROSBuildConfiguration *>(target()->activeBuildConfiguration());
}

ROSColconStep::~ROSColconStep()
{
}

bool ROSColconStep::init()
{
    ROSBuildConfiguration *bc = rosBuildConfiguration();
    if (!bc)
        bc = targetsActiveBuildConfiguration();

    ToolChain *tc = ToolChainKitAspect::toolChain(target()->kit(), ProjectExplorer::Constants::CXX_LANGUAGE_ID);

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

void ROSColconStep::setupOutputFormatter(Utils::OutputFormatter *formatter)
{
    formatter->addLineParser(new GnuMakeParser);
    formatter->addLineParser(new CMakeProjectManager::CMakeParser);

    QList<Utils::OutputLineParser *> parsers = target()->kit()->createOutputParsers();

    if (!parsers.empty())
        formatter->addLineParsers(parsers);

    formatter->addSearchDir(processParameters()->effectiveWorkingDirectory());

    AbstractProcessStep::setupOutputFormatter(formatter);
}

QVariantMap ROSColconStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());

    map.insert(QLatin1String(ROS_COLCON_STEP), m_target);
    map.insert(QLatin1String(ROS_COLCON_STEP_ARGUMENTS_KEY), m_colconArguments);
    map.insert(QLatin1String(ROS_COLCON_STEP_CMAKE_ARGUMENTS_KEY), m_cmakeArguments);
    map.insert(QLatin1String(ROS_COLCON_STEP_MAKE_ARGUMENTS_KEY), m_makeArguments);
    return map;
}

bool ROSColconStep::fromMap(const QVariantMap &map)
{
    m_target = (BuildTargets)map.value(QLatin1String(ROS_COLCON_STEP)).toInt();
    m_colconArguments = map.value(QLatin1String(ROS_COLCON_STEP_ARGUMENTS_KEY)).toString();
    m_cmakeArguments = map.value(QLatin1String(ROS_COLCON_STEP_CMAKE_ARGUMENTS_KEY)).toString();
    m_makeArguments = map.value(QLatin1String(ROS_COLCON_STEP_MAKE_ARGUMENTS_KEY)).toString();

    return BuildStep::fromMap(map);
}

QString ROSColconStep::allArguments(ROSUtils::BuildType buildType, bool includeDefault) const
{
    QString args;

    switch(m_target) {
    case BUILD:
        Utils::QtcProcess::addArgs(&args, QLatin1String("build"));
        Utils::QtcProcess::addArgs(&args, m_colconArguments);
        if (includeDefault)
            if (buildType == ROSUtils::BuildTypeUserDefined)
                Utils::QtcProcess::addArgs(&args, QString("--cmake-args -G \"CodeBlocks - Unix Makefiles\" %1").arg(m_cmakeArguments));
            else
                Utils::QtcProcess::addArgs(&args, QString("--cmake-args -G \"CodeBlocks - Unix Makefiles\" %1 %2").arg(ROSUtils::getCMakeBuildTypeArgument(buildType), m_cmakeArguments));
        else
            if (!m_cmakeArguments.isEmpty())
                Utils::QtcProcess::addArgs(&args, QString("--cmake-args %1").arg(m_cmakeArguments));

        break;
    case CLEAN:
        Utils::QtcProcess::addArgs(&args, QLatin1String("-r build/ install/ log/"));

        break;
    }

    if (!m_makeArguments.isEmpty())
        Utils::QtcProcess::addArgs(&args, QString("--make-args %1").arg(m_makeArguments));

    return args;
}

Utils::CommandLine ROSColconStep::makeCommand(const QString &args) const
{
    QLatin1String exec;
    switch(m_target) {
    case BUILD:
        exec = QLatin1String("colcon");
        break;
    case CLEAN:
        exec = QLatin1String("rm");
        break;
    default:
        exec = QLatin1String("colcon");
        break;
    }

    Utils::CommandLine cmd(exec);
    cmd.addArgs(args, Utils::CommandLine::RawType::Raw);
    return cmd;
}

void ROSColconStep::stdOutput(const QString &line)
{
    AbstractProcessStep::stdOutput(line);
    if (m_percentProgress.indexIn(line, 0) != -1)
    {
        bool ok = false;
        int percent = (m_percentProgress.cap(1).toDouble(&ok)/m_percentProgress.cap(2).toDouble(&ok)) * 100.0;
        if (ok)
          emit progress(percent, QString());
    }
}

ROSColconStep::BuildTargets ROSColconStep::buildTarget() const
{
    return m_target;
}

void ROSColconStep::setBuildTarget(const BuildTargets &target)
{
    m_target = target;
}

//
// ROSColconStepConfigWidget
//

ROSColconStepWidget::ROSColconStepWidget(ROSColconStep *makeStep)
    : m_makeStep(makeStep)
{
    m_ui = new Ui::ROSColconStep;
    m_ui->setupUi(this);

    m_ui->colconArgumentsLineEdit->setText(m_makeStep->m_colconArguments);
    m_ui->cmakeArgumentsLineEdit->setText(m_makeStep->m_cmakeArguments);
    m_ui->makeArgumentsLineEdit->setText(m_makeStep->m_makeArguments);

    updateDetails();

    connect(m_ui->colconArgumentsLineEdit, &QLineEdit::textEdited,
            this, &ROSColconStepWidget::updateDetails);

    connect(m_ui->cmakeArgumentsLineEdit, &QLineEdit::textEdited,
            this, &ROSColconStepWidget::updateDetails);

    connect(m_ui->makeArgumentsLineEdit, &QLineEdit::textEdited,
            this, &ROSColconStepWidget::updateDetails);

    connect(m_makeStep, SIGNAL(enabledChanged()),
            this, SLOT(enabledChanged()));

    ROSBuildConfiguration *bc = m_makeStep->rosBuildConfiguration();
    connect(bc, SIGNAL(buildSystemChanged(ROSUtils::BuildSystem)),
            this, SLOT(updateBuildSystem(ROSUtils::BuildSystem)));

    connect(bc, &ROSBuildConfiguration::cmakeBuildTypeChanged,
            this, &ROSColconStepWidget::updateDetails);

    connect(bc, &ROSBuildConfiguration::environmentChanged,
            this, &ROSColconStepWidget::updateDetails);

    connect(ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateDetails()));
}

ROSColconStepWidget::~ROSColconStepWidget()
{
    delete m_ui;
}

QString ROSColconStepWidget::displayName() const
{
    return tr("Colcon", "Colcon display name.");
}

void ROSColconStepWidget::updateDetails()
{
    m_makeStep->m_colconArguments = m_ui->colconArgumentsLineEdit->text();
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

void ROSColconStepWidget::updateBuildSystem(const ROSUtils::BuildSystem &buildSystem)
{
    m_makeStep->setEnabled((buildSystem == ROSUtils::Colcon));
}

void ROSColconStepWidget::enabledChanged()
{
    ROSBuildConfiguration *bc = m_makeStep->rosBuildConfiguration();
    if(m_makeStep->enabled() && (bc->rosBuildSystem() != ROSUtils::Colcon))
        m_makeStep->setEnabled(false);
}

QString ROSColconStepWidget::summaryText() const
{
    return m_summaryText;
}

//
// ROSColconStepFactory
//

ROSColconStepFactory::ROSColconStepFactory() : BuildStepFactory()
{
  registerStep<ROSColconStep>(ROS_COLCON_STEP_ID);
  setFlags(BuildStepInfo::Flags::UniqueStep);
  setDisplayName(QCoreApplication::translate("ROSProjectManager::Internal::ROSColconStep", ROS_COLCON_STEP_DISPLAY_NAME));
  setSupportedProjectType(Constants::ROS_PROJECT_ID);
  setSupportedStepLists(QList<Utils::Id>({ProjectExplorer::Constants::BUILDSTEPS_BUILD, ProjectExplorer::Constants::BUILDSTEPS_CLEAN}));
}

} // namespace Internal
} // namespace ROSProjectManager
