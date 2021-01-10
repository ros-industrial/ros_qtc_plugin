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
#include "ui_ros_catkin_tools_list_editor.h"
#include "ui_ros_catkin_tools_config_editor.h"

#include <boost/algorithm/string/join.hpp>
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
#include <coreplugin/variablechooser.h>
#include <coreplugin/messagemanager.h>

#include <fstream>
#include <QDir>
#include <QInputDialog>

using namespace ProjectExplorer;

namespace ROSProjectManager {
namespace Internal {

const char ROS_CTS_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("ROSProjectManager::Internal::ROSCatkinToolsStep",
                                                     "CatkinTools Step");
const char ROS_CTS_TARGET_KEY[] = "ROSProjectManager.ROSCatkinToolsStep.Target";
const char ROS_CTS_ACTIVE_PROFILE_KEY[] = "ROSProjectManager.ROSCatkinToolsStep.ActiveProfile";
const char ROS_CTS_CATKIN_TOOLS_ARGUMENTS_KEY[] = "ROSProjectManager.ROSCatkinToolsStep.CatkinToolsArguments";
const char ROS_CTS_CATKIN_TOOLS_WORKING_DIR_KEY[] = "ROSProjectManager.ROSCatkinToolsStep.CatkinToolsWorkingDir";
const char ROS_CTS_CATKIN_MAKE_ARGUMENTS_KEY[] = "ROSProjectManager.ROSCatkinToolsStep.CatkinMakeArguments";
const char ROS_CTS_CMAKE_ARGUMENTS_KEY[] = "ROSProjectManager.ROSCatkinToolsStep.CMakeArguments";
const char ROS_CTS_MAKE_ARGUMENTS_KEY[] = "ROSProjectManager.ROSCatkinToolsStep.MakeArguments";

ROSCatkinToolsStep::ROSCatkinToolsStep(BuildStepList *parent, const Utils::Id id) :
    AbstractProcessStep(parent, id)
{
    m_catkinToolsWorkingDir = Constants::ROS_DEFAULT_WORKING_DIR;

    setDefaultDisplayName(QCoreApplication::translate("ROSProjectManager::Internal::ROSCatkinToolsStep",
                                                      ROS_CTS_DISPLAY_NAME));

    if (m_activeProfile.isEmpty())
        m_activeProfile = "default";

    m_percentProgress = QRegExp(QLatin1String(".+\\[(\\d+)/(\\d+) complete\\]")); // Example: [0/24 complete]

    ROSBuildConfiguration *bc = rosBuildConfiguration();
    if (bc->rosBuildSystem() != ROSUtils::CatkinTools)
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

bool ROSCatkinToolsStep::init()
{
    ROSBuildConfiguration *bc = rosBuildConfiguration();
    if (!bc)
        bc = targetsActiveBuildConfiguration();
    if (!bc)
        emit addTask(Task::buildConfigurationMissingTask());

    ToolChain *tc = ToolChainKitAspect::toolChain(target()->kit(), ProjectExplorer::Constants::CXX_LANGUAGE_ID);

    if (!tc)
        emit addTask(Task::compilerMissingTask());

    if (!bc || !tc) {
        emitFaultyConfigurationMessage();
        return false;
    }

    // Set Catkin Tools Active Profile
    ROSUtils::setCatkinToolsActiveProfile(bc->project()->projectDirectory(), activeProfile());
    ROSUtils::WorkspaceInfo workspaceInfo = ROSUtils::getWorkspaceInfo(bc->project()->projectDirectory(), bc->rosBuildSystem(), bc->project()->distribution());
    Utils::Environment env(ROSUtils::getWorkspaceEnvironment(workspaceInfo, bc->environment()).toStringList());

    bc->updateQtEnvironment(env); // TODO: Not sure if this is required here

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setWorkingDirectory(Utils::FilePath::fromString(m_catkinToolsWorkingDir));

    // Force output to english for the parsers. Do this here and not in the toolchain's
    // addToEnvironment() to not screw up the users run environment.
    env.set(QLatin1String("LC_ALL"), QLatin1String("C"));
    pp->setEnvironment(env);
    pp->setCommandLine(makeCommand(allArguments(bc->cmakeBuildType())));
    pp->resolveAll();

    // If we are cleaning, then make can fail with an error code, but that doesn't mean
    // we should stop the clean queue
    // That is mostly so that rebuild works on an already clean project
    setIgnoreReturnValue(m_target == CLEAN);

    setOutputParser(new GnuMakeParser());
    setOutputParser(new CMakeProjectManager::CMakeParser());

    IOutputParser *parser = target()->kit()->createOutputParser();
    if (parser)
        appendOutputParser(parser);

    outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());

    return AbstractProcessStep::init();
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
    map.insert(QLatin1String(ROS_CTS_CATKIN_TOOLS_WORKING_DIR_KEY), m_catkinToolsWorkingDir);
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
    m_catkinToolsWorkingDir = map.value(QLatin1String(ROS_CTS_CATKIN_TOOLS_WORKING_DIR_KEY)).toString();

    if (m_catkinToolsWorkingDir.isEmpty())
        m_catkinToolsWorkingDir = Constants::ROS_DEFAULT_WORKING_DIR;

    return BuildStep::fromMap(map);
}

QString ROSCatkinToolsStep::allArguments(ROSUtils::BuildType buildType, bool includeDefault) const
{
    QString args;

    switch(m_target) {
    case BUILD:
        Utils::QtcProcess::addArgs(&args, QLatin1String("build"));
        Utils::QtcProcess::addArgs(&args, m_catkinToolsArguments);

        if (!m_catkinMakeArguments.isEmpty())
            Utils::QtcProcess::addArgs(&args, QString("--catkin-make-args %1").arg(m_catkinMakeArguments));

        if (includeDefault)
            Utils::QtcProcess::addArgs(&args, QString("--cmake-args -G \"CodeBlocks - Unix Makefiles\" %1 %2").arg(ROSUtils::getCMakeBuildTypeArgument(buildType), m_cmakeArguments));
        else
            if (!m_cmakeArguments.isEmpty())
                Utils::QtcProcess::addArgs(&args, QString("--cmake-args %1").arg(m_cmakeArguments));

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

Utils::CommandLine ROSCatkinToolsStep::makeCommand(const QString &args) const
{
    Utils::CommandLine cmd(QLatin1String("catkin"));
    cmd.addArgs(args, Utils::CommandLine::RawType::Raw);
    return cmd;
}

void ROSCatkinToolsStep::stdOutput(const QString &line)
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

BuildStepConfigWidget *ROSCatkinToolsStep::createConfigWidget()
{
    return new ROSCatkinToolsStepWidget(this);
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
    : ProjectExplorer::BuildStepConfigWidget(makeStep)
    , m_makeStep(makeStep)
{
    m_ui = new Ui::ROSCatkinToolsStep;
    m_ui->setupUi(this);

    m_ui->catkinToolsArgumentsLineEdit->setText(m_makeStep->m_catkinToolsArguments);
    m_ui->catkinMakeArgumentsLineEdit->setText(m_makeStep->m_catkinMakeArguments);
    m_ui->cmakeArgumentsLineEdit->setText(m_makeStep->m_cmakeArguments);
    m_ui->makeArgumentsLineEdit->setText(m_makeStep->m_makeArguments);

    m_ui->catkinToolsWorkingDirWidget->setPath(m_makeStep->m_catkinToolsWorkingDir);
    m_ui->catkinToolsWorkingDirWidget->setHistoryCompleter(QLatin1String("Qt.WorkingDir.History"));
    m_ui->catkinToolsWorkingDirWidget->setExpectedKind(Utils::PathChooser::Directory);
    m_ui->catkinToolsWorkingDirWidget->setFileName(makeStep->rosBuildConfiguration()->project()->projectDirectory());

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

    connect(m_ui->catkinToolsWorkingDirWidget, &Utils::PathChooser::rawPathChanged,
            this, &ROSCatkinToolsStepWidget::updateDetails);

    connect(m_makeStep, SIGNAL(enabledChanged()),
            this, SLOT(enabledChanged()));

    ROSBuildConfiguration *bc = m_makeStep->rosBuildConfiguration();
    connect(bc, SIGNAL(buildSystemChanged(ROSUtils::BuildSystem)),
            this, SLOT(updateBuildSystem(ROSUtils::BuildSystem)));

    connect(bc, &ROSBuildConfiguration::cmakeBuildTypeChanged,
            this, &ROSCatkinToolsStepWidget::updateDetails);

    connect(bc, &ROSBuildConfiguration::environmentChanged,
            this, &ROSCatkinToolsStepWidget::updateDetails);

    connect(ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateDetails()));

    Core::VariableChooser::addSupportForChildWidgets(this, makeStep->rosBuildConfiguration()->macroExpander());
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
    m_makeStep->m_catkinToolsWorkingDir = m_ui->catkinToolsWorkingDirWidget->rawPath();

    ROSBuildConfiguration *bc = m_makeStep->rosBuildConfiguration();
    ROSUtils::WorkspaceInfo workspaceInfo = ROSUtils::getWorkspaceInfo(bc->project()->projectDirectory(), bc->rosBuildSystem(), bc->project()->distribution());

    m_ui->catkinToolsWorkingDirWidget->setEnvironment(bc->environment());

    ProcessParameters param;
    param.setMacroExpander(bc->macroExpander());
    param.setEnvironment(bc->environment());
    param.setWorkingDirectory(Utils::FilePath::fromString(m_makeStep->m_catkinToolsWorkingDir));
    param.setCommandLine(m_makeStep->makeCommand(m_makeStep->allArguments(bc->cmakeBuildType(), false)));
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
    if(m_makeStep->enabled() && (bc->rosBuildSystem() != ROSUtils::CatkinTools))
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
    for(const QString& profile : profileNames)
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

    if (!ROSUtils::createCatkinToolsProfile(m_makeStep->rosBuildConfiguration()->project()->projectDirectory(), name, false))
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
    Utils::FilePath profile = ROSUtils::getCatkinToolsProfile(m_makeStep->rosBuildConfiguration()->project()->projectDirectory(), profileName);

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
            for (const QString& profile : profileNames) {
                if (profile == m_makeStep->activeProfile())
                    continue;
                pNames.append(profile);
            }
            return Utils::makeUniquelyNumbered(result, pNames);
        }

        return Utils::makeUniquelyNumbered(result, profileNames);
    }

    return result;
}

//
// ROSCatkinToolsListEditorWidget
//

ROSCatkinToolsListEditorWidget::ROSCatkinToolsListEditorWidget(QWidget *parent) : QDialog (parent)
{
    m_ui = new Ui::ROSCatkinToolsListEditor;
    m_ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);

