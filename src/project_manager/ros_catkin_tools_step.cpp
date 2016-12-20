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
#include "ros_catkin_tools_step.h"
#include "ros_project_constants.h"
#include "ros_project.h"
#include "ui_ros_catkin_tools_step.h"
#include "ui_ros_catkin_tools_config_editor.h"

#include <boost/algorithm/string/join.hpp>
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
#include <fstream>

#include <QDir>
#include <QInputDialog>

using namespace Core;
using namespace ProjectExplorer;

namespace ROSProjectManager {
namespace Internal {

const char ROS_CTS_ID[] = "ROSProjectManager.ROSCatkinToolsStep";
const char ROS_CTS_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("ROSProjectManager::Internal::ROSCatkinToolsStep",
                                                     "CatkinTools Step");
const char ROS_CTS_TARGET_KEY[] = "ROSProjectManager.ROSCatkinToolsStep.Target";
const char ROS_CTS_ACTIVE_PROFILE_KEY[] = "ROSProjectManager.ROSCatkinToolsStep.ActiveProfile";
const char ROS_CTS_CATKIN_TOOLS_ARGUMENTS_KEY[] = "ROSProjectManager.ROSCatkinToolsStep.CatkinToolsArguments";
const char ROS_CTS_CATKIN_MAKE_ARGUMENTS_KEY[] = "ROSProjectManager.ROSCatkinToolsStep.CatkinMakeArguments";
const char ROS_CTS_CMAKE_ARGUMENTS_KEY[] = "ROSProjectManager.ROSCatkinToolsStep.CMakeArguments";
const char ROS_CTS_MAKE_ARGUMENTS_KEY[] = "ROSProjectManager.ROSCatkinToolsStep.MakeArguments";

ROSCatkinToolsStep::ROSCatkinToolsStep(BuildStepList *parent) :
    AbstractProcessStep(parent, Id(ROS_CTS_ID))
{
   ctor();
}

ROSCatkinToolsStep::ROSCatkinToolsStep(BuildStepList *parent, const Id id) :
    AbstractProcessStep(parent, id)
{
    ctor();
}

ROSCatkinToolsStep::ROSCatkinToolsStep(BuildStepList *parent, ROSCatkinToolsStep *bs) :
    AbstractProcessStep(parent, bs),
    m_target(bs->m_target),
    m_activeProfile(bs->m_activeProfile),
    m_catkinToolsArguments(bs->m_catkinToolsArguments),
    m_catkinMakeArguments(bs->m_catkinMakeArguments),
    m_cmakeArguments(bs->m_cmakeArguments),
    m_makeArguments(bs->m_makeArguments)
{
    ctor();
}

void ROSCatkinToolsStep::ctor()
{
    setDefaultDisplayName(QCoreApplication::translate("ROSProjectManager::Internal::ROSCatkinToolsStep",
                                                      ROS_CTS_DISPLAY_NAME));

    if (m_activeProfile.isEmpty())
        m_activeProfile = "default";

    m_percentProgress = QRegExp(QLatin1String("\\[\\s{0,2}(\\d{1,3})%\\]")); // Example: [ 82%] [ 82%] [ 87%]

    ROSBuildConfiguration *bc = rosBuildConfiguration();
    if (bc->buildSystem() != ROSUtils::CatkinTools)
        setEnabled(false);
}

ROSBuildConfiguration *ROSCatkinToolsStep::rosBuildConfiguration() const
{
    return static_cast<ROSBuildConfiguration *>(buildConfiguration());
}

ROSBuildConfiguration *ROSCatkinToolsStep::targetsActiveBuildConfiguration() const
{
    return static_cast<ROSBuildConfiguration *>(target()->activeBuildConfiguration());
}

ROSCatkinToolsStep::~ROSCatkinToolsStep()
{
}

bool ROSCatkinToolsStep::init(QList<const BuildStep *> &earlierSteps)
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

    // Set Catkin Tools Active Profile
    ROSUtils::setCatkinToolsActiveProfile(bc->project()->projectDirectory(), activeProfile());

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setWorkingDirectory(ROSUtils::getWorkspaceBuildSpace(bc->project()->projectDirectory(), bc->buildSystem()).toString());
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

QVariantMap ROSCatkinToolsStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());

