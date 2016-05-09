#include "ros_run_steps.h"
#include "ros_project_constants.h"
#include "ros_project.h"
#include "ui_ros_generic_configuration.h"
#include "ros_build_configuration.h"
#include "ros_utils.h"
#include "ros_terminal_pane.h"

#include <projectexplorer/project.h>
#include <projectexplorer/processparameters.h>

namespace ROSProjectManager {
namespace Internal {

const char ROS_LAUNCH_ID[] = "ROSProjectManager.ROSLaunchStep";
const char ROS_RUN_ID[] = "ROSProjectManager.ROSRunStep";

const char ROS_GENERIC_COMMAND_KEY[] = "ROSProjectManager.ROSGenericStep.Command";
const char ROS_GENERIC_PACKAGE_KEY[] = "ROSProjectManager.ROSGenericStep.Package";
const char ROS_GENERIC_TARGET_KEY[] = "ROSProjectManager.ROSGenericStep.Target";
const char ROS_GENERIC_ARGUMENTS_KEY[] = "ROSProjectManager.ROSGenericStep.Arguments";

//ROSLaunchStep::ROSLaunchStep(RunStepList *parent) :
//    ROSLaunchStep(parent, Core::Id(ROS_LAUNCH_ID))
//{
//}

//ROSLaunchStep::ROSLaunchStep(RunStepList *rsl, Core::Id id) : RunStep(rsl, id)
//{
//}

//ROSLaunchStep::ROSLaunchStep(RunStepList *rsl, RunStep *rs) : RunStep(rsl, rs)
//{
//}

//ROSLaunchStep::~ROSLaunchStep()
//{
//}

//bool ROSLaunchStep::init(QList<const RunStep *> &earlierSteps)
//{
//    ROSRunConfiguration *rc = rosRunConfiguration();
//    if (!rc)
//        rc = targetsActiveRunConfiguration();

//    if (!rc)
//    {
//        return false;
//    }

//    return true;
//}
//void ROSLaunchStep::run(QFutureInterface<bool> &fi)
//{
//  ROSProject *rp = qobject_cast<ROSProject *>(target()->project());
//  ROSBuildConfiguration *bc = qobject_cast<ROSBuildConfiguration *>(target()->activeBuildConfiguration());

//  QString command;
//  command = QString::fromLatin1("%1 %2 %3 %4\n")
//      .arg(QLatin1String("roslaunch"),
//           m_packageName,
//           m_target,
//           m_arguments);

//  //create terminal without starting shell
//  QTermWidget &terminal = ROSManager::instance()->startTerminal(0, QString::fromLatin1("%1 %2 %3").arg(QLatin1String("roslaunch"), m_packageName, m_target));
//  terminal.setWorkingDirectory(rp->projectDirectory().toString());
//  terminal.setEnvironment(bc->environment().toStringList());

//  //start bash now that everything is setup
//  terminal.startShellProgram();

//  //send roslaunch command
//  terminal.sendText(command);
//}

//QVariantMap ROSLaunchStep::toMap() const
//{
//    QVariantMap map(RunStep::toMap());

//    map.insert(QLatin1String(ROS_LAUNCH_PACKAGE_NAME_KEY), m_packageName);
//    map.insert(QLatin1String(ROS_LAUNCH_TARGET_KEY), m_target);
//    map.insert(QLatin1String(ROS_LAUNCH_ARGUMENTS_KEY), m_arguments);
//    return map;
//}

//bool ROSLaunchStep::fromMap(const QVariantMap &map)
//{
//    m_packageName = map.value(QLatin1String(ROS_LAUNCH_PACKAGE_NAME_KEY)).toString();
//    m_target = map.value(QLatin1String(ROS_LAUNCH_TARGET_KEY)).toString();
//    m_arguments = map.value(QLatin1String(ROS_LAUNCH_ARGUMENTS_KEY)).toString();

//    return RunStep::fromMap(map);
//}

//ROSRunConfiguration *ROSLaunchStep::rosRunConfiguration() const
//{
//    return static_cast<ROSRunConfiguration *>(runConfiguration());
//}

//ROSRunConfiguration *ROSLaunchStep::targetsActiveRunConfiguration() const
//{
//    return static_cast<ROSRunConfiguration *>(target()->activeRunConfiguration());
//}

//RunStepConfigWidget *ROSLaunchStep::createConfigWidget()
//{
//    return new ROSLaunchStepConfigWidget(this);
//}

//QString ROSLaunchStep::packageName() const
//{
//  return m_packageName;
//}

//QString ROSLaunchStep::launchFileName() const
//{
//  return m_target;
//}

//QString ROSLaunchStep::launchFileArguments() const
//{
//  return m_arguments;
//}

//void ROSLaunchStep::setPackageName(const QString &packageName)
//{
//  m_packageName = packageName;
//}

//void ROSLaunchStep::setLaunchFileName(const QString &launchFileName)
//{
//  m_target = launchFileName;
//}

//void ROSLaunchStep::setLaunchFileArguments(const QString &launchFileName)
//{
//  m_arguments = launchFileName;
//}


////
//// ROSLaunchStepConfigWidget
////

//ROSLaunchStepConfigWidget::ROSLaunchStepConfigWidget(ROSLaunchStep *launchStep)
//    : m_rosLaunchStep(launchStep)
//{
//    m_ui = new Ui::ROSLaunchStep();
//    m_ui->setupUi(this);
//    m_ui->packageNameComboBox->setStyleSheet(tr("combobox-popup: 0;"));
//    m_ui->launchFileComboBox->setStyleSheet(tr("combobox-popup: 0;"));
//    m_ui->packageNameComboBox->addItems(ROSUtils::getROSPackages());

//    int idx;
//    idx = m_ui->packageNameComboBox->findText(launchStep->packageName(), Qt::MatchExactly);
//    m_ui->packageNameComboBox->setCurrentIndex(idx);

//    m_ui->launchFileComboBox->addItems(ROSUtils::getROSPackageLaunchFiles(m_ui->packageNameComboBox->currentText()));
//    idx = m_ui->launchFileComboBox->findText(launchStep->launchFileName(), Qt::MatchExactly);
//    m_ui->launchFileComboBox->setCurrentIndex(idx);

//    m_ui->argumentsLineEdit->setText(launchStep->launchFileArguments());

//    connect(m_ui->packageNameComboBox, SIGNAL(currentIndexChanged(QString)),
//            this, SLOT(packageNameComboBox_currentIndexChanged(QString)));

//    connect(m_ui->launchFileComboBox, SIGNAL(currentIndexChanged(QString)),
//            this, SLOT(launchFileComboBox_currentIndexChanged(QString)));

//    connect(m_ui->argumentsLineEdit, SIGNAL(textChanged(QString)),
//            this, SLOT(argumentsLineEdit_textChanged(QString)));


////    ROSProject *pro = static_cast<ROSProject *>(m_makeStep->target()->project());
////    foreach (const QString &target, pro->buildTargets()) {
////        QListWidgetItem *item = new QListWidgetItem(target, m_ui->targetsList);
////        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
////        item->setCheckState(m_makeStep->buildsTarget(item->text()) ? Qt::Checked : Qt::Unchecked);
////    }

////    m_ui->makeLineEdit->setText(m_makeStep->m_makeCommand);
////    m_ui->makeArgumentsLineEdit->setText(m_makeStep->m_makeArguments);
////    updateMakeOverrrideLabel();
////    updateDetails();

////    connect(m_ui->targetsList, &QListWidget::itemChanged,
////            this, &ROSMakeStepConfigWidget::itemChanged);
////    connect(m_ui->makeLineEdit, &QLineEdit::textEdited,
////            this, &ROSMakeStepConfigWidget::makeLineEditTextEdited);
////    connect(m_ui->makeArgumentsLineEdit, &QLineEdit::textEdited,
////            this, &ROSMakeStepConfigWidget::makeArgumentsLineEditTextEdited);

////    connect(ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
////            this, SLOT(updateMakeOverrrideLabel()));
////    connect(ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
////            this, SLOT(updateDetails()));

////    connect(m_makeStep->target(), SIGNAL(kitChanged()),
////            this, SLOT(updateMakeOverrrideLabel()));

////    connect(pro, &ROSProject::environmentChanged,
////            this, &ROSMakeStepConfigWidget::updateMakeOverrrideLabel);
////    connect(pro, &ROSProject::environmentChanged,
////            this, &ROSMakeStepConfigWidget::updateDetails);
//}

//ROSLaunchStepConfigWidget::~ROSLaunchStepConfigWidget()
//{
//    delete m_ui;
//}

//QString ROSLaunchStepConfigWidget::displayName() const
//{
//    return tr("ROS Run Step", "ROS Launch run step display name.");
//}

//QString ROSLaunchStepConfigWidget::summaryText() const
//{
//  //this is causing a the plugin to crash
//  return QString::fromLatin1("<b>%1:</b> %2 %3 %4 %5")
//          .arg(displayName(),
//               QLatin1String("roslaunch"),
//               m_ui->packageNameComboBox->currentText(),
//               m_ui->launchFileComboBox->currentText(),
//               m_ui->argumentsLineEdit->text());
//}

//void ROSLaunchStepConfigWidget::packageNameComboBox_currentIndexChanged(const QString &arg1)
//{
//  m_ui->launchFileComboBox->addItems(ROSUtils::getROSPackageLaunchFiles(arg1));
//  m_rosLaunchStep->setPackageName(arg1);
//  emit updateSummary();
//}

//void ROSLaunchStepConfigWidget::launchFileComboBox_currentIndexChanged(const QString &arg1)
//{
//  m_rosLaunchStep->setLaunchFileName(arg1);
//  emit updateSummary();
//}

//void ROSLaunchStepConfigWidget::argumentsLineEdit_textChanged(const QString &arg1)
//{
//  m_rosLaunchStep->setLaunchFileArguments(arg1);
//  emit updateSummary();
//}

ROSRunStep::ROSRunStep(RunStepList *rsl) :
    ROSRunStep(rsl, Core::Id(ROS_RUN_ID))
{
}

ROSRunStep::ROSRunStep(RunStepList *rsl, Core::Id id) : ROSGenericRunStep(rsl, id)
{
}

ROSRunStep::ROSRunStep(RunStepList *rsl, RunStep *rs) : ROSGenericRunStep(rsl, rs)
{
}

ROSLaunchStep::ROSLaunchStep(RunStepList *rsl) :
    ROSLaunchStep(rsl, Core::Id(ROS_LAUNCH_ID))
{
}

ROSLaunchStep::ROSLaunchStep(RunStepList *rsl, Core::Id id) : ROSGenericRunStep(rsl, id)
{
}

ROSLaunchStep::ROSLaunchStep(RunStepList *rsl, RunStep *rs) : ROSGenericRunStep(rsl, rs)
{
}

ROSGenericRunStep::ROSGenericRunStep(RunStepList *rsl, Core::Id id) : RunStep(rsl, id)
{
  if(id == ROS_LAUNCH_ID)
  {
    m_command = QLatin1String("roslaunch");
  }
  else if (id == ROS_RUN_ID)
  {
    m_command = QLatin1String("rosrun");
  }
  else
  {
    // need to throw error
  }
}

ROSGenericRunStep::ROSGenericRunStep(RunStepList *rsl, RunStep *rs) : RunStep(rsl, rs)
{
}

ROSGenericRunStep::~ROSGenericRunStep()
{
}

bool ROSGenericRunStep::init(QList<const RunStep *> &earlierSteps)
{
    Q_UNUSED(earlierSteps);

    ROSRunConfiguration *rc = rosRunConfiguration();
    if (!rc)
    {
      rc = targetsActiveRunConfiguration();

      if (!rc)
      {
          return false;
      }
    }

    return true;
}
void ROSGenericRunStep::run(QFutureInterface<bool> &fi)
{
  Q_UNUSED(fi);

  ROSProject *rp = qobject_cast<ROSProject *>(target()->project());

  QString command;
  command = QString::fromLatin1("%1 %2 %3 %4\n")
      .arg(m_command,
           m_package,
           m_target,
           m_arguments);

  ROSTerminalPane *terminal = ROSManager::instance()->getTerminalPane();

  terminal->createTerminal(QString::fromLatin1("%1 %2 %3").arg(m_command, m_package, m_target), QDir(rp->projectDirectory().toString()));

  // source workspace (This is a hack because the setEnvironment is not working as I expected)
  terminal->sendInput(QLatin1String("source devel/setup.bash\n"));

  //send roslaunch command
  terminal->sendInput(command);
}

QVariantMap ROSGenericRunStep::toMap() const
{
    QVariantMap map(RunStep::toMap());

    map.insert(QLatin1String(ROS_GENERIC_COMMAND_KEY), m_command);
    map.insert(QLatin1String(ROS_GENERIC_PACKAGE_KEY), m_package);
    map.insert(QLatin1String(ROS_GENERIC_TARGET_KEY), m_target);
    map.insert(QLatin1String(ROS_GENERIC_ARGUMENTS_KEY), m_arguments);
    return map;
}

bool ROSGenericRunStep::fromMap(const QVariantMap &map)
{
    m_command = map.value(QLatin1String(ROS_GENERIC_COMMAND_KEY)).toString();
    m_package = map.value(QLatin1String(ROS_GENERIC_PACKAGE_KEY)).toString();
    m_target = map.value(QLatin1String(ROS_GENERIC_TARGET_KEY)).toString();
    m_arguments = map.value(QLatin1String(ROS_GENERIC_ARGUMENTS_KEY)).toString();

    return RunStep::fromMap(map);
}

ROSRunConfiguration *ROSGenericRunStep::rosRunConfiguration() const
{
    return static_cast<ROSRunConfiguration *>(runConfiguration());
}

ROSRunConfiguration *ROSGenericRunStep::targetsActiveRunConfiguration() const
{
    return static_cast<ROSRunConfiguration *>(target()->activeRunConfiguration());
}

RunStepConfigWidget *ROSGenericRunStep::createConfigWidget()
{
    return new ROSGenericRunStepConfigWidget(this);
}

QString ROSGenericRunStep::getCommand() const
{
  return m_command;
}

QString ROSGenericRunStep::getPackage() const
{
  return m_package;
}

QString ROSGenericRunStep::getTarget() const
{
  return m_target;
}

QString ROSGenericRunStep::getArguments() const
{
  return m_arguments;
}

void ROSGenericRunStep::setPackage(const QString &package)
{
  m_package = package;
}

void ROSGenericRunStep::setTarget(const QString &target)
{
  m_target = target;
}

void ROSGenericRunStep::setArguments(const QString &arguments)
{
  m_arguments = arguments;
}


//
// ROSLaunchStepConfigWidget
//

ROSGenericRunStepConfigWidget::ROSGenericRunStepConfigWidget(ROSGenericRunStep *genericStep)
    : m_rosGenericStep(genericStep)
{
    ROSBuildConfiguration *bc = qobject_cast<ROSBuildConfiguration *>(genericStep->target()->activeBuildConfiguration());
    m_availablePackages = ROSUtils::getROSPackages(bc->environment().toStringList());

    m_ui = new Ui::ROSGenericStep();
    m_ui->setupUi(this);
    m_ui->packageComboBox->setStyleSheet(tr("combobox-popup: 0;"));
    m_ui->targetComboBox->setStyleSheet(tr("combobox-popup: 0;"));
    m_ui->packageComboBox->addItems(m_availablePackages.keys());

    int idx;
    idx = m_ui->packageComboBox->findText(genericStep->getPackage(), Qt::MatchExactly);
    m_ui->packageComboBox->setCurrentIndex(idx);

    m_ui->targetComboBox->addItems(getAvailableTargets());
    idx = m_ui->targetComboBox->findText(genericStep->getTarget(), Qt::MatchExactly);
    m_ui->targetComboBox->setCurrentIndex(idx);

    m_ui->argumentsLineEdit->setText(genericStep->getArguments());

    connect(m_ui->packageComboBox, SIGNAL(currentIndexChanged(QString)),
            this, SLOT(packageComboBox_currentIndexChanged(QString)));

    connect(m_ui->targetComboBox, SIGNAL(currentIndexChanged(QString)),
            this, SLOT(targetComboBox_currentIndexChanged(QString)));

    connect(m_ui->argumentsLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(argumentsLineEdit_textChanged(QString)));
}

ROSGenericRunStepConfigWidget::~ROSGenericRunStepConfigWidget()
{
    delete m_ui;
}

QString ROSGenericRunStepConfigWidget::displayName() const
{
    return tr("ROS Step", "ROS step display name.");
}

QString ROSGenericRunStepConfigWidget::summaryText() const
{
  //this is causing a the plugin to crash
  return QString::fromLatin1("<b>%1:</b> %2 %3 %4 %5")
          .arg(displayName(),
               m_rosGenericStep->getCommand(),
               m_rosGenericStep->getPackage(),
               m_rosGenericStep->getTarget(),
               m_rosGenericStep->getArguments());
}

void ROSGenericRunStepConfigWidget::packageComboBox_currentIndexChanged(const QString &arg1)
{
  m_rosGenericStep->setPackage(arg1);
  m_ui->targetComboBox->addItems(getAvailableTargets());
  emit updateSummary();
}

void ROSGenericRunStepConfigWidget::targetComboBox_currentIndexChanged(const QString &arg1)
{
  m_rosGenericStep->setTarget(arg1);
  emit updateSummary();
}

void ROSGenericRunStepConfigWidget::argumentsLineEdit_textChanged(const QString &arg1)
{
  m_rosGenericStep->setArguments(arg1);
  emit updateSummary();
}

QStringList ROSGenericRunStepConfigWidget::getAvailableTargets()
{
    if(m_rosGenericStep->id() == ROS_LAUNCH_ID)
    {
      return ROSUtils::getROSPackageLaunchFiles(m_availablePackages[m_rosGenericStep->getPackage()]);
    }
    else if (m_rosGenericStep->id() == ROS_RUN_ID)
    {
      ROSBuildConfiguration *bc = qobject_cast<ROSBuildConfiguration *>(m_rosGenericStep->target()->activeBuildConfiguration());
      return ROSUtils::getROSPackageExecutables(m_rosGenericStep->getPackage(),bc->environment().toStringList());
    }

    return QStringList();
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
    {
      if (id == ROS_LAUNCH_ID || id == ROS_RUN_ID)
      {
        return true;
      }
    }
    return false;
}

RunStep *ROSRunStepFactory::create(RunStepList *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;

    if (id == ROS_LAUNCH_ID)
    {
      return new ROSLaunchStep(parent, id);
    }
    else if (id == ROS_RUN_ID)
    {
      return new ROSRunStep(parent, id);
    }

    return 0;
}

bool ROSRunStepFactory::canClone(RunStepList *parent, RunStep *source) const
{
    return canCreate(parent, source->id());
}

RunStep *ROSRunStepFactory::clone(RunStepList *parent, RunStep *source)
{
    if (!canClone(parent, source))
        return 0;

    if (source->id() == ROS_LAUNCH_ID)
    {
      ROSLaunchStep *old(qobject_cast<ROSLaunchStep *>(source));
      Q_ASSERT(old);
      return new ROSLaunchStep(parent, old);
    }
    else if (source->id() == ROS_RUN_ID)
    {
      ROSRunStep *old(qobject_cast<ROSRunStep *>(source));
      Q_ASSERT(old);
      return new ROSRunStep(parent, old);
    }
    else
    {
      return 0;
    }
}

bool ROSRunStepFactory::canRestore(RunStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

RunStep *ROSRunStepFactory::restore(RunStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    if (ProjectExplorer::idFromMap(map) == ROS_LAUNCH_ID)
    {
      ROSLaunchStep *rs(new ROSLaunchStep(parent, ProjectExplorer::idFromMap(map)));
      if (rs->fromMap(map))
          return rs;
      delete rs;
      return 0;
    }
    else if (ProjectExplorer::idFromMap(map) == ROS_RUN_ID)
    {
      ROSRunStep *rs(new ROSRunStep(parent, ProjectExplorer::idFromMap(map)));
      if (rs->fromMap(map))
          return rs;
      delete rs;
      return 0;
    }

    return 0;
}

QList<Core::Id> ROSRunStepFactory::availableCreationIds(RunStepList *parent) const
{
    if (parent->target()->project()->id() == Constants::ROSPROJECT_ID)
        return QList<Core::Id>() << Core::Id(ROS_LAUNCH_ID) << Core::Id(ROS_RUN_ID);
    return QList<Core::Id>();
}

QString ROSRunStepFactory::displayNameForId(const Core::Id id) const
{
    if (id == ROS_LAUNCH_ID)
        return tr("ROS Launch Step");
    else if (id== ROS_RUN_ID)
        return tr("ROS Run Step");

    return QString();
}


} // namespace Internal
} // namespace ROSProjectManager
