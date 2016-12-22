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
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtparser.h>
#include <utils/stringutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QDir>
#include <QComboBox>
#include <QLabel>

using namespace Core;
using namespace ProjectExplorer;

namespace ROSProjectManager {
namespace Internal {

const char ROS_CMS_ID[] = "ROSProjectManager.ROSCatkinMakeStep";
const char ROS_CMS_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("ROSProjectManager::Internal::ROSCatkinMakeStep",
                                                     "CatkinMake Step");

const char ROS_CMS_TARGET_KEY[] = "ROSProjectManager.ROSCatkinMakeStep.Target";
const char ROS_CMS_CATKIN_MAKE_ARGUMENTS_KEY[] = "ROSProjectManager.ROSCatkinMakeStep.CatkinMakeArguments";
const char ROS_CMS_CMAKE_ARGUMENTS_KEY[] = "ROSProjectManager.ROSCatkinMakeStep.CMakeArguments";
const char ROS_CMS_MAKE_ARGUMENTS_KEY[] = "ROSProjectManager.ROSCatkinMakeStep.MakeArguments";

ROSCatkinMakeStep::ROSCatkinMakeStep(BuildStepList *parent) :
    AbstractProcessStep(parent, Id(ROS_CMS_ID))
{
    ctor();
}

ROSCatkinMakeStep::ROSCatkinMakeStep(BuildStepList *parent, const Id id) :
    AbstractProcessStep(parent, id)
{
    ctor();
}

ROSCatkinMakeStep::ROSCatkinMakeStep(BuildStepList *parent, ROSCatkinMakeStep *bs) :
    AbstractProcessStep(parent, bs),
    m_target(bs->m_target),
    m_catkinMakeArguments(bs->m_catkinMakeArguments),
    m_cmakeArguments(bs->m_cmakeArguments),
    m_makeArguments(bs->m_makeArguments)
{
    ctor();
}

void ROSCatkinMakeStep::ctor()
{
    setDefaultDisplayName(QCoreApplication::translate("ROSProjectManager::Internal::ROSCatkinMakeStep",
                                                      ROS_CMS_DISPLAY_NAME));

    m_percentProgress = QRegExp(QLatin1String("\\[\\s{0,2}(\\d{1,3})%\\]")); // Example: [ 82%] [ 82%] [ 87%]

    ROSBuildConfiguration *bc = rosBuildConfiguration();
    if (bc->buildSystem() != ROSUtils::CatkinMake)
        setEnabled(false);
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

bool ROSCatkinMakeStep::init(QList<const BuildStep *> &earlierSteps)
{
    ROSBuildConfiguration *bc = rosBuildConfiguration();
    if (!bc)
        bc = targetsActiveBuildConfiguration();
    if (!bc)
        emit addTask(Task::buildConfigurationMissingTask());

    ToolChain *tc = ToolChainKitInformation::toolChain(target()->kit());
    if (!tc)
        emit addTask(Task::compilerMissingTask());

    if (!bc || !tc) {
        emitFaultyConfigurationMessage();
        return false;
    }

    // TODO: Need to get build data (build directory, environment, etc.) based on build System
    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setWorkingDirectory(bc->project()->projectDirectory().toString());
    Utils::Environment env(ROSUtils::getWorkspaceEnvironment(bc->project()->projectDirectory(), bc->project()->distribution(), bc->buildSystem()).toStringList());
    // Force output to english for the parsers. Do this here and not in the toolchain's
    // addToEnvironment() to not screw up the users run environment.
    env.set(QLatin1String("LC_ALL"), QLatin1String("C"));
    pp->setEnvironment(env);
    pp->setCommand(makeCommand());
    pp->setArguments(allArguments(bc->cmakeBuildType()));
    pp->resolveAll();

    // If we are cleaning, then make can fail with an error code, but that doesn't mean
    // we should stop the clean queue
    // That is mostly so that rebuild works on an already clean project
    setIgnoreReturnValue(m_target == CLEAN);

    setOutputParser(new GnuMakeParser());
    IOutputParser *parser = target()->kit()->createOutputParser();
    if (parser)
        appendOutputParser(parser);
    outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());

    return AbstractProcessStep::init(earlierSteps);
}

QVariantMap ROSCatkinMakeStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());

    map.insert(QLatin1String(ROS_CMS_TARGET_KEY), m_target);
    map.insert(QLatin1String(ROS_CMS_CATKIN_MAKE_ARGUMENTS_KEY), m_catkinMakeArguments);
    map.insert(QLatin1String(ROS_CMS_CMAKE_ARGUMENTS_KEY), m_cmakeArguments);
    map.insert(QLatin1String(ROS_CMS_MAKE_ARGUMENTS_KEY), m_makeArguments);
    return map;
}

bool ROSCatkinMakeStep::fromMap(const QVariantMap &map)
{
    m_target = (BuildTargets)map.value(QLatin1String(ROS_CMS_TARGET_KEY)).toInt();
    m_catkinMakeArguments = map.value(QLatin1String(ROS_CMS_CATKIN_MAKE_ARGUMENTS_KEY)).toString();
    m_cmakeArguments = map.value(QLatin1String(ROS_CMS_CMAKE_ARGUMENTS_KEY)).toString();
    m_makeArguments = map.value(QLatin1String(ROS_CMS_MAKE_ARGUMENTS_KEY)).toString();

    return BuildStep::fromMap(map);
}