    map.insert(QLatin1String(ROS_CTS_TARGET_KEY), m_target);
    map.insert(QLatin1String(ROS_CTS_ACTIVE_PROFILE_KEY), m_activeProfile);
    map.insert(QLatin1String(ROS_CTS_CATKIN_TOOLS_ARGUMENTS_KEY), m_catkinToolsArguments);
    map.insert(QLatin1String(ROS_CTS_CATKIN_MAKE_ARGUMENTS_KEY), m_catkinMakeArguments);
    map.insert(QLatin1String(ROS_CTS_CMAKE_ARGUMENTS_KEY), m_cmakeArguments);
    map.insert(QLatin1String(ROS_CTS_MAKE_ARGUMENTS_KEY), m_makeArguments);
    return map;
}

bool ROSCatkinToolsStep::fromMap(const QVariantMap &map)
{
    m_target = (BuildTargets)map.value(QLatin1String(ROS_CTS_TARGET_KEY)).toInt();
    m_activeProfile = map.value(QLatin1String(ROS_CTS_ACTIVE_PROFILE_KEY)).toString();
    m_catkinToolsArguments = map.value(QLatin1String(ROS_CTS_CATKIN_TOOLS_ARGUMENTS_KEY)).toString();
    m_catkinMakeArguments = map.value(QLatin1String(ROS_CTS_CATKIN_MAKE_ARGUMENTS_KEY)).toString();
    m_cmakeArguments = map.value(QLatin1String(ROS_CTS_CMAKE_ARGUMENTS_KEY)).toString();
    m_makeArguments = map.value(QLatin1String(ROS_CTS_MAKE_ARGUMENTS_KEY)).toString();
    return BuildStep::fromMap(map);
}

QString ROSCatkinToolsStep::allArguments(ROSUtils::BuildType buildType) const
{
    QString args;

    switch(m_target) {
    case BUILD:
        Utils::QtcProcess::addArgs(&args, QLatin1String("build"));
        Utils::QtcProcess::addArgs(&args, m_catkinToolsArguments);

        if (!m_catkinMakeArguments.isEmpty())
            Utils::QtcProcess::addArgs(&args, QString("--catkin-make-args %1").arg(m_catkinMakeArguments));

        Utils::QtcProcess::addArgs(&args, QString("--cmake-args -G \"CodeBlocks - Unix Makefiles\" %1").arg(ROSUtils::getCMakeBuildTypeArgument(buildType)));
        if (!m_cmakeArguments.isEmpty())
            Utils::QtcProcess::addArgs(&args, m_cmakeArguments);
        break;
    case CLEAN:
        Utils::QtcProcess::addArgs(&args, QLatin1String("clean"));
        Utils::QtcProcess::addArgs(&args, QLatin1String("-y"));
        Utils::QtcProcess::addArgs(&args, m_catkinToolsArguments);

        if (!m_catkinMakeArguments.isEmpty())
            Utils::QtcProcess::addArgs(&args, QString("--catkin-make-args %1").arg(m_catkinMakeArguments));

        if (!m_cmakeArguments.isEmpty())
            Utils::QtcProcess::addArgs(&args, QString("--cmake-args %1").arg(m_cmakeArguments));
        break;
    }

    if (!m_makeArguments.isEmpty())
        Utils::QtcProcess::addArgs(&args, QString("--make-args %1").arg(m_makeArguments));

    return args;
}

QString ROSCatkinToolsStep::makeCommand() const
{
    return QLatin1String("catkin");
}

void ROSCatkinToolsStep::run(QFutureInterface<bool> &fi)
{
    AbstractProcessStep::run(fi);
}

void ROSCatkinToolsStep::processStarted()
{
    futureInterface()->setProgressRange(0, 100);
    AbstractProcessStep::processStarted();
}

void ROSCatkinToolsStep::processFinished(int exitCode, QProcess::ExitStatus status)
{
    AbstractProcessStep::processFinished(exitCode, status);
    futureInterface()->setProgressValue(100);
}

void ROSCatkinToolsStep::stdOutput(const QString &line)
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

BuildStepConfigWidget *ROSCatkinToolsStep::createConfigWidget()
{
    return new ROSCatkinToolsStepWidget(this);
}

bool ROSCatkinToolsStep::immutable() const
{
    return false;
}

ROSCatkinToolsStep::BuildTargets ROSCatkinToolsStep::buildTarget() const
{
    return m_target;
}

void ROSCatkinToolsStep::setBuildTarget(const BuildTargets &target)
{
    m_target = target;
}

