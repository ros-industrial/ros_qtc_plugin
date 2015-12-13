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

#include "ros_make_step.h"
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

const char ROS_MS_ID[] = "ROSProjectManager.ROSMakeStep";
const char ROS_MS_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("ROSProjectManager::Internal::ROSMakeStep",
                                                     "catkin_make");

const char BUILD_TARGETS_KEY[] = "ROSProjectManager.ROSMakeStep.BuildTargets";
const char MAKE_ARGUMENTS_KEY[] = "ROSProjectManager.ROSMakeStep.MakeArguments";
const char MAKE_COMMAND_KEY[] = "ROSProjectManager.ROSMakeStep.MakeCommand";
const char CLEAN_KEY[] = "ROSProjectManager.ROSMakeStep.Clean";

ROSMakeStep::ROSMakeStep(BuildStepList *parent) :
    AbstractProcessStep(parent, Id(ROS_MS_ID)),
    m_clean(false)
{
    ctor();
}

ROSMakeStep::ROSMakeStep(BuildStepList *parent, const Id id) :
    AbstractProcessStep(parent, id),
    m_clean(false)
{
    ctor();
}

ROSMakeStep::ROSMakeStep(BuildStepList *parent, ROSMakeStep *bs) :
    AbstractProcessStep(parent, bs),
    m_buildTargets(bs->m_buildTargets),
    m_makeArguments(bs->m_makeArguments),
    m_makeCommand(bs->m_makeCommand),
    m_clean(bs->m_clean)
{
    ctor();
}

void ROSMakeStep::ctor()
{
    setDefaultDisplayName(QCoreApplication::translate("ROSProjectManager::Internal::ROSMakeStep",
                                                      ROS_MS_DISPLAY_NAME));
}

ROSBuildConfiguration *ROSMakeStep::rosBuildConfiguration() const
{
    return static_cast<ROSBuildConfiguration *>(buildConfiguration());
}

ROSBuildConfiguration *ROSMakeStep::targetsActiveBuildConfiguration() const
{
    return static_cast<ROSBuildConfiguration *>(target()->activeBuildConfiguration());
}

//ROSBuildConfiguration *ROSMakeStep::targetsActiveRunConfiguration() const
//{
//    return qobject_cast<ROSRunConfiguration *>(target()->activeRunConfiguration());
//}

//void ROSMakeStep::activeBuildConfigurationChanged()
//{
//    if (m_activeConfiguration)
//        disconnect(m_activeConfiguration, &CMakeBuildConfiguration::useNinjaChanged, this, &MakeStep::makeCommandChanged);

//    m_activeConfiguration = targetsActiveBuildConfiguration();

//    if (m_activeConfiguration)
//        connect(m_activeConfiguration, &CMakeBuildConfiguration::useNinjaChanged, this, &MakeStep::makeCommandChanged);

//    emit makeCommandChanged();
//}

ROSMakeStep::~ROSMakeStep()
{
}

bool ROSMakeStep::init(QList<const BuildStep *> &earlierSteps)
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

void ROSMakeStep::setClean(bool clean)
{
    m_clean = clean;
}

bool ROSMakeStep::isClean() const
{
    return m_clean;
}

QVariantMap ROSMakeStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());

    map.insert(QLatin1String(BUILD_TARGETS_KEY), m_buildTargets);
    map.insert(QLatin1String(MAKE_ARGUMENTS_KEY), m_makeArguments);
    map.insert(QLatin1String(MAKE_COMMAND_KEY), m_makeCommand);
    map.insert(QLatin1String(CLEAN_KEY), m_clean);
    return map;
}

bool ROSMakeStep::fromMap(const QVariantMap &map)
{
    m_buildTargets = map.value(QLatin1String(BUILD_TARGETS_KEY)).toStringList();
    m_makeArguments = map.value(QLatin1String(MAKE_ARGUMENTS_KEY)).toString();
    m_makeCommand = map.value(QLatin1String(MAKE_COMMAND_KEY)).toString();
    m_clean = map.value(QLatin1String(CLEAN_KEY)).toBool();

    return BuildStep::fromMap(map);
}

QString ROSMakeStep::allArguments(QString initial_arguments) const
{
    QString args = m_makeArguments;
    args.prepend(initial_arguments + QLatin1Char(' '));

    Utils::QtcProcess::addArgs(&args, m_buildTargets);
    return args;
}

QString ROSMakeStep::makeCommand() const
{
    QString command = m_makeCommand;
    if (command.isEmpty())
    {
      command = QLatin1String("catkin_make");
    }

    return command;
}

void ROSMakeStep::run(QFutureInterface<bool> &fi)
{
    AbstractProcessStep::run(fi);
}

