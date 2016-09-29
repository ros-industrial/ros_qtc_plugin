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
#include "ui_ros_make_step.h"

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

using namespace Core;
using namespace ProjectExplorer;

namespace ROSProjectManager {
namespace Internal {

const char ROS_CMS_ID[] = "ROSProjectManager.ROSCatkinMakeStep";
const char ROS_CMS_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("ROSProjectManager::Internal::ROSCatkinMakeStep",
                                                     "catkin_make");

const char ROS_CMS_BUILD_TARGETS_KEY[] = "ROSProjectManager.ROSCatkinMakeStep.BuildTargets";
const char ROS_CMS_MAKE_ARGUMENTS_KEY[] = "ROSProjectManager.ROSCatkinMakeStep.MakeArguments";
const char ROS_CMS_MAKE_COMMAND_KEY[] = "ROSProjectManager.ROSCatkinMakeStep.MakeCommand";
const char ROS_CMS_CLEAN_KEY[] = "ROSProjectManager.ROSCatkinMakeStep.Clean";

ROSCatkinMakeStep::ROSCatkinMakeStep(BuildStepList *parent) :
    AbstractProcessStep(parent, Id(ROS_CMS_ID)),
    m_clean(false)
{
    ctor();
}

ROSCatkinMakeStep::ROSCatkinMakeStep(BuildStepList *parent, const Id id) :
    AbstractProcessStep(parent, id),
    m_clean(false)
{
    ctor();
}

ROSCatkinMakeStep::ROSCatkinMakeStep(BuildStepList *parent, ROSCatkinMakeStep *bs) :
    AbstractProcessStep(parent, bs),
    m_buildTargets(bs->m_buildTargets),
    m_makeArguments(bs->m_makeArguments),
    m_makeCommand(bs->m_makeCommand),
    m_clean(bs->m_clean)
{
    ctor();
}

void ROSCatkinMakeStep::ctor()
{
    setDefaultDisplayName(QCoreApplication::translate("ROSProjectManager::Internal::ROSCatkinMakeStep",
                                                      ROS_CMS_DISPLAY_NAME));

    m_percentProgress = QRegExp(QLatin1String("\\[\\s{0,2}(\\d{1,3})%\\]")); // Example: [ 82%] [ 82%] [ 87%]
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

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setWorkingDirectory(bc->buildDirectory().toString());
    Utils::Environment env = bc->environment();
    // Force output to english for the parsers. Do this here and not in the toolchain's
    // addToEnvironment() to not screw up the users run environment.
    env.set(QLatin1String("LC_ALL"), QLatin1String("C"));
    pp->setEnvironment(env);
    pp->setCommand(makeCommand());
    pp->setArguments(allArguments(bc->initialArguments()));
    pp->resolveAll();

    // If we are cleaning, then make can fail with an error code, but that doesn't mean
    // we should stop the clean queue
    // That is mostly so that rebuild works on an already clean project
    setIgnoreReturnValue(m_clean);

    setOutputParser(new GnuMakeParser());
    IOutputParser *parser = target()->kit()->createOutputParser();
    if (parser)
        appendOutputParser(parser);
    outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());

    return AbstractProcessStep::init(earlierSteps);
}

void ROSCatkinMakeStep::setClean(bool clean)
{
    m_clean = clean;
}

bool ROSCatkinMakeStep::isClean() const
{
    return m_clean;
}

QVariantMap ROSCatkinMakeStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());

    map.insert(QLatin1String(ROS_CMS_BUILD_TARGETS_KEY), m_buildTargets);
    map.insert(QLatin1String(ROS_CMS_MAKE_ARGUMENTS_KEY), m_makeArguments);
    map.insert(QLatin1String(ROS_CMS_MAKE_COMMAND_KEY), m_makeCommand);
    map.insert(QLatin1String(ROS_CMS_CLEAN_KEY), m_clean);
    return map;
}

bool ROSCatkinMakeStep::fromMap(const QVariantMap &map)
{
    m_buildTargets = map.value(QLatin1String(ROS_CMS_BUILD_TARGETS_KEY)).toStringList();
    m_makeArguments = map.value(QLatin1String(ROS_CMS_MAKE_ARGUMENTS_KEY)).toString();
    m_makeCommand = map.value(QLatin1String(ROS_CMS_MAKE_COMMAND_KEY)).toString();
    m_clean = map.value(QLatin1String(ROS_CMS_CLEAN_KEY)).toBool();

    return BuildStep::fromMap(map);
}

QString ROSCatkinMakeStep::allArguments(QString initial_arguments) const
{
    QString args = m_makeArguments;
    args.prepend(initial_arguments + QLatin1Char(' '));

    Utils::QtcProcess::addArgs(&args, m_buildTargets);
    return args;
}

QString ROSCatkinMakeStep::makeCommand() const
{
    QString command = m_makeCommand;
    if (command.isEmpty())
    {
      command = QLatin1String("catkin_make");
    }

    return command;
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
    return new ROSCatkinMakeStepConfigWidget(this);
}

