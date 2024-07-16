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
#include "ros_package_wizard.h"
#include "ros_utils.h"
#include "ui_ros_package_wizard_details_page.h"
#include "ros_project_constants.h"
#include "ros_project.h"
#include "ros_build_configuration.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreicons.h>
#include <coreplugin/messagemanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/customwizard/customwizard.h>
#include <projectexplorer/projecttree.h>

#include <utils/filewizardpage.h>
#include <utils/wizard.h>

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QPainter>
#include <QPixmap>
#include <QStyle>
#include <QProcess>
#include <QXmlStreamReader>
#include <QPlainTextEdit>
#include <QMessageBox>
#include <projectexplorer/projecttree.h>

#include <functional>

namespace ROSProjectManager {
namespace Internal {

//////////////////////////////////////////////////////////////////////////////
//
// ROSPackageWizardDialog
//
//////////////////////////////////////////////////////////////////////////////

ROSPackageWizardDialog::ROSPackageWizardDialog(const Core::BaseFileWizardFactory *factory,
                                                       QWidget *parent) :
    Core::BaseFileWizard(factory, QVariantMap(), parent)
{

    setWindowTitle(tr("Create ROS Package"));

    // first page
    m_detailsPage = new ROSPackageWizardDetailsPage;
    m_detailsPage->setTitle(tr("Package Details"));

    addPage(m_detailsPage);
}

void ROSPackageWizardDialog::setPath(const Utils::FilePath &path) {m_detailsPage->setPath(path);}

void ROSPackageWizardDialog::setProjectDirectory(const Utils::FilePath &path) {m_detailsPage->setProjectDirectory(path);}

QString ROSPackageWizardDialog::packageName() const {return m_detailsPage->packageName();}

QString ROSPackageWizardDialog::packagePath() const {return m_detailsPage->packagePath();}

QString ROSPackageWizardDialog::version() const {return m_detailsPage->version();}

QString ROSPackageWizardDialog::licenses() const {return m_detailsPage->licenses();}

QString ROSPackageWizardDialog::description() const {return m_detailsPage->description();}

QStringList ROSPackageWizardDialog::catkin_dependencies() const {return m_detailsPage->catkin_dependencies();}

QStringList ROSPackageWizardDialog::system_dependencies() const {return m_detailsPage->system_dependencies();}

QStringList ROSPackageWizardDialog::boost_components() const {return m_detailsPage->boost_components();}

//////////////////////////////////////////////////////////////////////////////
//
// ROSPackageWizardPagePrivate
//
//////////////////////////////////////////////////////////////////////////////
class ROSPackageWizardDetailsPagePrivate
{
public:
    ROSPackageWizardDetailsPagePrivate();
    Ui::ROSPackageWizardDetailsPage m_ui;
    bool m_complete;

};

ROSPackageWizardDetailsPagePrivate::ROSPackageWizardDetailsPagePrivate() :
    m_complete(false)
{
}

//////////////////////////////////////////////////////////////////////////////
//
// ROSPackageWizardDetailsPage
//
//////////////////////////////////////////////////////////////////////////////
ROSPackageWizardDetailsPage::ROSPackageWizardDetailsPage(QWidget *parent) :
    WizardPage(parent),
    d(new ROSPackageWizardDetailsPagePrivate)
{
    d->m_ui.setupUi(this);

    std::function<bool(Utils::FancyLineEdit *, QString *)> fn = std::bind( &ROSPackageWizardDetailsPage::validateWithValidator, this, std::placeholders::_1, std::placeholders::_2);
    d->m_ui.pathChooser->setValidationFunction(fn);

    connect(d->m_ui.pathChooser, &Utils::PathChooser::validChanged,
            this, &ROSPackageWizardDetailsPage::slotPackagePathValidChanged);
    connect(d->m_ui.packageNameLineEdit, &Utils::FancyLineEdit::validChanged,
            this, &ROSPackageWizardDetailsPage::slotPackageNameValidChanged);

    connect(d->m_ui.pathChooser, &Utils::PathChooser::returnPressed,
            this, &ROSPackageWizardDetailsPage::slotActivated);
    connect(d->m_ui.packageNameLineEdit, &Utils::FancyLineEdit::validReturnPressed,
            this, &ROSPackageWizardDetailsPage::slotActivated);

}

ROSPackageWizardDetailsPage::~ROSPackageWizardDetailsPage() {delete d;}

void ROSPackageWizardDetailsPage::setPath(const Utils::FilePath &path) {d->m_ui.pathChooser->setPath(path.toString());}

void ROSPackageWizardDetailsPage::setProjectDirectory(const Utils::FilePath &path)
{
    d->m_ui.pathChooser->setInitialBrowsePathBackup(path);
    d->m_ui.pathChooser->lineEdit()->setPlaceholderText(path.toString());
}

QString ROSPackageWizardDetailsPage::packageName() const {return d->m_ui.packageNameLineEdit->text();}

QString ROSPackageWizardDetailsPage::packagePath() const {return d->m_ui.pathChooser->filePath().toString();}

QString ROSPackageWizardDetailsPage::version() const {return d->m_ui.versionLineEdit->text();}

QString ROSPackageWizardDetailsPage::licenses() const {return d->m_ui.licenseComboBox->currentText();}

QString ROSPackageWizardDetailsPage::description() const {return d->m_ui.descriptionPlainTextEdit->toPlainText();}

QStringList ROSPackageWizardDetailsPage::catkin_dependencies() const {return processList(d->m_ui.catkinLineEdit->text());}

QStringList ROSPackageWizardDetailsPage::system_dependencies() const {return processList(d->m_ui.systemLineEdit->text());}

QStringList ROSPackageWizardDetailsPage::boost_components() const {return processList(d->m_ui.boostLineEdit->text());}

bool ROSPackageWizardDetailsPage::isComplete() const {return d->m_complete;}

void ROSPackageWizardDetailsPage::slotPackageNameValidChanged() {validChangedHelper();}

void ROSPackageWizardDetailsPage::slotPackagePathValidChanged() {validChangedHelper();}

void ROSPackageWizardDetailsPage::validChangedHelper()
{
    const bool newComplete = d->m_ui.pathChooser->isValid() && d->m_ui.packageNameLineEdit->isValid();
    if (newComplete != d->m_complete) {
        d->m_complete = newComplete;
        emit completeChanged();
    }
}

void ROSPackageWizardDetailsPage::slotActivated()
{
    if (d->m_complete)
        emit activated();
}

QStringList ROSPackageWizardDetailsPage::processList(const QString &text) const
{
    return text.split(QRegularExpression(QLatin1String("[,; ]")), Qt::SkipEmptyParts);
}

bool ROSPackageWizardDetailsPage::validateWithValidator(Utils::FancyLineEdit *edit, QString *errorMessage)
{
    const QString path = edit->text();
    if (path.isEmpty()) {
        if (errorMessage)
            *errorMessage = tr("The path \"%1\" expanded to an empty string.").arg(QDir::toNativeSeparators(path));
        return false;
    }

    const QFileInfo fi(path);

    if (!path.startsWith(edit->placeholderText()))
    {
        if (errorMessage)
            *errorMessage = tr("The path \"%1\" is not in the workspace.").arg(QDir::toNativeSeparators(path));

        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////////
//
// ROSPackageWizard
//
//////////////////////////////////////////////////////////////////////////////

ROSPackageWizard::ROSPackageWizard()
{
    setSupportedProjectTypes({});
    setIcon(QIcon(QLatin1String(":rosproject/folderpackage.png")));
    setDisplayName(tr("Package"));
    setId("A.ROS");
    setDescription(tr("Create a ROS package."));
    setCategory(QLatin1String(Constants::ROS_WIZARD_CATEGORY));
    setDisplayCategory(QLatin1String(Constants::ROS_WIZARD_CATEGORY_DISPLAY));
    setFlags(Core::IWizardFactory::PlatformIndependent);
}

Core::BaseFileWizard *ROSPackageWizard::create(QWidget *parent, const Core::WizardDialogParameters &parameters) const
{
    Utils::FilePath defaultPath = parameters.defaultPath();

    ROSProject *rosProject = qobject_cast<ROSProject *>(ProjectExplorer::ProjectTree::currentProject());

    if(!rosProject )
        return nullptr;

    ROSBuildConfiguration *bc = rosProject->rosBuildConfiguration();

    if( bc )
    {
        ROSUtils::WorkspaceInfo workspaceInfo = ROSUtils::getWorkspaceInfo(bc->project()->projectDirectory(),
                                                                           bc->rosBuildSystem(),
                                                                           bc->project()->distribution());

        if( defaultPath ==  workspaceInfo.path )
            defaultPath = workspaceInfo.sourcePath;
    }

    m_wizard = new ROSPackageWizardDialog(this, parent);

    m_wizard->setProjectDirectory(rosProject->projectDirectory());
    m_wizard->setPath(defaultPath);

    for (QWizardPage *p : m_wizard->extensionPages())
        m_wizard->addPage(p);

    return m_wizard;
}

Core::GeneratedFiles ROSPackageWizard::generateFiles(const QWizard *w,
                                                         QString *errorMessage) const
{
    Q_UNUSED(w);
    Q_UNUSED(errorMessage);

    Utils::FilePath packagePath = Utils::FilePath::fromString(m_wizard->packagePath());
    Utils::FilePath cmakelistPath = Utils::FilePath::fromString(m_wizard->packagePath());

    packagePath = packagePath.pathAppended(m_wizard->packageName()).pathAppended(QLatin1String("package.xml"));
    cmakelistPath = cmakelistPath.pathAppended(m_wizard->packageName()).pathAppended(QLatin1String("CMakeLists.txt"));

    Core::GeneratedFile generatedPackageFile(packagePath);
    generatedPackageFile.setAttributes(Core::GeneratedFile::CustomGeneratorAttribute);

    Core::GeneratedFile generatedCMakeListFile(cmakelistPath);
    generatedCMakeListFile.setAttributes(Core::GeneratedFile::CustomGeneratorAttribute);

    Core::GeneratedFiles files;
    files.append(generatedPackageFile);
    files.append(generatedCMakeListFile);

    return files;
}

bool ROSPackageWizard::writeFiles(const Core::GeneratedFiles &files, QString *errorMessage) const
{
  Q_UNUSED(files);
  Q_UNUSED(errorMessage);

  const ROSProject *project = static_cast<ROSProject *>(ProjectExplorer::ProjectTree::currentProject());

  const ROSUtils::BuildSystem bs = project->defaultBuildSystem();

  QStringList create_args;

  switch (bs) {
  case ROSUtils::BuildSystem::CatkinMake:
      create_args.append("catkin_create_pkg");
      break;
  case ROSUtils::BuildSystem::CatkinTools:
      create_args.append({"catkin", "create", "pkg"});
      break;
  case ROSUtils::BuildSystem::Colcon:
      create_args.append({"ros2", "pkg", "create"});
      break;
  }

  create_args.append(m_wizard->packageName());

  // for colcon, all dependencies are of the same type
  // TODO: change UI to a single set of dependencies for colcon
  if (bs == ROSUtils::BuildSystem::Colcon &&
      !m_wizard->catkin_dependencies().isEmpty() &&
      !m_wizard->system_dependencies().isEmpty() &&
      !m_wizard->boost_components().isEmpty())
  {
      create_args.append("--dependencies");
  }

  if (!m_wizard->catkin_dependencies().isEmpty()) {
      if (bs == ROSUtils::BuildSystem::CatkinTools)
        create_args.append("--catkin-deps");
      create_args.append(m_wizard->catkin_dependencies());
  }

  if (!m_wizard->system_dependencies().isEmpty()) {
      if (bs != ROSUtils::BuildSystem::Colcon)
        create_args.append("-s");
      create_args.append(m_wizard->system_dependencies());
  }

  if (!m_wizard->boost_components().isEmpty()) {
      if (bs == ROSUtils::BuildSystem::CatkinMake)
        create_args.append("--boost-comps");
      else if (bs == ROSUtils::BuildSystem::CatkinTools)
        create_args.append("--boost-components");
      create_args.append(m_wizard->boost_components());
  }

  if (!m_wizard->version().isEmpty()) {
      if (bs == ROSUtils::BuildSystem::CatkinMake)
          create_args.append({"-V", m_wizard->version()});
      else if (bs == ROSUtils::BuildSystem::CatkinTools)
        create_args.append({"-v", m_wizard->version()});
  }

  if (!m_wizard->licenses().isEmpty()) {
      if (bs == ROSUtils::BuildSystem::Colcon)
          create_args.append({"--license", QString("'%1'").arg(m_wizard->licenses())});
      else
          create_args.append({"-l", QString("'%1'").arg(m_wizard->licenses())});
  }

  if (!m_wizard->description().isEmpty())
      create_args.append({"--description", QString("'%1'").arg(m_wizard->description())});

  if (bs != ROSUtils::BuildSystem::Colcon)
    create_args.append({"--rosdistro", project->distribution().fileName()});

  // create package using ros command catkin_create_pkg
  const QDir packagePath = m_wizard->packagePath();
  if (!packagePath.exists()) {
      packagePath.mkpath(".");
  }

  QProcess create_pkg_proc;
  create_pkg_proc.setWorkingDirectory(packagePath.path());

  ROSUtils::sourceROS(&create_pkg_proc, project->distribution());
  create_pkg_proc.start("bash", {"-c", create_args.join(" ")});
  if (!create_pkg_proc.waitForStarted(-1)) {
      Core::MessageManager::writeFlashing(tr("[ROS Error] Failed to start catkin_create_pkg."));
      return false;
  }
  if (!create_pkg_proc.waitForFinished(-1)) {
      Core::MessageManager::writeFlashing(tr("[ROS Error] Failed to finish catkin_create_pkg."));
      return false;
  }
  const QByteArray message_stdio = create_pkg_proc.readAllStandardOutput();
  if (!message_stdio.isEmpty()) {
      Core::MessageManager::writeSilently(QString::fromStdString(message_stdio.toStdString()));
  }
  const QByteArray message_err = create_pkg_proc.readAllStandardError();
  if (!message_err.isEmpty()) {
      Core::MessageManager::writeFlashing(QString::fromStdString(message_err.toStdString()));
      return false;
  }
  if (create_pkg_proc.exitStatus() != QProcess::NormalExit) {
      Core::MessageManager::writeFlashing(tr("[ROS Error] Failed to create catkin package."));
      return false;
  }
  return true;
}

bool ROSPackageWizard::postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l,
                                             QString *errorMessage) const
{
    Q_UNUSED(w);
    Q_UNUSED(l);
    Q_UNUSED(errorMessage);

    return true;
}

} // namespace Internal
} // namespace ROSProjectManager
