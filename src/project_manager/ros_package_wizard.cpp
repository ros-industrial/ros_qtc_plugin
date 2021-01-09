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
#include <utils/mimetypes/mimedatabase.h>
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

void ROSPackageWizardDialog::setPath(const QString &path) {m_detailsPage->setPath(path);}

void ROSPackageWizardDialog::setProjectDirectory(const QString &path) {m_detailsPage->setProjectDirectory(path);}

QString ROSPackageWizardDialog::packageName() const {return m_detailsPage->packageName();}

QString ROSPackageWizardDialog::packagePath() const {return m_detailsPage->packagePath();}

QString ROSPackageWizardDialog::version() const {return m_detailsPage->version();}

QString ROSPackageWizardDialog::licenses() const {return m_detailsPage->licenses();}

QString ROSPackageWizardDialog::description() const {return m_detailsPage->description();}

QStringList ROSPackageWizardDialog::authors() const {return m_detailsPage->authors();}

QStringList ROSPackageWizardDialog::maintainers() const {return m_detailsPage->maintainers();}

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

void ROSPackageWizardDetailsPage::setPath(const QString &path) {d->m_ui.pathChooser->setPath(path);}

void ROSPackageWizardDetailsPage::setProjectDirectory(const QString &path)
{
    d->m_ui.pathChooser->setInitialBrowsePathBackup(path);
    d->m_ui.pathChooser->lineEdit()->setPlaceholderText(path);
}

QString ROSPackageWizardDetailsPage::packageName() const {return d->m_ui.packageNameLineEdit->text();}

QString ROSPackageWizardDetailsPage::packagePath() const {return d->m_ui.pathChooser->path();}

QString ROSPackageWizardDetailsPage::version() const {return d->m_ui.versionLineEdit->text();}

QString ROSPackageWizardDetailsPage::licenses() const {return d->m_ui.licenseComboBox->currentText();}

QString ROSPackageWizardDetailsPage::description() const {return d->m_ui.descriptionPlainTextEdit->toPlainText();}

QStringList ROSPackageWizardDetailsPage::authors() const {return processList(d->m_ui.authorsLineEdit->text());}

QStringList ROSPackageWizardDetailsPage::maintainers() const {return processList(d->m_ui.maintainersLineEdit->text());}

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
    QStringList newList;
    for (const QString& str : text.split(QRegExp(QLatin1String("[,;]")), QString::SkipEmptyParts))
    {
        newList.append(QString::fromLatin1("\"%1\"").arg(str.trimmed()));
    }
    return newList;
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
    QString defaultPath = parameters.defaultPath();

    ROSProject *rosProject = qobject_cast<ROSProject *>(ProjectExplorer::ProjectTree::currentProject());

    if(!rosProject )
        return nullptr;

    ROSBuildConfiguration *bc = rosProject->rosBuildConfiguration();

    if( bc )
    {
        ROSUtils::WorkspaceInfo workspaceInfo = ROSUtils::getWorkspaceInfo(bc->project()->projectDirectory(),
                                                                           bc->rosBuildSystem(),
                                                                           bc->project()->distribution());

        if( defaultPath ==  workspaceInfo.path.toString() )
            defaultPath = workspaceInfo.sourcePath.toString();
    }

    m_wizard = new ROSPackageWizardDialog(this, parent);

    m_wizard->setProjectDirectory(rosProject->projectDirectory().toString() + QDir::separator());
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

    QString package;
    Utils::FilePath packagePath = Utils::FilePath::fromString(m_wizard->packagePath());
    Utils::FilePath cmakelistPath = Utils::FilePath::fromString(m_wizard->packagePath());

    packagePath = packagePath.pathAppended(m_wizard->packageName()).pathAppended(QLatin1String("package.xml"));
    cmakelistPath = cmakelistPath.pathAppended(m_wizard->packageName()).pathAppended(QLatin1String("CMakeLists.txt"));

    Core::GeneratedFile generatedPackageFile(packagePath.toString());
    generatedPackageFile.setAttributes(Core::GeneratedFile::CustomGeneratorAttribute);

    Core::GeneratedFile generatedCMakeListFile(cmakelistPath.toString());
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

  ROSProject *project = static_cast<ROSProject *>(ProjectExplorer::ProjectTree::currentProject());

  QProcess *catkin_create_pkg = new QProcess();

  QString cmd = QLatin1String("catkin_create_pkg");

  cmd += QString::fromLatin1(" \"%1\"").arg(m_wizard->packageName());

  if (!m_wizard->catkin_dependencies().isEmpty())
      cmd += m_wizard->catkin_dependencies().join(QLatin1Literal(" ")).insert(0,QLatin1Literal(" "));

  if (!m_wizard->system_dependencies().isEmpty())
      cmd += m_wizard->system_dependencies().join(QLatin1Literal(" ")).insert(0,QLatin1Literal(" -s "));

  if (!m_wizard->boost_components().isEmpty())
      cmd += m_wizard->boost_components().join(QLatin1Literal(" ")).insert(0,QLatin1Literal(" -b "));

  if (!m_wizard->version().isEmpty())
      cmd += QString::fromLatin1(" -V \"%1\"").arg(m_wizard->version());

  if (!m_wizard->licenses().isEmpty())
      cmd += QString::fromLatin1(" -l \"%1\"").arg(m_wizard->licenses());

  if (!m_wizard->description().isEmpty())
      cmd += QString::fromLatin1(" -D \"%1\"").arg(m_wizard->description());

  if (!m_wizard->authors().isEmpty())
      cmd += m_wizard->authors().join(QLatin1Literal(" -a ")).insert(0,QLatin1Literal(" -a "));

  if (!m_wizard->maintainers().isEmpty())
      cmd += m_wizard->maintainers().join(QLatin1Literal(" -m ")).insert(0,QLatin1Literal(" -m "));

  cmd += QString::fromLatin1(" --rosdistro \"%1\"").arg(project->distribution().fileName());

  // create package using ros command catkin_create_pkg
  Utils::FilePath packagePath = Utils::FilePath::fromString(m_wizard->packagePath());
  catkin_create_pkg->setWorkingDirectory(packagePath.toString());

  ROSUtils::sourceROS(catkin_create_pkg, project->distribution());
  catkin_create_pkg->start(QLatin1String("bash"), QStringList() << QLatin1String("-c") << cmd);
  catkin_create_pkg->waitForFinished();
  bool success;
  if (catkin_create_pkg->exitStatus() != QProcess::CrashExit)
  {
    success = true;
  }
  else
  {
    Core::MessageManager::write(tr("[ROS Error] Faild to create catkin package."));
    success = false;
  }

  delete catkin_create_pkg;
  return success;
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