QString ROSCatkinToolsStep::activeProfile() const
{
    return m_activeProfile;
}

void ROSCatkinToolsStep::setActiveProfile(const QString &profileName)
{
    m_activeProfile = profileName;
}

//
// ROSCatkinToolsStepWidget
//

ROSCatkinToolsStepWidget::ROSCatkinToolsStepWidget(ROSCatkinToolsStep *makeStep)
    : m_makeStep(makeStep)
{
    m_ui = new Ui::ROSCatkinToolsStep;
    m_ui->setupUi(this);

    m_ui->catkinToolsArgumentsLineEdit->setText(m_makeStep->m_catkinToolsArguments);
    m_ui->catkinMakeArgumentsLineEdit->setText(m_makeStep->m_catkinMakeArguments);
    m_ui->cmakeArgumentsLineEdit->setText(m_makeStep->m_cmakeArguments);
    m_ui->makeArgumentsLineEdit->setText(m_makeStep->m_makeArguments);
    setProfile(m_makeStep->m_activeProfile);

    m_addButtonMenu = new QMenu(this);
    m_ui->addPushButton->setMenu(m_addButtonMenu);

    QAction *cloneAction = m_addButtonMenu->addAction(tr("&Clone Selected"));
    connect(cloneAction, &QAction::triggered,
            this, [this]() { cloneProfile(m_makeStep->activeProfile()); });

    QAction *newAction = m_addButtonMenu->addAction(tr("&New Profile"));
    connect(newAction, &QAction::triggered,
            this, &ROSCatkinToolsStepWidget::newProfile);

    m_profileMenu = new QMenu(this);
    m_ui->profilePushButton->setMenu(m_profileMenu);

    connect(m_profileMenu, &QMenu::aboutToShow,
            this, &ROSCatkinToolsStepWidget::updateProfileButtonMenu);

    updateDetails();

    connect(m_ui->removePushButton, &QAbstractButton::clicked,
            this, [this]() { removeProfile(m_makeStep->activeProfile()); });

    connect(m_ui->renamePushButton, &QAbstractButton::clicked,
            this, [this]() { renameProfile(m_makeStep->activeProfile()); });

    connect(m_ui->editPushButton, &QAbstractButton::clicked,
            this, [this]() { editProfile(m_makeStep->activeProfile()); });

    connect(m_ui->catkinToolsArgumentsLineEdit, &QLineEdit::textEdited,
            this, &ROSCatkinToolsStepWidget::updateDetails);

    connect(m_ui->catkinMakeArgumentsLineEdit, &QLineEdit::textEdited,
            this, &ROSCatkinToolsStepWidget::updateDetails);

    connect(m_ui->cmakeArgumentsLineEdit, &QLineEdit::textEdited,
            this, &ROSCatkinToolsStepWidget::updateDetails);

    connect(m_ui->makeArgumentsLineEdit, &QLineEdit::textEdited,
            this, &ROSCatkinToolsStepWidget::updateDetails);

    connect(m_makeStep, SIGNAL(enabledChanged()),
            this, SLOT(enabledChanged()));

    ROSBuildConfiguration *bc = m_makeStep->rosBuildConfiguration();
    connect(bc, SIGNAL(buildSystemChanged(ROSUtils::BuildSystem)),
            this, SLOT(updateBuildSystem(ROSUtils::BuildSystem)));

    connect(bc, &ROSBuildConfiguration::cmakeBuildTypeChanged,
            this, &ROSCatkinToolsStepWidget::updateDetails);

    ROSProject *pro = static_cast<ROSProject *>(m_makeStep->target()->project());
    connect(pro, &ROSProject::environmentChanged,
            this, &ROSCatkinToolsStepWidget::updateDetails);

    connect(ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateDetails()));
}

ROSCatkinToolsStepWidget::~ROSCatkinToolsStepWidget()
{
    delete m_ui;
}

QString ROSCatkinToolsStepWidget::displayName() const
{
    return tr("CatkinTools", "Catkin Tools display name.");
}