BuildStepConfigWidget *ROSMakeStep::createConfigWidget()
{
    return new ROSMakeStepConfigWidget(this);
}

bool ROSMakeStep::immutable() const
{
    return false;
}

bool ROSMakeStep::buildsTarget(const QString &target) const
{
    return m_buildTargets.contains(target);
}

void ROSMakeStep::setBuildTarget(const QString &target, bool on)
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

ROSMakeStepConfigWidget::ROSMakeStepConfigWidget(ROSMakeStep *makeStep)
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
            this, &ROSMakeStepConfigWidget::itemChanged);
    connect(m_ui->makeLineEdit, &QLineEdit::textEdited,
            this, &ROSMakeStepConfigWidget::makeLineEditTextEdited);
    connect(m_ui->makeArgumentsLineEdit, &QLineEdit::textEdited,
            this, &ROSMakeStepConfigWidget::makeArgumentsLineEditTextEdited);

    connect(ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateMakeOverrrideLabel()));
    connect(ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateDetails()));

    connect(m_makeStep->target(), SIGNAL(kitChanged()),
            this, SLOT(updateMakeOverrrideLabel()));

    connect(pro, &ROSProject::environmentChanged,
            this, &ROSMakeStepConfigWidget::updateMakeOverrrideLabel);
    connect(pro, &ROSProject::environmentChanged,
            this, &ROSMakeStepConfigWidget::updateDetails);
}

ROSMakeStepConfigWidget::~ROSMakeStepConfigWidget()
{
    delete m_ui;
}

QString ROSMakeStepConfigWidget::displayName() const
{
    return tr("Make", "GenericMakestep display name.");
}

void ROSMakeStepConfigWidget::updateMakeOverrrideLabel()
{
    BuildConfiguration *bc = m_makeStep->buildConfiguration();
    if (!bc)
        bc = m_makeStep->target()->activeBuildConfiguration();

    m_ui->makeLabel->setText(tr("Override %1:").arg(QDir::toNativeSeparators(m_makeStep->makeCommand())));
}

void ROSMakeStepConfigWidget::updateDetails()
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

QString ROSMakeStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

void ROSMakeStepConfigWidget::itemChanged(QListWidgetItem *item)
{
    m_makeStep->setBuildTarget(item->text(), item->checkState() & Qt::Checked);
    updateDetails();
}

void ROSMakeStepConfigWidget::makeLineEditTextEdited()
{
    m_makeStep->m_makeCommand = m_ui->makeLineEdit->text();
    updateDetails();
}

void ROSMakeStepConfigWidget::makeArgumentsLineEditTextEdited()
{
    m_makeStep->m_makeArguments = m_ui->makeArgumentsLineEdit->text();
    updateDetails();
}

//
// ROSMakeStepFactory
//

ROSMakeStepFactory::ROSMakeStepFactory(QObject *parent) :
    IBuildStepFactory(parent)
{
}

bool ROSMakeStepFactory::canCreate(BuildStepList *parent, const Id id) const
{
    if (parent->target()->project()->id() == Constants::ROSPROJECT_ID)
        return id == ROS_MS_ID;
    return false;
}

BuildStep *ROSMakeStepFactory::create(BuildStepList *parent, const Id id)
{
    if (!canCreate(parent, id))
        return 0;
    ROSMakeStep *step = new ROSMakeStep(parent);
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN) {
        step->setClean(true);
        step->setBuildTarget(QLatin1String("clean"), /* on = */ true);
    } else if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_BUILD) {
        step->setBuildTarget(QLatin1String("all"), /* on = */ true);
    }
    return step;
}

bool ROSMakeStepFactory::canClone(BuildStepList *parent, BuildStep *source) const
{
    return canCreate(parent, source->id());
}

BuildStep *ROSMakeStepFactory::clone(BuildStepList *parent, BuildStep *source)
{
    if (!canClone(parent, source))
        return 0;
    ROSMakeStep *old(qobject_cast<ROSMakeStep *>(source));
    Q_ASSERT(old);
    return new ROSMakeStep(parent, old);
}

bool ROSMakeStepFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

BuildStep *ROSMakeStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    ROSMakeStep *bs(new ROSMakeStep(parent));
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QList<Id> ROSMakeStepFactory::availableCreationIds(BuildStepList *parent) const
{
    if (parent->target()->project()->id() == Constants::ROSPROJECT_ID)
        return QList<Id>() << Id(ROS_MS_ID);
    return QList<Id>();
}

QString ROSMakeStepFactory::displayNameForId(const Id id) const
{
    if (id == ROS_MS_ID)
        return QCoreApplication::translate("ROSProjectManager::Internal::ROSMakeStep",
                                           ROS_MS_DISPLAY_NAME);
    return QString();
}

} // namespace Internal
} // namespace ROSProjectManager