bool ROSCatkinMakeStep::immutable() const
{
    return false;
}

bool ROSCatkinMakeStep::buildsTarget(const QString &target) const
{
    return m_buildTargets.contains(target);
}

void ROSCatkinMakeStep::setBuildTarget(const QString &target, bool on)
{
    QStringList old = m_buildTargets;
    if (on && !old.contains(target))
         old << target;
    else if (!on && old.contains(target))
        old.removeOne(target);

    m_buildTargets = old;
}

//
// ROSMakeStepConfigWidget
//

ROSCatkinMakeStepConfigWidget::ROSCatkinMakeStepConfigWidget(ROSCatkinMakeStep *makeStep)
    : m_makeStep(makeStep)
{
    m_ui = new Ui::ROSMakeStep;
    m_ui->setupUi(this);

    ROSProject *pro = static_cast<ROSProject *>(m_makeStep->target()->project());
    foreach (const QString &target, pro->buildTargets()) {
        QListWidgetItem *item = new QListWidgetItem(target, m_ui->targetsList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(m_makeStep->buildsTarget(item->text()) ? Qt::Checked : Qt::Unchecked);
    }

    m_ui->makeLineEdit->setText(m_makeStep->m_makeCommand);
    m_ui->makeArgumentsLineEdit->setText(m_makeStep->m_makeArguments);
    updateMakeOverrrideLabel();
    updateDetails();

    connect(m_ui->targetsList, &QListWidget::itemChanged,
            this, &ROSCatkinMakeStepConfigWidget::itemChanged);
    connect(m_ui->makeLineEdit, &QLineEdit::textEdited,
            this, &ROSCatkinMakeStepConfigWidget::makeLineEditTextEdited);
    connect(m_ui->makeArgumentsLineEdit, &QLineEdit::textEdited,
            this, &ROSCatkinMakeStepConfigWidget::makeArgumentsLineEditTextEdited);

    connect(ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateMakeOverrrideLabel()));
    connect(ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateDetails()));

    connect(m_makeStep->target(), SIGNAL(kitChanged()),
            this, SLOT(updateMakeOverrrideLabel()));

    connect(pro, &ROSProject::environmentChanged,
            this, &ROSCatkinMakeStepConfigWidget::updateMakeOverrrideLabel);
    connect(pro, &ROSProject::environmentChanged,
            this, &ROSCatkinMakeStepConfigWidget::updateDetails);
}

ROSCatkinMakeStepConfigWidget::~ROSCatkinMakeStepConfigWidget()
{
    delete m_ui;
}

QString ROSCatkinMakeStepConfigWidget::displayName() const
{
    return tr("Make", "GenericMakestep display name.");
}

void ROSCatkinMakeStepConfigWidget::updateMakeOverrrideLabel()
{
    BuildConfiguration *bc = m_makeStep->buildConfiguration();
    if (!bc)
        bc = m_makeStep->target()->activeBuildConfiguration();

    m_ui->makeLabel->setText(tr("Override %1:").arg(QDir::toNativeSeparators(m_makeStep->makeCommand())));
}

void ROSCatkinMakeStepConfigWidget::updateDetails()
{
    ROSBuildConfiguration *bc = m_makeStep->rosBuildConfiguration();
    if (!bc)
        bc = m_makeStep->targetsActiveBuildConfiguration();

    ProcessParameters param;
    param.setMacroExpander(bc->macroExpander());
    param.setWorkingDirectory(bc->buildDirectory().toString());
    param.setEnvironment(bc->environment());
    param.setCommand(m_makeStep->makeCommand());
    param.setArguments(m_makeStep->allArguments(bc->initialArguments()));
    m_summaryText = param.summary(displayName());
    emit updateSummary();
}

QString ROSCatkinMakeStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

void ROSCatkinMakeStepConfigWidget::itemChanged(QListWidgetItem *item)
{
    m_makeStep->setBuildTarget(item->text(), item->checkState() & Qt::Checked);
    updateDetails();
}

void ROSCatkinMakeStepConfigWidget::makeLineEditTextEdited()
{
    m_makeStep->m_makeCommand = m_ui->makeLineEdit->text();
    updateDetails();
}

void ROSCatkinMakeStepConfigWidget::makeArgumentsLineEditTextEdited()
{
    m_makeStep->m_makeArguments = m_ui->makeArgumentsLineEdit->text();
    updateDetails();
}

//
// ROSMakeStepFactory
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
        step->setClean(true);
        step->setBuildTarget(QLatin1String("clean"), /* on = */ true);
    } else if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_BUILD) {
        step->setBuildTarget(QLatin1String("all"), /* on = */ true);
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
    if (parent->target()->project()->id() != Constants::ROSPROJECT_ID)
        return {};

    return {{ROS_CMS_ID,  QCoreApplication::translate("ROSProjectManager::Internal::ROSMakeStep", ROS_CMS_DISPLAY_NAME)}};
}

} // namespace Internal
} // namespace ROSProjectManager
