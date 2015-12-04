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

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/customwizard/customwizard.h>

#include <utils/filewizardpage.h>
#include <utils/mimetypes/mimedatabase.h>

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
    m_firstPage = new Utils::FileWizardPage;
    m_firstPage->setTitle(tr("Project Name and Location"));
    m_firstPage->setFileNameLabel(tr("Project name:"));
    m_firstPage->setPathLabel(tr("Location:"));
    addPage(m_firstPage);
}

QString ROSProjectWizardDialog::path() const
{
    return m_firstPage->path();
}

QStringList ROSProjectWizardDialog::selectedPaths() const
{
    return QStringList();
}

QStringList ROSProjectWizardDialog::selectedFiles() const
{
    return QStringList();
}

void ROSProjectWizardDialog::setPath(const QString &path)
{
    m_firstPage->setPath(path);
}

QString ROSProjectWizardDialog::projectName() const
{
    return m_firstPage->fileName();
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

    wizard->setPath(parameters.defaultPath());

    foreach (QWizardPage *p, wizard->extensionPages())
        wizard->addPage(p);

    return wizard;
}

Core::GeneratedFiles ROSProjectWizard::generateFiles(const QWizard *w,
                                                         QString *errorMessage) const
{
    Q_UNUSED(errorMessage)

    const ROSProjectWizardDialog *wizard = qobject_cast<const ROSProjectWizardDialog *>(w);
    const QString projectPath = wizard->path();
    const QDir wsDir(projectPath);
    const QDir srcDir(projectPath + QLatin1String("/src"));
    const QDir bldDir(projectPath + QLatin1String("/build"));
    const QDir devDir(projectPath + QLatin1String("/devel"));
    const QString projectName = wizard->projectName();
    const QString creatorFileName = QFileInfo(wsDir, projectName + QLatin1String(".ros_creator")).absoluteFilePath();
    const QString filesFileName = QFileInfo(wsDir, projectName + QLatin1String(".ros_files")).absoluteFilePath();
    const QString includesFileName = QFileInfo(wsDir, projectName + QLatin1String(".ros_includes")).absoluteFilePath();
    const QString configFileName = QFileInfo(wsDir, projectName + QLatin1String(".ros_config")).absoluteFilePath();

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

    // Generate CodeBlocks Project File
    QString cmd = QLatin1String(" cmake ") + srcDir.absolutePath() + QLatin1String(" -G \"CodeBlocks - Unix Makefiles\"");
    QProcess runCmake;
    runCmake.setWorkingDirectory(bldDir.absolutePath());
    runCmake.start(QLatin1String("bash"), QStringList() << QLatin1String("-c") << cmd);
    runCmake.waitForBytesWritten();
    runCmake.waitForFinished();
    if (runCmake.exitStatus() == QProcess::CrashExit)
    {
      qDebug() << runCmake.errorString();
    }
    else
    {
      qDebug() << runCmake.readAll();
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

    Core::GeneratedFile generatedCreatorFile(creatorFileName);
    generatedCreatorFile.setContents(QLatin1String("[General]\n"));
    generatedCreatorFile.setAttributes(Core::GeneratedFile::OpenProjectAttribute);

    Core::GeneratedFile generatedFilesFile(filesFileName);
    generatedFilesFile.setContents(workspaceFiles.join(QLatin1Char('\n')));

    Core::GeneratedFile generatedIncludesFile(includesFileName);
    generatedIncludesFile.setContents(includePaths.join(QLatin1Char('\n')));

    Core::GeneratedFile generatedConfigFile(configFileName);
    generatedConfigFile.setContents(QLatin1String(ConfigFileTemplate));

    Core::GeneratedFiles files;
    files.append(generatedFilesFile);
    files.append(generatedIncludesFile);
    files.append(generatedConfigFile);
    files.append(generatedCreatorFile);

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