    connect(m_ui->listWidget, &QListWidget::itemChanged, this, &ROSCatkinToolsListEditorWidget::listWidget_itemChanged);
    connect(m_ui->listWidget, &QListWidget::itemSelectionChanged, this, &ROSCatkinToolsListEditorWidget::listWidget_itemSelectionChanged);

    connect(m_ui->addPushButton, &QAbstractButton::clicked, this, &ROSCatkinToolsListEditorWidget::addPushButton_clicked);
    connect(m_ui->removePushButton, &QAbstractButton::clicked, this, &ROSCatkinToolsListEditorWidget::removePushButton_clicked);

    connect(m_ui->moveUpPushButton, &QAbstractButton::clicked, this, &ROSCatkinToolsListEditorWidget::moveUpPushButton_clicked);
    connect(m_ui->moveDownPushButton, &QAbstractButton::clicked, this, &ROSCatkinToolsListEditorWidget::moveDownPushButton_clicked);

    connect(m_ui->cancelPushButton, &QAbstractButton::clicked, this, &ROSCatkinToolsListEditorWidget::reject);
    connect(m_ui->acceptPushButton, &QAbstractButton::clicked, this, &ROSCatkinToolsListEditorWidget::accept);
}

ROSCatkinToolsListEditorWidget::~ROSCatkinToolsListEditorWidget()
{
    delete m_ui;
}

void ROSCatkinToolsListEditorWidget::setList(const QStringList &list)
{
    m_ui->listWidget->clear();
    for (const QString &str : list)
        addItem(str);

    listWidget_itemChanged();
}

QStringList ROSCatkinToolsListEditorWidget::list() const
{
    return m_list;
}

void ROSCatkinToolsListEditorWidget::listWidget_itemChanged()
{
    int cnt = m_ui->listWidget->count();
    m_ui->removePushButton->setEnabled(cnt != 0);

    m_list.clear();
    for (auto index = 0; index < cnt; index++)
        m_list.append(m_ui->listWidget->item(index)->text().trimmed());
}

void ROSCatkinToolsListEditorWidget::listWidget_itemSelectionChanged()
{
    int cnt = m_ui->listWidget->count();
    int index = m_ui->listWidget->currentRow();

    m_ui->moveDownPushButton->setEnabled(cnt > 1 && index != cnt-1);
    m_ui->moveUpPushButton->setEnabled(cnt > 1 && index != 0);
}

void ROSCatkinToolsListEditorWidget::addPushButton_clicked()
{
    addItem(QLatin1String("New Item"));

    listWidget_itemChanged();
}

void ROSCatkinToolsListEditorWidget::removePushButton_clicked()
{
   qDeleteAll(m_ui->listWidget->selectedItems());

   listWidget_itemChanged();
}

void ROSCatkinToolsListEditorWidget::moveUpPushButton_clicked()
{
    int currentIndex = m_ui->listWidget->currentRow();
    if (currentIndex != -1)
    {
        QListWidgetItem *currentItem = m_ui->listWidget->takeItem(currentIndex);
        m_ui->listWidget->insertItem(currentIndex-1, currentItem);
        m_ui->listWidget->setCurrentRow(currentIndex-1);

        listWidget_itemChanged();
    }
}

void ROSCatkinToolsListEditorWidget::moveDownPushButton_clicked()
{
    int currentIndex = m_ui->listWidget->currentRow();
    if (currentIndex != -1)
    {
        QListWidgetItem *currentItem = m_ui->listWidget->takeItem(currentIndex);
        m_ui->listWidget->insertItem(currentIndex+1, currentItem);
        m_ui->listWidget->setCurrentRow(currentIndex+1);

        listWidget_itemChanged();
    }
}

void ROSCatkinToolsListEditorWidget::addItem(const QString &str)
{
    QListWidgetItem *item = new QListWidgetItem();
    item->setFlags(item->flags () | Qt::ItemIsEditable);
    item->setText(str);
    m_ui->listWidget->addItem(item);
    m_ui->listWidget->setCurrentItem(item);
    m_ui->listWidget->editItem(item);
}


//
// ROSCatkinToolsListWidget
//

ROSCatkinToolsListWidget::ROSCatkinToolsListWidget(QWidget *parent) : QLineEdit(parent)
{
    this->setReadOnly(true);
    m_editor = new ROSCatkinToolsListEditorWidget(this);

    QAction *editor;
    editor = this->addAction(QIcon(QLatin1String(":rosproject/pencil_icon.png")), QLineEdit::TrailingPosition);
    connect(editor, SIGNAL(triggered(bool)), this, SLOT(onActionEditListTriggered()));
}

ROSCatkinToolsListWidget::~ROSCatkinToolsListWidget()
{
    delete m_editor;
}

void ROSCatkinToolsListWidget::onActionEditListTriggered()
{
    QPoint globalPos = this->mapToGlobal(this->rect().bottomLeft());
    m_editor->setFixedSize(this->width(), m_editor->height());
    m_editor->move(globalPos);

    if(m_editor->exec() == QDialog::Accepted)
        setList(m_editor->list());
}

void ROSCatkinToolsListWidget::setList(const QStringList &list)
{
    m_list = list;
    setText(QString("[%1]").arg(list.join("; ")));
}

QStringList ROSCatkinToolsListWidget::list() const
{
    return m_list;
}

//
// ROSCatkinToolsConfigEditorWidget
//

ROSCatkinToolsConfigEditorWidget::ROSCatkinToolsConfigEditorWidget() : QWidget()
{
    m_ui = new Ui::ROSCatkinToolsConfigEditor;
    m_ui->setupUi(this);
    m_modified = false;

    m_editor = new ROSCatkinToolsListEditorWidget();

    connect(m_ui->saveButton, &QAbstractButton::clicked, this, &ROSCatkinToolsConfigEditorWidget::saveProfileConfig);

    connect(m_ui->extend_path_chooser, &Utils::PathChooser::pathChanged, this, &ROSCatkinToolsConfigEditorWidget::propertyChanged);

    connect(m_ui->blacklist_lineEdit, &QLineEdit::textChanged, this, &ROSCatkinToolsConfigEditorWidget::propertyChanged);
    connect(m_ui->whitelist_lineEdit, &QLineEdit::textChanged, this, &ROSCatkinToolsConfigEditorWidget::propertyChanged);
    connect(m_ui->space_source_lineEdit, &QLineEdit::textChanged, this, &ROSCatkinToolsConfigEditorWidget::propertyChanged);
    connect(m_ui->space_build_lineEdit, &QLineEdit::textChanged, this, &ROSCatkinToolsConfigEditorWidget::propertyChanged);
    connect(m_ui->space_devel_lineEdit, &QLineEdit::textChanged, this, &ROSCatkinToolsConfigEditorWidget::propertyChanged);
    connect(m_ui->space_install_lineEdit, &QLineEdit::textChanged, this, &ROSCatkinToolsConfigEditorWidget::propertyChanged);
    connect(m_ui->space_log_lineEdit, &QLineEdit::textChanged, this, &ROSCatkinToolsConfigEditorWidget::propertyChanged);
    connect(m_ui->catkin_args_lineEdit, &QLineEdit::textChanged, this, &ROSCatkinToolsConfigEditorWidget::propertyChanged);
    connect(m_ui->cmake_args_lineEdit, &QLineEdit::textChanged, this, &ROSCatkinToolsConfigEditorWidget::propertyChanged);
    connect(m_ui->make_args_lineEdit, &QLineEdit::textChanged, this, &ROSCatkinToolsConfigEditorWidget::propertyChanged);

    connect(m_ui->space_devel_layout_comboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ROSCatkinToolsConfigEditorWidget::propertyChanged);
    connect(m_ui->space_install_option_comboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ROSCatkinToolsConfigEditorWidget::propertyChanged);
    connect(m_ui->space_install_layout_comboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ROSCatkinToolsConfigEditorWidget::propertyChanged);

    connect(m_ui->jobs_checkBox, &QCheckBox::stateChanged, this, &ROSCatkinToolsConfigEditorWidget::propertyChanged);
    connect(m_ui->package_jobs_checkBox, &QCheckBox::stateChanged, this, &ROSCatkinToolsConfigEditorWidget::propertyChanged);
    connect(m_ui->env_cache_checkBox, &QCheckBox::stateChanged, this, &ROSCatkinToolsConfigEditorWidget::propertyChanged);
    connect(m_ui->jobserver_checkBox, &QCheckBox::stateChanged, this, &ROSCatkinToolsConfigEditorWidget::propertyChanged);

    connect(m_ui->jobs_spinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ROSCatkinToolsConfigEditorWidget::propertyChanged);
    connect(m_ui->package_jobs_spinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ROSCatkinToolsConfigEditorWidget::propertyChanged);
}

ROSCatkinToolsConfigEditorWidget::~ROSCatkinToolsConfigEditorWidget()
{
    delete m_ui;
    delete m_editor;
}

bool ROSCatkinToolsConfigEditorWidget::parseProfileConfig(Utils::FilePath filePath)
{
    m_profileConfigPath = filePath;
    if (!m_profileConfigPath.exists())
    {
        Core::MessageManager::write(tr("[ROS Warning] Catkin Tools Profile Config File: %1, does not exist.").arg(m_profileConfigPath.toString()));
        return false;
    }
    m_parsing = true;

    m_profile_original = YAML::LoadFile(m_profileConfigPath.toString().toStdString());
    m_profile_current = m_profile_original;
    m_modified = false;

    m_ui->extend_path_chooser->setPath(parseString("extend_path"));

    m_ui->whitelist_lineEdit->setList(parseList("whitelist"));
    m_ui->blacklist_lineEdit->setList(parseList("blacklist"));
    m_ui->space_source_lineEdit->setText(parseString("source_space"));
    m_ui->space_build_lineEdit->setText(parseString("build_space"));
    m_ui->space_devel_lineEdit->setText(parseString("devel_space"));
    m_ui->space_install_lineEdit->setText(parseString("install_space"));
    m_ui->space_log_lineEdit->setText(parseString("log_space"));
    m_ui->catkin_args_lineEdit->setList(parseList("catkin_make_args"));
    m_ui->cmake_args_lineEdit->setList(parseList("cmake_args"));
    m_ui->make_args_lineEdit->setList(parseList("make_args"));

    QString layout = parseString("devel_layout");
    layout[0] = layout[0].toUpper();
    m_ui->space_devel_layout_comboBox->setCurrentText(layout);
    m_ui->space_install_option_comboBox->setCurrentIndex(parseBool("install"));
    m_ui->space_install_layout_comboBox->setCurrentIndex(parseBool("isolate_install"));

    QStringList jobs_args = parseList("jobs_args");
    m_ui->jobs_checkBox->setChecked(!jobs_args.isEmpty());
    if (!jobs_args.isEmpty())
        m_ui->jobs_spinBox->setValue(jobs_args[0].mid(2).trimmed().toInt());

    //TODO: Need to figure out how to parse package jobs

    m_ui->env_cache_checkBox->setChecked(parseBool("use_env_cache"));
    m_ui->jobserver_checkBox->setChecked(parseBool("use_internal_make_jobserver"));

    m_parsing = false;
    return true;
}

QStringList ROSCatkinToolsConfigEditorWidget::parseList(const std::string &key) const
{
    if (!m_profile_current[key].IsSequence())
        return QStringList();

    std::vector<std::string> vector_list = m_profile_current[key].as<std::vector<std::string> >();
    return toQStringList(vector_list);
}

QString ROSCatkinToolsConfigEditorWidget::parseString(const std::string &key) const
{
    if (!m_profile_current[key].IsScalar())
        return QString();

    return QString::fromStdString(m_profile_current[key].as<std::string>());
}

bool ROSCatkinToolsConfigEditorWidget::parseBool(const std::string &key) const
{
    if (!m_profile_current[key].IsScalar())
        return false;

    return m_profile_current[key].as<bool>();
}

std::vector<std::string> ROSCatkinToolsConfigEditorWidget::toStdVector(const QStringList &list) const
{
    std::vector<std::string> l;
    for (auto const& str: list)
        l.push_back(str.toStdString());

    return l;
}

QStringList ROSCatkinToolsConfigEditorWidget::toQStringList(const std::vector<std::string> &list) const
{
    QStringList l;
    for (auto const& str: list)
        l.append(QString::fromStdString(str));

    return l;
}

bool ROSCatkinToolsConfigEditorWidget::saveProfileConfig()
{
    if (!m_profileConfigPath.exists())
    {
        Core::MessageManager::write(tr("[ROS Warning] Catkin Tools Profile Config File: %1, does not exist.").arg(m_profileConfigPath.toString()));
        return false;
    }

    std::ofstream fout(m_profileConfigPath.toString().toStdString());
    fout << m_profile_current; // dump it back into the file
    fout.close();

    m_profile_original = m_profile_current;
    m_modified = false;
    m_ui->saveButton->setEnabled(false);

    return true;
}

void ROSCatkinToolsConfigEditorWidget::propertyChanged()
{
    if (!m_parsing)
    {
        m_modified = true;

        if (!m_ui->extend_path_chooser->path().isEmpty())
            m_profile_original["extend_path"] = m_ui->extend_path_chooser->path().trimmed().toStdString();
        else
            m_profile_original["extend_path"] = "null";

        m_profile_original["whitelist"] = toStdVector(m_ui->whitelist_lineEdit->list());
        m_profile_original["blacklist"] = toStdVector(m_ui->blacklist_lineEdit->list());

        m_profile_original["source_space"] = m_ui->space_source_lineEdit->text().trimmed().toStdString();
        m_profile_original["build_space"] = m_ui->space_build_lineEdit->text().trimmed().toStdString();
        m_profile_original["devel_space"] = m_ui->space_devel_lineEdit->text().trimmed().toStdString();
        m_profile_original["install_space"] = m_ui->space_install_lineEdit->text().trimmed().toStdString();
        m_profile_original["log_space"] = m_ui->space_log_lineEdit->text().trimmed().toStdString();

        m_profile_original["catkin_make_args"] = toStdVector(m_ui->catkin_args_lineEdit->list());
        m_profile_original["cmake_args"] = toStdVector(m_ui->cmake_args_lineEdit->list());
        m_profile_original["make_args"] = toStdVector(m_ui->make_args_lineEdit->list());

        m_profile_original["devel_layout"] = m_ui->space_devel_layout_comboBox->currentText().toLower().trimmed().toStdString();
        m_profile_original["install"] = m_ui->space_install_option_comboBox->currentIndex() ? "true" : "false";
        m_profile_original["isolate_install"] = m_ui->space_install_layout_comboBox->currentIndex() ? "true" : "false";

        std::vector<std::string> jobs_args;
        if (m_ui->jobs_checkBox->checkState() == Qt::Checked)
            jobs_args.push_back(QString("-j%1").arg(m_ui->jobs_spinBox->value()).toStdString());

        m_profile_original["jobs_args"] = jobs_args;

        //TODO: Need to figure out how to parse jobs and package jobs

        m_profile_original["use_env_cache"] = m_ui->env_cache_checkBox->checkState() == Qt::Checked ? "true" : "false";
        m_profile_original["use_internal_make_jobserver"] = m_ui->jobserver_checkBox->checkState() == Qt::Checked ? "true" : "false";

        m_ui->saveButton->setEnabled(isValid());
    }
}

bool ROSCatkinToolsConfigEditorWidget::isModified() const
{
    return m_modified;
}

bool ROSCatkinToolsConfigEditorWidget::isValid() const
{
    bool valid = true;
    if (!m_ui->extend_path_chooser->path().isEmpty())
        valid &= m_ui->extend_path_chooser->isValid();

    valid &= m_ui->space_source_lineEdit->isValid();
    valid &= m_ui->space_build_lineEdit->isValid();
    valid &= m_ui->space_devel_lineEdit->isValid();
    valid &= m_ui->space_install_lineEdit->isValid();
    valid &= m_ui->space_log_lineEdit->isValid();

    return valid;
}

//
// ROSCatkinToolsProfileEditorDialog
//

ROSCatkinToolsProfileEditorDialog::ROSCatkinToolsProfileEditorDialog(Utils::FilePath filePath) : QDialog()
{
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Dialog);
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

ROSCatkinToolsStepFactory::ROSCatkinToolsStepFactory() : BuildStepFactory()
{
  registerStep<ROSCatkinToolsStep>(ROS_CTS_ID);
  setFlags(BuildStepInfo::Flags::UniqueStep);
  setDisplayName(QCoreApplication::translate("ROSProjectManager::Internal::ROSCatkinToolsConfigStep", ROS_CTS_DISPLAY_NAME));
  setSupportedProjectType(Constants::ROS_PROJECT_ID);
  setSupportedStepLists({ProjectExplorer::Constants::BUILDSTEPS_BUILD, ProjectExplorer::Constants::BUILDSTEPS_CLEAN});
}

} // namespace Internal
} // namespace ROSProjectManager