void ROSCatkinToolsStepWidget::updateDetails()
{
    m_makeStep->m_catkinToolsArguments = m_ui->catkinToolsArgumentsLineEdit->text();
    m_makeStep->m_catkinMakeArguments = m_ui->catkinMakeArgumentsLineEdit->text();
    m_makeStep->m_cmakeArguments = m_ui->cmakeArgumentsLineEdit->text();
    m_makeStep->m_makeArguments = m_ui->makeArgumentsLineEdit->text();

    ROSBuildConfiguration *bc = m_makeStep->rosBuildConfiguration();
    ProcessParameters param;
    param.setCommand(m_makeStep->makeCommand());
    param.setArguments(m_makeStep->allArguments(bc->cmakeBuildType()));
    m_summaryText = param.summary(displayName());
    emit updateSummary();
}

void ROSCatkinToolsStepWidget::updateBuildSystem(const ROSUtils::BuildSystem &buildSystem)
{
    m_makeStep->setEnabled((buildSystem == ROSUtils::CatkinTools));
}

void ROSCatkinToolsStepWidget::enabledChanged()
{
    ROSBuildConfiguration *bc = m_makeStep->rosBuildConfiguration();
    if(m_makeStep->enabled() && (bc->buildSystem() != ROSUtils::CatkinTools))
        m_makeStep->setEnabled(false);
}

QString ROSCatkinToolsStepWidget::summaryText() const
{
    return m_summaryText;
}

void ROSCatkinToolsStepWidget::updateProfileButtonMenu()
{
    m_profileMenu->clear();

    QStringList profileNames = ROSUtils::getCatkinToolsProfileNames(m_makeStep->rosBuildConfiguration()->project()->projectDirectory());
    foreach( QString profile, profileNames)
    {
        QAction *action = m_profileMenu->addAction(profile);
        connect(action, &QAction::triggered, this, [this, profile] { setProfile(profile); });
    }
}

void ROSCatkinToolsStepWidget::setProfile(const QString profileName)
{
    m_makeStep->setActiveProfile(profileName);
    m_ui->profilePushButton->setText(QString(" %1").arg(profileName));
}

void ROSCatkinToolsStepWidget::cloneProfile(const QString profileName)
{
    QString name = uniqueName(profileName, false);
    ROSUtils::cloneCatkinToolsProfile(m_makeStep->rosBuildConfiguration()->project()->projectDirectory(), profileName, name);
    setProfile(name);
}

void ROSCatkinToolsStepWidget::newProfile()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("New Profile..."),
                                         tr("Pofile Name:"),
                                         QLineEdit::Normal,
                                         QLatin1String("default"), &ok);
    if (!ok)
        return;

    name = uniqueName(name, false);
    if (name.isEmpty())
        return;

    if (!ROSUtils::createCatkinToolsProfile(m_makeStep->rosBuildConfiguration()->project()->projectDirectory(), name))
        return;

    setProfile(name);
}

void ROSCatkinToolsStepWidget::renameProfile(const QString profileName)
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("Rename..."),
                                         tr("New name for profile <b>%1</b>:").
                                            arg(profileName),
                                         QLineEdit::Normal,
                                         profileName, &ok);
    if (!ok)
        return;

    name = uniqueName(name, true);
    if (name.isEmpty())
        return;

    if (!ROSUtils::renameCatkinToolsProfile(m_makeStep->rosBuildConfiguration()->project()->projectDirectory(), profileName, name))
        return;

    setProfile(name);
}

void ROSCatkinToolsStepWidget::editProfile(const QString profileName)
{
    Utils::FileName profile = ROSUtils::getCatkinToolsProfile(m_makeStep->rosBuildConfiguration()->project()->projectDirectory(), profileName);

    ROSCatkinToolsProfileEditorDialog *editor = new ROSCatkinToolsProfileEditorDialog(profile);
    editor->show();
}

void ROSCatkinToolsStepWidget::removeProfile(const QString profileName)
{
    ROSUtils::removeCatkinToolsProfile(m_makeStep->rosBuildConfiguration()->project()->projectDirectory(), profileName);
    setProfile(ROSUtils::getCatkinToolsProfileNames(m_makeStep->rosBuildConfiguration()->project()->projectDirectory())[0]);
}

QString ROSCatkinToolsStepWidget::uniqueName(const QString &name, const bool &isRename)
{
    QString result = name.trimmed();
    QStringList profileNames = ROSUtils::getCatkinToolsProfileNames(m_makeStep->rosBuildConfiguration()->project()->projectDirectory());
    if (!result.isEmpty()) {
        if(isRename)
        {
            QStringList pNames;
            foreach (QString profile, profileNames) {
                if (profile == m_makeStep->activeProfile())
                    continue;
                pNames.append(profile);
            }
            return Project::makeUnique(result, pNames);
        }

        return Project::makeUnique(result, profileNames);
    }

    return result;
}