QString ROSCatkinMakeStep::allArguments(ROSUtils::BuildType buildType, bool includeDefault) const
{
    QString args;

    switch(m_target) {
    case BUILD:
        Utils::QtcProcess::addArgs(&args, m_catkinMakeArguments);
        if (includeDefault)
            Utils::QtcProcess::addArgs(&args, QString("--cmake-args -G \"CodeBlocks - Unix Makefiles\" %1 %2").arg(ROSUtils::getCMakeBuildTypeArgument(buildType), m_cmakeArguments));
        else
            if (!m_cmakeArguments.isEmpty())
                Utils::QtcProcess::addArgs(&args, QString("--cmake-args %1").arg(m_cmakeArguments));

        break;
    case CLEAN:
        Utils::QtcProcess::addArgs(&args, QLatin1String("clean"));
        Utils::QtcProcess::addArgs(&args, m_catkinMakeArguments);
        if (!m_cmakeArguments.isEmpty())
            Utils::QtcProcess::addArgs(&args, QString("--cmake-args %1").arg(m_cmakeArguments));

        break;
    }

    if (!m_makeArguments.isEmpty())
        Utils::QtcProcess::addArgs(&args, QString("--make-args %1").arg(m_makeArguments));

    return args;
}

QString ROSCatkinMakeStep::makeCommand() const
{
    return QLatin1String("catkin_make");;
}

void ROSCatkinMakeStep::run(QFutureInterface<bool> &fi)
{
    AbstractProcessStep::run(fi);
}

void ROSCatkinMakeStep::processStarted()
{
    futureInterface()->setProgressRange(0, 100);
    AbstractProcessStep::processStarted();
}

void ROSCatkinMakeStep::processFinished(int exitCode, QProcess::ExitStatus status)
{
    AbstractProcessStep::processFinished(exitCode, status);
    futureInterface()->setProgressValue(100);
}

void ROSCatkinMakeStep::stdOutput(const QString &line)
{
    AbstractProcessStep::stdOutput(line);
    int pos = 0;
    while ((pos = m_percentProgress.indexIn(line, pos)) != -1) {
        bool ok = false;
        int percent = m_percentProgress.cap(1).toInt(&ok);
        if (ok)
            futureInterface()->setProgressValue(percent);

        pos += m_percentProgress.matchedLength();
    }
}

BuildStepConfigWidget *ROSCatkinMakeStep::createConfigWidget()
{
    return new ROSCatkinMakeStepWidget(this);
}

bool ROSCatkinMakeStep::immutable() const
{
    return false;
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

    ROSProject *pro = static_cast<ROSProject *>(m_makeStep->target()->project());
    connect(pro, &ROSProject::environmentChanged,
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
    Utils::Environment env(ROSUtils::getWorkspaceEnvironment(bc->project()->projectDirectory(), bc->project()->distribution(), bc->buildSystem()).toStringList());

    ProcessParameters param;
    param.setMacroExpander(bc->macroExpander());
    param.setWorkingDirectory(bc->buildDirectory().toString());
    param.setEnvironment(env);
    param.setCommand(m_makeStep->makeCommand());
    param.setArguments(m_makeStep->allArguments(bc->cmakeBuildType(), false));
    m_summaryText = param.summary(displayName());
    emit updateSummary();
}

void ROSCatkinMakeStepWidget::updateBuildSystem(const ROSUtils::BuildSystem &buildSystem)
{
    m_makeStep->setEnabled((buildSystem == ROSUtils::CatkinMake));
}

void ROSCatkinMakeStepWidget::enabledChanged()
{
    ROSBuildConfiguration *bc = m_makeStep->rosBuildConfiguration();
    if(m_makeStep->enabled() && (bc->buildSystem() != ROSUtils::CatkinMake))
        m_makeStep->setEnabled(false);
}

QString ROSCatkinMakeStepWidget::summaryText() const
{
    return m_summaryText;
}

//
// ROSCatkinMakeStepFactory
//

ROSCatkinMakeStepFactory::ROSCatkinMakeStepFactory(QObject *parent) :
    IBuildStepFactory(parent)
{
}

BuildStep *ROSCatkinMakeStepFactory::create(BuildStepList *parent, const Id id)
{
    Q_UNUSED(id);
    ROSCatkinMakeStep *step = new ROSCatkinMakeStep(parent);
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN) {
        step->setBuildTarget(ROSCatkinMakeStep::CLEAN);
    } else if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_BUILD) {
        step->setBuildTarget(ROSCatkinMakeStep::BUILD);
    }
    return step;
}

BuildStep *ROSCatkinMakeStepFactory::clone(BuildStepList *parent, BuildStep *source)
{
    return new ROSCatkinMakeStep(parent, qobject_cast<ROSCatkinMakeStep *>(source));
}

BuildStep *ROSCatkinMakeStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    ROSCatkinMakeStep *bs(new ROSCatkinMakeStep(parent));
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QList<ProjectExplorer::BuildStepInfo> ROSCatkinMakeStepFactory::availableSteps(BuildStepList *parent) const
{
    if (parent->target()->project()->id() != Constants::ROS_PROJECT_ID)
        return {};

    return {{ROS_CMS_ID,  QCoreApplication::translate("ROSProjectManager::Internal::ROSCatkinMakeStep", ROS_CMS_DISPLAY_NAME)}};
}

} // namespace Internal
} // namespace ROSProjectManager
