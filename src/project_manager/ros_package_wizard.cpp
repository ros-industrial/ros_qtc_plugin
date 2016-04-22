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

#include "ros_package_wizard.h"
#include "ros_utils.h"
#include "ui_ros_package_wizard_details_page.h"
#include "ros_project_constants.h"
#include "ros_project.h"
#include "ros_build_configuration.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreicons.h>
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

    connect(d->m_ui.pathChooser, &Utils::PathChooser::validChanged,
            this, &ROSPackageWizardDetailsPage::slotPackagePathValidChanged);
    connect(d->m_ui.packageNameLineEdit, &Utils::FancyLineEdit::validChanged,
            this, &ROSPackageWizardDetailsPage::slotPackageNameValidChanged);

    connect(d->m_ui.pathChooser, &Utils::PathChooser::pathChanged,
            this, &ROSPackageWizardDetailsPage::slotPackagePathChanged);

    connect(d->m_ui.pathChooser, &Utils::PathChooser::returnPressed,
            this, &ROSPackageWizardDetailsPage::slotActivated);
    connect(d->m_ui.packageNameLineEdit, &Utils::FancyLineEdit::validReturnPressed,
            this, &ROSPackageWizardDetailsPage::slotActivated);

}

ROSPackageWizardDetailsPage::~ROSPackageWizardDetailsPage() {delete d;}

void ROSPackageWizardDetailsPage::setPath(const QString &path) {d->m_ui.pathChooser->setPath(path);}

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

void ROSPackageWizardDetailsPage::slotPackagePathChanged(const QString &path)
{
  Q_UNUSED(path)

  if (!d->m_ui.pathChooser->isValid())
  {
      d->m_ui.pathChooser->setPath(QLatin1String(""));
  }

  validChangedHelper();
}

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
    foreach(QString str, text.split(QRegExp(QLatin1String("[,;]")), QString::SkipEmptyParts))
    {
        newList.append(QString::fromLatin1("\"%1\"").arg(str.trimmed()));
    }
    return newList;
}

//////////////////////////////////////////////////////////////////////////////
//
// ROSPackageWizard
//
//////////////////////////////////////////////////////////////////////////////

ROSPackageWizard::ROSPackageWizard()
{
    setSupportedProjectTypes({});
    setIcon(qApp->style()->standardIcon(QStyle::SP_DirIcon));
    setDisplayName(tr("Package"));
    setId("A.ROS");
    setDescription(tr("Create a ROS package."));
    setCategory(QLatin1String(Constants::ROS_WIZARD_CATEGORY));
    setDisplayCategory(QLatin1String(Constants::ROS_WIZARD_CATEGORY_DISPLAY));
    setFlags(Core::IWizardFactory::PlatformIndependent);
}

Core::BaseFileWizard *ROSPackageWizard::create(QWidget *parent,
                                                   const Core::WizardDialogParameters &parameters) const
{
    Q_UNUSED(parameters)
    m_wizard = new ROSPackageWizardDialog(this, parent);
    m_wizard->setPath(parameters.defaultPath());

    foreach (QWizardPage *p, m_wizard->extensionPages())
        m_wizard->addPage(p);

    return m_wizard;
}

Core::GeneratedFiles ROSPackageWizard::generateFiles(const QWizard *w,
                                                         QString *errorMessage) const
{
    Q_UNUSED(w)
    Q_UNUSED(errorMessage)

    QString package;
    Utils::FileName packagePath = Utils::FileName::fromString(m_wizard->packagePath());
    Utils::FileName cmakelistPath = Utils::FileName::fromString(m_wizard->packagePath());

    packagePath.appendPath(m_wizard->packageName()).appendPath(QLatin1String("package.xml"));
    cmakelistPath.appendPath(m_wizard->packageName()).appendPath(QLatin1String("CMakeList.txt"));

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
  Q_UNUSED(files)
  Q_UNUSED(errorMessage)

  ROSProject *project = static_cast<ROSProject *>(ProjectExplorer::ProjectTree::currentProject());
  ROSBuildConfiguration *bc = static_cast<ROSBuildConfiguration *>(project->activeTarget()->activeBuildConfiguration());

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

  cmd += QString::fromLatin1(" --rosdistro \"%1\"").arg(bc->rosDistribution());

  // create package using ros command catkin_create_pkg
  Utils::FileName packagePath = Utils::FileName::fromString(m_wizard->packagePath());
  catkin_create_pkg->setWorkingDirectory(packagePath.toString());

  ROSUtils::sourceROS(catkin_create_pkg, bc->rosDistribution());
  catkin_create_pkg->start(QLatin1String("bash"), QStringList() << QLatin1String("-c") << cmd);
  catkin_create_pkg->waitForFinished();
  bool success;
  if (catkin_create_pkg->exitStatus() != QProcess::CrashExit)
  {
    success = true;
  }
  else
  {
    qDebug() << "Faild to create catkin package.";
    success = false;
  }

  delete catkin_create_pkg;
  return success;
}

bool ROSPackageWizard::postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l,
                                             QString *errorMessage) const
{
    Q_UNUSED(w)
    Q_UNUSED(l)
    Q_UNUSED(errorMessage)

    return true;
}

} // namespace Internal
} // namespace ROSProjectManager
