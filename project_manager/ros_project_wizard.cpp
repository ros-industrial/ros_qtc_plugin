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

#include "ros_project_wizard.h"
#include "ros_utils.h"
#include "ui_ros_import_wizard_page.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/customwizard/customwizard.h>

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


namespace ROSProjectManager {
namespace Internal {


static const char *const ConfigFileTemplate =
        "// Add predefined macros for your project here. For example:\n"
        "// #define THE_ANSWER 42\n"
        ;

//////////////////////////////////////////////////////////////////////////////
//
// ROSProjectWizardDialog
//
//////////////////////////////////////////////////////////////////////////////

ROSProjectWizardDialog::ROSProjectWizardDialog(const Core::BaseFileWizardFactory *factory,
                                                       QWidget *parent) :
    Core::BaseFileWizard(factory, QVariantMap(), parent)
{
    setWindowTitle(tr("Import Existing ROS Project"));

    // first page
    m_firstPage = new ROSImportWizardPage;
    m_firstPage->setTitle(tr("Project Name and Location"));

    addPage(m_firstPage);
}

QString ROSProjectWizardDialog::projectName() const
{
    return m_firstPage->projectName();
}

Utils::FileName ROSProjectWizardDialog::workspaceDirectory() const
{
    return m_firstPage->workspaceDirectory();
}

void ROSProjectWizardDialog::setWorkspaceDirectory(const QString &path)
{
    m_firstPage->setWorkspaceDirectory(path);
}

Utils::FileName ROSProjectWizardDialog::develDirectory() const
{
    return m_firstPage->develDirectory();
}

Utils::FileName ROSProjectWizardDialog::buildDirectory() const
{
    return m_firstPage->buildDirectory();
}

Utils::FileName ROSProjectWizardDialog::sourceDirectory() const
{
    return m_firstPage->sourceDirectory();
}

//////////////////////////////////////////////////////////////////////////////
//
// ROSFileWizardPage
//
//////////////////////////////////////////////////////////////////////////////
class ROSImportWizardPagePrivate
{
public:
    ROSImportWizardPagePrivate();
    Ui::ROSImportWizardPage m_ui;
    bool m_complete;

};

ROSImportWizardPagePrivate::ROSImportWizardPagePrivate() :
    m_complete(false)
{
}


ROSImportWizardPage::ROSImportWizardPage(QWidget *parent) :
    WizardPage(parent),
    d(new ROSImportWizardPagePrivate),
    m_hasValidCodeBlocksProjectFile(false)
{
    d->m_ui.setupUi(this);
    d->m_ui.distributionComboBox->addItems(ROSUtils::installedDistributions());

    connect(d->m_ui.pathChooser, &Utils::PathChooser::validChanged,
            this, &ROSImportWizardPage::slotProjectPathValidChanged);
    connect(d->m_ui.nameLineEdit, &Utils::FancyLineEdit::validChanged,
            this, &ROSImportWizardPage::slotProjectNameValidChanged);

    connect(d->m_ui.pathChooser, &Utils::PathChooser::pathChanged,
            this, &ROSImportWizardPage::slotProjectPathChanged);

    connect(d->m_ui.generateProjectFileButton, &QPushButton::pressed,
            this, &ROSImportWizardPage::slotGenerateCodeBlocksProjectFile);

    connect(d->m_ui.pathChooser, &Utils::PathChooser::returnPressed,
            this, &ROSImportWizardPage::slotActivated);
    connect(d->m_ui.nameLineEdit, &Utils::FancyLineEdit::validReturnPressed,
            this, &ROSImportWizardPage::slotActivated);

}

ROSImportWizardPage::~ROSImportWizardPage()
{
    delete d;
    delete m_runCmake;
}

QString ROSImportWizardPage::projectName() const
{
    return d->m_ui.nameLineEdit->text();
}

void ROSImportWizardPage::setWorkspaceDirectory(const QString &path)
{
    d->m_ui.pathChooser->setPath(path);
}

bool ROSImportWizardPage::isComplete() const
{
    return d->m_complete;
}

bool ROSImportWizardPage::forceFirstCapitalLetterForFileName() const
{
    return d->m_ui.nameLineEdit->forceFirstCapitalLetter();
}

void ROSImportWizardPage::setForceFirstCapitalLetterForFileName(bool b)
{
    d->m_ui.nameLineEdit->setForceFirstCapitalLetter(b);
}

Utils::FileName ROSImportWizardPage::workspaceDirectory() const
{
  return m_wsDir;
}

Utils::FileName ROSImportWizardPage::buildDirectory() const
{
  return m_bldDir;
}

Utils::FileName ROSImportWizardPage::sourceDirectory() const
{
  return m_srcDir;
}

Utils::FileName ROSImportWizardPage::develDirectory() const
{
  return m_devDir;
}

void ROSImportWizardPage::slotGenerateCodeBlocksProjectFile()
{
  // Generate CodeBlocks Project File
  m_runCmake = new QProcess();
  connect(m_runCmake, SIGNAL(readyReadStandardOutput()),this, SLOT(slotUpdateStdText()));
  connect(m_runCmake, SIGNAL(readyReadStandardError()),this, SLOT(slotUpdateStdError()));
  m_hasValidCodeBlocksProjectFile = false;
  if (ROSUtils::sourceWorkspace(m_runCmake, m_wsDir, d->m_ui.distributionComboBox->currentText()))
  {
    if (ROSUtils::generateCodeBlocksProjectFile(m_runCmake, m_srcDir, m_bldDir))
    {
      m_hasValidCodeBlocksProjectFile = true;
    }
  }
  validChangedHelper();
  slotActivated();
}

void ROSImportWizardPage::slotUpdateStdText()
{
  QByteArray strdata = m_runCmake->readAllStandardOutput();
  d->m_ui.outputTextEdit->append(QString::fromLatin1(strdata.data()).simplified());
  QCoreApplication::processEvents();
}

void ROSImportWizardPage::slotUpdateStdError()
{
  QByteArray strdata = m_runCmake->readAllStandardError();
  d->m_ui.outputTextEdit->append(QString::fromLatin1(strdata.data()).simplified());
  QCoreApplication::processEvents();
}

void ROSImportWizardPage::slotProjectNameValidChanged()
{
  validChangedHelper();
}

void ROSImportWizardPage::slotProjectPathChanged(const QString &path)
{
  Q_UNUSED(path)
  m_hasValidCodeBlocksProjectFile = false;

  if (d->m_ui.pathChooser->isValid())
  {
    m_wsDir = Utils::FileName::fromString(d->m_ui.pathChooser->path());
    m_bldDir = Utils::FileName::fromString(d->m_ui.pathChooser->path() + QLatin1String("/build"));
    m_srcDir = Utils::FileName::fromString(d->m_ui.pathChooser->path() + QLatin1String("/src"));
    m_devDir = Utils::FileName::fromString(d->m_ui.pathChooser->path() + QLatin1String("/devel"));

    d->m_ui.generateProjectFileButton->setEnabled(true);
  }
  else
  {
    d->m_ui.generateProjectFileButton->setEnabled(false);
  }

  validChangedHelper();
}

void ROSImportWizardPage::slotProjectPathValidChanged()
{
  validChangedHelper();
}


void ROSImportWizardPage::validChangedHelper()
{
    const bool newComplete = d->m_ui.pathChooser->isValid() && d->m_ui.nameLineEdit->isValid() && m_hasValidCodeBlocksProjectFile;
    if (newComplete != d->m_complete) {
        d->m_complete = newComplete;
        emit completeChanged();
    }
}

void ROSImportWizardPage::slotActivated()
{
    if (d->m_complete)
        emit activated();
}

bool ROSImportWizardPage::validateBaseName(const QString &name, QString *errorMessage /* = 0*/)
{
    return Utils::FileNameValidatingLineEdit::validateFileName(name, false, errorMessage);
}


//////////////////////////////////////////////////////////////////////////////
//
// ROSProjectWizard
//
//////////////////////////////////////////////////////////////////////////////

ROSProjectWizard::ROSProjectWizard()
{
    setWizardKind(ProjectWizard);
    // TODO do something about the ugliness of standard icons in sizes different than 16, 32, 64, 128
    {
        QPixmap icon(22, 22);
        icon.fill(Qt::transparent);
        QPainter p(&icon);
        p.drawPixmap(3, 3, 16, 16, qApp->style()->standardIcon(QStyle::SP_DirIcon).pixmap(16));
        setIcon(icon);
    }
    setDisplayName(tr("Import ROS Workspace"));
    setId("Z.ROSIndustrial");
    setDescription(tr("Used to import ROS Workspace."));
    setCategory(QLatin1String(ProjectExplorer::Constants::IMPORT_WIZARD_CATEGORY));
    setDisplayCategory(QLatin1String(ProjectExplorer::Constants::IMPORT_WIZARD_CATEGORY_DISPLAY));
    setFlags(Core::IWizardFactory::PlatformIndependent);
}

Core::BaseFileWizard *ROSProjectWizard::create(QWidget *parent,
                                                   const Core::WizardDialogParameters &parameters) const
{
    ROSProjectWizardDialog *wizard = new ROSProjectWizardDialog(this, parent);

    wizard->setWorkspaceDirectory(parameters.defaultPath());

    foreach (QWizardPage *p, wizard->extensionPages())
        wizard->addPage(p);

    return wizard;
}

Core::GeneratedFiles ROSProjectWizard::generateFiles(const QWizard *w,
                                                         QString *errorMessage) const
{
    Q_UNUSED(errorMessage)

    const ROSProjectWizardDialog *wizard = qobject_cast<const ROSProjectWizardDialog *>(w);
    const QDir wsDir(wizard->workspaceDirectory().toString());
    const QDir srcDir(wizard->sourceDirectory().toString());
    const QDir bldDir(wizard->buildDirectory().toString());

    const QString projectName = wizard->projectName();
    //const QString creatorFileName = QFileInfo(wsDir, projectName + QLatin1String(".ros")).absoluteFilePath();
    const QString workspaceFileName = QFileInfo(wsDir, projectName + QLatin1String(".workspace")).absoluteFilePath();

    // Get all file in the workspace source directory
    QStringList workspaceFiles;
    QDirIterator it(srcDir, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
      QDir curDir(it.next());
      QStringList dirFiles = curDir.entryList(QDir::NoDotAndDotDot | QDir::Files);
      curDir.makeAbsolute();
      foreach (const QString &str, dirFiles)
      {
        workspaceFiles.append(curDir.filePath(str));
      }
    }

    // Parse CodeBlocks Project File
    // Need to search for all of the tags <Add directory="include path" />
    QStringList includePaths;
    QXmlStreamReader cbpXml;
    QFile cbpFile(bldDir.filePath(QLatin1String("Project.cbp")));
    if (!cbpFile.open(QFile::ReadOnly | QFile::Text))
    {
      qDebug() << "Error opening CodeBlocks Project File";
    }

    cbpXml.setDevice(&cbpFile);
    cbpXml.readNext();
    while(!cbpXml.atEnd())
    {
      if(cbpXml.isStartElement())
      {
        if(cbpXml.name() == QLatin1String("Add"))
        {
          foreach(const QXmlStreamAttribute &attr, cbpXml.attributes())
          {
            if(attr.name().toString() == QLatin1String("directory"))
            {
              QString attribute_value = attr.value().toString();
              if(!includePaths.contains(attribute_value))
              {
                includePaths.append(attribute_value);
              }
            }
          }
        }
      }
      cbpXml.readNext();
    }

//    Core::GeneratedFile generatedCreatorFile(creatorFileName);
//    generatedCreatorFile.setContents(QLatin1String("[General]\n"));
//    generatedCreatorFile.setAttributes(Core::GeneratedFile::OpenProjectAttribute);

    Core::GeneratedFile generatedWorkspaceFile(workspaceFileName);
    QString content;
    QXmlStreamWriter workspaceXml(&content);
    ROSUtils::gererateQtCreatorWorkspaceFile(workspaceXml, workspaceFiles, includePaths);
    generatedWorkspaceFile.setContents(content);
    generatedWorkspaceFile.setAttributes(Core::GeneratedFile::OpenProjectAttribute);

    Core::GeneratedFiles files;
    files.append(generatedWorkspaceFile);
    //files.append(generatedCreatorFile);

    return files;
}

bool ROSProjectWizard::postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l,
                                             QString *errorMessage) const
{
    Q_UNUSED(w);
    return ProjectExplorer::CustomProjectWizard::postGenerateOpen(l, errorMessage);
}

} // namespace Internal
} // namespace ROSProjectManager