//
// ROSCatkinToolsConfigEditorWidget
//

ROSCatkinToolsConfigEditorWidget::ROSCatkinToolsConfigEditorWidget() : QWidget()
{
    m_ui = new Ui::ROSCatkinToolsConfigEditor;
    m_ui->setupUi(this);
    m_modified = false;

    QAction *editor;
    editor = m_ui->extend_paths_lineEdit->addAction(QIcon(QLatin1String(":rosproject/pencil_icon.png")), QLineEdit::TrailingPosition);
    connect(editor, SIGNAL(triggered(bool)), this, SLOT(onActionEditFilePathListTriggered()));

    editor = m_ui->whitelist_lineEdit->addAction(QIcon(QLatin1String(":rosproject/pencil_icon.png")), QLineEdit::TrailingPosition);
    connect(editor, SIGNAL(triggered(bool)), this, SLOT(onActionEditPackageListTriggered()));

    editor = m_ui->blacklist_lineEdit->addAction(QIcon(QLatin1String(":rosproject/pencil_icon.png")), QLineEdit::TrailingPosition);
    connect(editor, SIGNAL(triggered(bool)), this, SLOT(onActionEditPackageListTriggered()));

    editor = m_ui->cmake_args_lineEdit->addAction(QIcon(QLatin1String(":rosproject/pencil_icon.png")), QLineEdit::TrailingPosition);
    connect(editor, SIGNAL(triggered(bool)), this, SLOT(onActionEditStingListTriggered()));

    editor = m_ui->make_args_lineEdit->addAction(QIcon(QLatin1String(":rosproject/pencil_icon.png")), QLineEdit::TrailingPosition);
    connect(editor, SIGNAL(triggered(bool)), this, SLOT(onActionEditStingListTriggered()));

    editor = m_ui->catkin_args_lineEdit->addAction(QIcon(QLatin1String(":rosproject/pencil_icon.png")), QLineEdit::TrailingPosition);
    connect(editor, SIGNAL(triggered(bool)), this, SLOT(onActionEditStingListTriggered()));
}

ROSCatkinToolsConfigEditorWidget::~ROSCatkinToolsConfigEditorWidget()
{
    delete m_ui;
}

bool ROSCatkinToolsConfigEditorWidget::parseProfileConfig(Utils::FileName filePath)
{
    m_profileConfigPath = filePath;
    if (!m_profileConfigPath.exists())
    {
        qWarning() << QString("Catkin Tools Profile Config File: %1, does not exist.").arg(m_profileConfigPath.toString());
        return false;
    }
    m_profile_original = YAML::LoadFile(m_profileConfigPath.toString().toStdString());
    m_profile_current = m_profile_original;
    m_modified = false;

    // both work for getting list of string (preferece to use vector)
    m_ui->blacklist_lineEdit->setText(convertListToString("blacklist"));
    m_ui->space_build_lineEdit->setText(parseString("build_space"));
    m_ui->catkin_args_lineEdit->setText(convertListToString("catkin_make_args"));
    m_ui->cmake_args_lineEdit->setText(convertListToString("cmake_args"));
    m_ui->space_devel_layout_comboBox->setCurrentText(parseString("devel_layout"));
    m_ui->space_devel_lineEdit->setText(parseString("devel_space"));
    m_ui->extend_paths_lineEdit->setText(convertListToString("extended_path"));
    m_ui->space_install_option_comboBox->setCurrentIndex(parseBool("install"));
    m_ui->space_install_lineEdit->setText(parseString("install_space"));
    m_ui->space_install_layout_comboBox->setCurrentIndex(parseBool("isolate_install"));
    //TODO: Need to figure out how to parse jobs and package jobs
    m_ui->space_log_lineEdit->setText(parseString("log_space"));
    m_ui->make_args_lineEdit->setText(convertListToString("make_args"));
    m_ui->space_source_lineEdit->setText(parseString("source_space"));
    m_ui->env_cache_checkBox->setChecked(parseBool("use_env_cache"));
    m_ui->jobserver_checkBox->setChecked(parseBool("use_internal_make_jobserver"));
    m_ui->whitelist_lineEdit->setText(convertListToString("whitelist"));
    return true;
}

