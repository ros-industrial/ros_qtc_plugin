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
#include "ros_run_step.h"
#include "ros_project_constants.h"

#include <coreplugin/messagemanager.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <utils/algorithm.h>
#include <QDebug>

static const char runStepEnabledKey[] = "ProjectExplorer.RunStep.Enabled";



namespace ROSProjectManager {
namespace Internal {

static QList<RunStepFactory *> g_runStepFactories;

RunStepFactory::RunStepFactory()
{
    g_runStepFactories.append(this);
}

RunStepFactory::~RunStepFactory()
{
    g_runStepFactories.removeOne(this);
}

const QList<RunStepFactory *> RunStepFactory::allRunStepFactories()
{
    return g_runStepFactories;
}

bool RunStepFactory::canHandle(RunStepList *rsl) const
{
    if (!m_supportedStepLists.isEmpty() && !m_supportedStepLists.contains(rsl->id()))
        return false;

    auto config = qobject_cast<ProjectExplorer::ProjectConfiguration *>(rsl->parent());

    if (!m_supportedDeviceTypes.isEmpty()) {
        ProjectExplorer::Target *target = rsl->target();
        QTC_ASSERT(target, return false);
        Core::Id deviceType = ProjectExplorer::DeviceTypeKitAspect::deviceTypeId(target->kit());
        if (!m_supportedDeviceTypes.contains(deviceType))
            return false;
    }

    if (m_supportedProjectType.isValid()) {
        if (!config)
            return false;
        Core::Id projectId = config->project()->id();
        if (projectId != m_supportedProjectType)
            return false;
    }

    if (!m_isRepeatable && rsl->contains(m_info.id))
        return false;

    if (m_supportedConfiguration.isValid()) {
        if (!config)
            return false;
        Core::Id configId = config->id();
        if (configId != m_supportedConfiguration)
            return false;
    }

    return true;
}

void RunStepFactory::setDisplayName(const QString &displayName)
{
    m_info.displayName = displayName;
}

void RunStepFactory::setFlags(RunStepInfo::Flags flags)
{
    m_info.flags = flags;
}

void RunStepFactory::setSupportedStepList(Core::Id id)
{
    m_supportedStepLists = {id};
}

void RunStepFactory::setSupportedStepLists(const QList<Core::Id> &ids)
{
    m_supportedStepLists = ids;
}

void RunStepFactory::setSupportedConfiguration(Core::Id id)
{
    m_supportedConfiguration = id;
}

void RunStepFactory::setSupportedProjectType(Core::Id id)
{
    m_supportedProjectType = id;
}

void RunStepFactory::setSupportedDeviceType(Core::Id id)
{
    m_supportedDeviceTypes = {id};
}

void RunStepFactory::setSupportedDeviceTypes(const QList<Core::Id> &ids)
{
    m_supportedDeviceTypes = ids;
}

RunStepInfo RunStepFactory::stepInfo() const
{
    return m_info;
}

Core::Id RunStepFactory::stepId() const
{
    return m_info.id;
}

RunStep *RunStepFactory::create(RunStepList *parent, Core::Id id)
{
    RunStep *rs = nullptr;
    if (id == m_info.id)
        rs = m_info.creator(parent);
    return rs;
}

RunStep *RunStepFactory::restore(RunStepList *parent, const QVariantMap &map)
{
    RunStep *rs = m_info.creator(parent);
    if (!rs)
        return nullptr;
    if (!rs->fromMap(map)) {
        QTC_CHECK(false);
        delete rs;
        return nullptr;
    }
    return rs;
}

namespace {
const char STEPS_COUNT_KEY[] = "ProjectExplorer.RunStepList.StepsCount";
const char STEPS_PREFIX[] = "ProjectExplorer.RunStepList.Step.";
}


RunStep::RunStep(RunStepList *rsl, Core::Id id) :
    ProjectConfiguration(rsl, id)
{
  Utils::MacroExpander *expander = macroExpander();
  expander->setDisplayName(tr("Run Step"));
  expander->setAccumulating(true);
  expander->registerSubProvider([this] { return projectConfiguration()->macroExpander(); });
}

bool RunStep::fromMap(const QVariantMap &map)
{
    m_enabled = map.value(runStepEnabledKey, true).toBool();
    return ProjectConfiguration::fromMap(map);
}

QVariantMap RunStep::toMap() const
{
    QVariantMap map = ProjectConfiguration::toMap();
    map.insert(runStepEnabledKey, m_enabled);
    return map;
}

ProjectExplorer::RunConfiguration *RunStep::runConfiguration() const
{
    auto config = qobject_cast<ProjectExplorer::RunConfiguration *>(parent()->parent());
    if (config)
      return config;

    return target()->activeRunConfiguration();
}

ProjectExplorer::ProjectConfiguration *RunStep::projectConfiguration() const
{
    return static_cast<ProjectConfiguration *>(parent()->parent());
}

ProjectExplorer::Target *RunStep::target() const
{
    return qobject_cast<ProjectExplorer::Target *>(parent()->parent()->parent());
}

ProjectExplorer::Project *RunStep::project() const
{
    return target()->project();
}

/*!
    If this function returns \c true, the user cannot delete this build step for
    this target and the user is prevented from changing the order in which
    immutable steps are run. The default implementation returns \c false.
*/

bool RunStep::immutable() const
{
    return false;
}

bool RunStep::runInGuiThread() const
{
    return false;
}

/*!
    This function needs to be reimplemented only for build steps that return
    \c false from runInGuiThread().

    \sa runInGuiThread()
*/
void RunStep::cancel()
{
    // Do nothing
}

void RunStep::setEnabled(bool b)
{
    if (m_enabled == b)
        return;
    m_enabled = b;
    emit enabledChanged();
}

bool RunStep::enabled() const
{
    return m_enabled;
}

RunStepList::RunStepList(QObject *parent, Core::Id id) :
    ProjectConfiguration(parent, id)
{
    Q_ASSERT(parent);
    setDefaultDisplayName(tr("Run"));
}


RunStepList::~RunStepList()
{
    clear();
}

void RunStepList::clear()
{
    qDeleteAll(m_steps);
    m_steps.clear();
}

QVariantMap RunStepList::toMap() const
{
    QVariantMap map(ProjectConfiguration::toMap());
    // Save run steps
    map.insert(QString::fromLatin1(STEPS_COUNT_KEY), m_steps.count());
    for (int i = 0; i < m_steps.count(); ++i)
        map.insert(QString::fromLatin1(STEPS_PREFIX) + QString::number(i), m_steps.at(i)->toMap());

    return map;
}

int RunStepList::count() const
{
    return m_steps.count();
}

bool RunStepList::isEmpty() const
{
    return m_steps.isEmpty();
}

bool RunStepList::contains(Core::Id id) const
{
    return Utils::anyOf(steps(), [id](RunStep *rs){
        return rs->id() == id;
    });
}

bool RunStepList::enabled() const
{
    for (RunStep *rs : m_steps) {
        if (rs->enabled() == true)
            return true;
    }
    return false;
}

bool RunStepList::fromMap(const QVariantMap &map)
{
    clear();

    // We need the ID set before trying to restore the steps!
    if (!ProjectConfiguration::fromMap(map))
        return false;

    const QList<RunStepFactory *> factories = RunStepFactory::allRunStepFactories();
    int maxSteps = map.value(QString::fromLatin1(STEPS_COUNT_KEY), 0).toInt();
    for (int i = 0; i < maxSteps; ++i) {
        QVariantMap rsData(map.value(QString::fromLatin1(STEPS_PREFIX) + QString::number(i)).toMap());
        if (rsData.isEmpty()) {
            Core::MessageManager::write(tr("[ROS Warning] No step data found for step %1 (continuing).").arg(i));
            continue;
        }
        bool handled = false;
        Core::Id stepId = ProjectExplorer::idFromMap(rsData);
        for (RunStepFactory *factory : factories) {
            if (factory->stepId() == stepId) {
                if (factory->canHandle(this)) {
                    if (RunStep *rs = factory->restore(this, rsData)) {
                        appendStep(rs);
                        handled = true;
                    } else {
                        Core::MessageManager::write(tr("[ROS Warning] Restoration of step %1 failed (continuing).").arg(i));
                    }
                }
            }
        }
        QTC_ASSERT(handled, qDebug() << "No factory for run step" << stepId.toString() << "found.");
    }
    return true;
}

void RunStepList::runStep_enabledChanged(RunStep *step)
{
    if (step->enabled() == true)
    {
        for (RunStep *rs : m_steps)
        {
            if (rs->enabled() == true && rs->id() == Constants::ROS_ATTACH_TO_NODE_ID && rs != step)
            {
                rs->setEnabled(false);
            }
        }
    }
}

QList<RunStep *> RunStepList::steps() const
{
    return m_steps;
}

QList<RunStep *> RunStepList::steps(const std::function<bool (const RunStep *)> &filter) const
{
    return Utils::filtered(steps(), filter);
}

void RunStepList::insertStep(int position, RunStep *step)
{
    if (step->id() == Constants::ROS_ATTACH_TO_NODE_ID)
    {
        if (step->enabled() == true)
        {
            for (RunStep *rs : m_steps)
            {
                if (rs->enabled() == true && rs->id() == Constants::ROS_ATTACH_TO_NODE_ID)
                {
                    rs->setEnabled(false);
                }
            }
        }
        connect(step, &RunStep::enabledChanged,
                this, [this, step] () {runStep_enabledChanged(step);});
    }
    m_steps.insert(position, step);
    emit stepInserted(position);
}

bool RunStepList::removeStep(int position)
{
    RunStep *rs = at(position);

    emit aboutToRemoveStep(position);
    m_steps.removeAt(position);
    delete rs;
    emit stepRemoved(position);
    return true;
}

void RunStepList::moveStepUp(int position)
{
    m_steps.swap(position - 1, position);
    emit stepMoved(position, position - 1);
}

RunStep *RunStepList::at(int position)
{
    return m_steps.at(position);
}

ProjectExplorer::Target *RunStepList::target() const
{
    Q_ASSERT(parent());
    ProjectExplorer::RunConfiguration *rc = qobject_cast<ProjectExplorer::RunConfiguration *>(parent());
    if (rc)
      return rc->target();

    return 0;
}

} // namespace Internal
} // namespace ROSProjectManager
