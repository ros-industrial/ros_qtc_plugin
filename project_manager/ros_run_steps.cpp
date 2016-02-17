#include "ros_run_steps.h"
#include "ros_project_constants.h"
#include "ui_ros_launch_configuration.h"

#include <projectexplorer/project.h>

namespace ROSProjectManager {
namespace Internal {

const char ROS_LAUNCH_ID[] = "ROSProjectManager.ROSLaunchStep";

const char ROS_LAUNCH_PACKAGE_NAME_KEY[] = "ROSProjectManager.ROSLaunchStep.PackageName";
const char ROS_LAUNCH_TARGET_KEY[] = "ROSProjectManager.ROSLaunchStep.Target";
const char ROS_LAUNCH_ARGUMENTS_KEY[] = "ROSProjectManager.ROSLaunchStep.Arguments";

ROSLaunchStep::ROSLaunchStep(RunStepList *parent) :
    ROSLaunchStep(parent, Core::Id(ROS_LAUNCH_ID))
{
}

ROSLaunchStep::ROSLaunchStep(RunStepList *rsl, Core::Id id) : RunStep(rsl, id)
{
}

ROSLaunchStep::ROSLaunchStep(RunStepList *rsl, RunStep *rs) : RunStep(rsl, rs)
{
}

ROSLaunchStep::~ROSLaunchStep()
{
}

bool ROSLaunchStep::init(QList<const RunStep *> &earlierSteps)
{
    ROSRunConfiguration *rc = rosRunConfiguration();
    if (!rc)
        rc = targetsActiveRunConfiguration();

    if (!rc)
    {
        return false;
        //emit addTask(ProjectExplorer::Task::Error);
    }

    return true;
}
void ROSLaunchStep::run(QFutureInterface<bool> &fi)
{
  //Need to implement the roslaunch
}

QVariantMap ROSLaunchStep::toMap() const
{
    QVariantMap map(RunStep::toMap());

    map.insert(QLatin1String(ROS_LAUNCH_PACKAGE_NAME_KEY), m_packageName);
    map.insert(QLatin1String(ROS_LAUNCH_TARGET_KEY), m_target);
    map.insert(QLatin1String(ROS_LAUNCH_ARGUMENTS_KEY), m_arguments);
    return map;
}

bool ROSLaunchStep::fromMap(const QVariantMap &map)
{
    m_packageName = map.value(QLatin1String(ROS_LAUNCH_PACKAGE_NAME_KEY)).toString();
    m_target = map.value(QLatin1String(ROS_LAUNCH_TARGET_KEY)).toString();
    m_arguments = map.value(QLatin1String(ROS_LAUNCH_ARGUMENTS_KEY)).toString();

    return RunStep::fromMap(map);
}

ROSRunConfiguration *ROSLaunchStep::rosRunConfiguration() const
{
    return static_cast<ROSRunConfiguration *>(runConfiguration());
}

ROSRunConfiguration *ROSLaunchStep::targetsActiveRunConfiguration() const
{
    return static_cast<ROSRunConfiguration *>(target()->activeRunConfiguration());
}

RunStepConfigWidget *ROSLaunchStep::createConfigWidget()
{
    return new ROSLaunchStepConfigWidget(this);
}

//
// ROSLaunchStepConfigWidget
//

ROSLaunchStepConfigWidget::ROSLaunchStepConfigWidget(ROSLaunchStep *launchStep)
    : m_rosLaunchStep(launchStep)
{
    m_ui = new Ui::ROSLaunchStep;
    m_ui->setupUi(this);

//    ROSProject *pro = static_cast<ROSProject *>(m_makeStep->target()->project());
//    foreach (const QString &target, pro->buildTargets()) {
//        QListWidgetItem *item = new QListWidgetItem(target, m_ui->targetsList);
//        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
//        item->setCheckState(m_makeStep->buildsTarget(item->text()) ? Qt::Checked : Qt::Unchecked);
//    }

//    m_ui->makeLineEdit->setText(m_makeStep->m_makeCommand);
//    m_ui->makeArgumentsLineEdit->setText(m_makeStep->m_makeArguments);
//    updateMakeOverrrideLabel();
//    updateDetails();

//    connect(m_ui->targetsList, &QListWidget::itemChanged,
//            this, &ROSMakeStepConfigWidget::itemChanged);
//    connect(m_ui->makeLineEdit, &QLineEdit::textEdited,
//            this, &ROSMakeStepConfigWidget::makeLineEditTextEdited);
//    connect(m_ui->makeArgumentsLineEdit, &QLineEdit::textEdited,
//            this, &ROSMakeStepConfigWidget::makeArgumentsLineEditTextEdited);

//    connect(ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
//            this, SLOT(updateMakeOverrrideLabel()));
//    connect(ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
//            this, SLOT(updateDetails()));

//    connect(m_makeStep->target(), SIGNAL(kitChanged()),
//            this, SLOT(updateMakeOverrrideLabel()));

//    connect(pro, &ROSProject::environmentChanged,
//            this, &ROSMakeStepConfigWidget::updateMakeOverrrideLabel);
//    connect(pro, &ROSProject::environmentChanged,
//            this, &ROSMakeStepConfigWidget::updateDetails);
}

ROSLaunchStepConfigWidget::~ROSLaunchStepConfigWidget()
{
    delete m_ui;
}

QString ROSLaunchStepConfigWidget::displayName() const
{
    return tr("ROS Launch", "ROS Launch run step display name.");
}

QString ROSLaunchStepConfigWidget::summaryText() const
{
  return QString::fromLatin1("<b>%1:</b> %2 %3")
          .arg(displayName(),
               m_ui->packageNameComboBox->currentText(),
               m_ui->argumentsLineEdit->text());
}

//
// ROSRunStepFactory
//

ROSRunStepFactory::ROSRunStepFactory(QObject *parent) :
    IRunStepFactory(parent)
{
}

bool ROSRunStepFactory::canCreate(RunStepList *parent, const Core::Id id) const
{
    if (parent->target()->project()->id() == Constants::ROSPROJECT_ID)
        return id == ROS_LAUNCH_ID;
    return false;
}

RunStep *ROSRunStepFactory::create(RunStepList *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    ROSLaunchStep *step = new ROSLaunchStep(parent, id);

    return step;
}

bool ROSRunStepFactory::canClone(RunStepList *parent, RunStep *source) const
{
    return canCreate(parent, source->id());
}

RunStep *ROSRunStepFactory::clone(RunStepList *parent, RunStep *source)
{
    if (!canClone(parent, source))
        return 0;
    ROSLaunchStep *old(qobject_cast<ROSLaunchStep *>(source));
    Q_ASSERT(old);
    return new ROSLaunchStep(parent, old);
}

bool ROSRunStepFactory::canRestore(RunStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

RunStep *ROSRunStepFactory::restore(RunStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    ROSLaunchStep *rs(new ROSLaunchStep(parent, ProjectExplorer::idFromMap(map)));
    if (rs->fromMap(map))
        return rs;
    delete rs;
    return 0;
}

QList<Core::Id> ROSRunStepFactory::availableCreationIds(RunStepList *parent) const
{
    if (parent->target()->project()->id() == Constants::ROSPROJECT_ID)
        return QList<Core::Id>() << Core::Id(ROS_LAUNCH_ID);
    return QList<Core::Id>();
}

QString ROSRunStepFactory::displayNameForId(const Core::Id id) const
{
    if (id == ROS_LAUNCH_ID)
        return tr("ROS Launch Step");

    return QString();
}


} // namespace Internal
} // namespace ROSProjectManager