std::vector<std::string> ROSCatkinToolsConfigEditorWidget::parseList(std::string key)
{
    if (m_profile_current[key].Type() != YAML::NodeType::Sequence)
        return std::vector<std::string>();

    return m_profile_current[key].as<std::vector<std::string> >();
}

QString ROSCatkinToolsConfigEditorWidget::parseString(std::string key)
{
    if (m_profile_current[key].Type() != YAML::NodeType::Scalar)
        return QString();

    return QString::fromStdString(m_profile_current[key].as<std::string>());
}

bool ROSCatkinToolsConfigEditorWidget::parseBool(std::string key)
{
    if (m_profile_current[key].Type() != YAML::NodeType::Scalar)
        return false;

    return m_profile_current[key].as<bool>();
}

QString ROSCatkinToolsConfigEditorWidget::convertListToString(std::string key)
{
    return convertListToString(parseList(key));
}

QString ROSCatkinToolsConfigEditorWidget::convertListToString(std::vector<std::string> list)
{
    return "[" + QString::fromStdString(boost::algorithm::join(list, "; ")) + "]";
}

bool ROSCatkinToolsConfigEditorWidget::saveProfileConfig()
{
    if (!m_profileConfigPath.exists())
    {
        qWarning() << QString("Catkin Tools Profile Config File: %1, does not exist.").arg(m_profileConfigPath.toString());
        return false;
    }
    std::ofstream fout(m_profileConfigPath.toString().toStdString());
    fout << m_profile_current; // dump it back into the file

    m_profile_original = m_profile_current;
    m_modified = false;
    return true;
}

bool ROSCatkinToolsConfigEditorWidget::isModified() const
{
    return m_modified;
}

void ROSCatkinToolsConfigEditorWidget::onActionEditFilePathListTriggered()
{
    qDebug() << m_editor->parent()->objectName();
}

void ROSCatkinToolsConfigEditorWidget::onActionEditPackageListTriggered()
{

    qDebug() << m_editor->parent()->objectName();
}

void ROSCatkinToolsConfigEditorWidget::onActionEditStingListTriggered()
{

    qDebug() << m_editor->parent()->objectName();
}



//
// ROSCatkinToolsProfileEditorDialog
//

ROSCatkinToolsProfileEditorDialog::ROSCatkinToolsProfileEditorDialog(Utils::FileName filePath) : QDialog()
{
    QVBoxLayout *vlayout = new QVBoxLayout();
    ROSCatkinToolsConfigEditorWidget *editorWidget = new ROSCatkinToolsConfigEditorWidget();
    vlayout->addWidget(editorWidget);
    setLayout(vlayout);
    setWindowTitle(QLatin1String("Catkin Tools Configuration Editor"));
    if(!editorWidget->parseProfileConfig(filePath))
        this->close();
}

//
// ROSCatkinToolsStepFactory
//

ROSCatkinToolsStepFactory::ROSCatkinToolsStepFactory(QObject *parent) :
    IBuildStepFactory(parent)
{
}

BuildStep *ROSCatkinToolsStepFactory::create(BuildStepList *parent, const Id id)
{
    Q_UNUSED(id);
    ROSCatkinToolsStep *step = new ROSCatkinToolsStep(parent);
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN) {
        step->setBuildTarget(ROSCatkinToolsStep::CLEAN);
    } else if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_BUILD) {
        step->setBuildTarget(ROSCatkinToolsStep::BUILD);
    }
    return step;
}

BuildStep *ROSCatkinToolsStepFactory::clone(BuildStepList *parent, BuildStep *source)
{
    return new ROSCatkinToolsStep(parent, qobject_cast<ROSCatkinToolsStep *>(source));
}

BuildStep *ROSCatkinToolsStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    ROSCatkinToolsStep *bs(new ROSCatkinToolsStep(parent));
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QList<ProjectExplorer::BuildStepInfo> ROSCatkinToolsStepFactory::availableSteps(BuildStepList *parent) const
{
    if (parent->target()->project()->id() != Constants::ROSPROJECT_ID)
        return {};

    return {{ROS_CTS_ID,  QCoreApplication::translate("ROSProjectManager::Internal::ROSCatkinToolsConfigStep", ROS_CTS_DISPLAY_NAME)}};
}

} // namespace Internal
} // namespace ROSProjectManager
