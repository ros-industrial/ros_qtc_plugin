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
#include "ros_project_wizard.h"
#include "ui_ros_import_wizard_page.h"
#include "ros_project_constants.h"
#include "ros_project_plugin.h"
#include "ros_settings_page.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/customwizard/customwizard.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/editorconfiguration.h>
#include <projectexplorer/project.h>

#include <cppeditor/cppcodestylepreferences.h>
#include <cppeditor/cppeditorconstants.h>

#include <texteditor/icodestylepreferences.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/codestylepool.h>

#include <utils/filewizardpage.h>
#include <utils/wizard.h>
#include <utils/icon.h>

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
#include <QBitmap>


namespace ROSProjectManager {
namespace Internal {

//////////////////////////////////////////////////////////////////////////////
//
// ROSProjectWizardDialog
//
//////////////////////////////////////////////////////////////////////////////

ROSProjectWizardDialog::ROSProjectWizardDialog(const Core::BaseFileWizardFactory *factory, QWidget *parent) :
    Core::BaseFileWizard(factory, QVariantMap(), parent)
{
    setWindowTitle(tr("Creates New ROS Project"));

    // first page
    m_firstPage = new ROSImportWizardPage;
    m_firstPage->setTitle(tr("Project Name and Location"));

    addPage(m_firstPage);
}

QString ROSProjectWizardDialog::projectName() const
{
    return m_firstPage->projectName();
}

Utils::FilePath ROSProjectWizardDialog::distribution() const
{
    return m_firstPage->distribution();
}

ROSUtils::BuildSystem ROSProjectWizardDialog::buildSystem() const
{
    return m_firstPage->buildSystem();
}

Utils::FilePath ROSProjectWizardDialog::workspaceDirectory() const
{
    return m_firstPage->workspaceDirectory();
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
    d(new ROSImportWizardPagePrivate)
{
    d->m_ui.setupUi(this);
    QStringList dist_list;
    dist_list.append(QString{});
    for(auto entry : ROSUtils::installedDistributions())
    {
        dist_list.append(entry.toString());
    }
    d->m_ui.distributionComboBox->addItems(dist_list);

    QSharedPointer<ROSSettings> ros_settings = ROSProjectPlugin::instance()->settings();
    int index = d->m_ui.distributionComboBox->findText(ros_settings->default_distribution, Qt::MatchExactly);
    d->m_ui.distributionComboBox->setCurrentIndex(index);

    d->m_ui.buildSystemComboBox->setCurrentIndex(static_cast<int>(ros_settings->default_build_system));

    connect(d->m_ui.pathChooser, &Utils::PathChooser::validChanged,
            this, &ROSImportWizardPage::slotProjectPathValidChanged);
    connect(d->m_ui.nameLineEdit, &Utils::FancyLineEdit::validChanged,
            this, &ROSImportWizardPage::slotProjectNameValidChanged);

    connect(d->m_ui.pathChooser, &Utils::PathChooser::returnPressed,
            this, &ROSImportWizardPage::slotActivated);
    connect(d->m_ui.nameLineEdit, &Utils::FancyLineEdit::validReturnPressed,
            this, &ROSImportWizardPage::slotActivated);

}

ROSImportWizardPage::~ROSImportWizardPage()
{
    delete d;
}

QString ROSImportWizardPage::projectName() const
{
    return d->m_ui.nameLineEdit->text();
}

Utils::FilePath ROSImportWizardPage::distribution() const
{
    return Utils::FilePath::fromString(d->m_ui.distributionComboBox->currentText());
}

ROSUtils::BuildSystem ROSImportWizardPage::buildSystem() const
{
    return static_cast<ROSUtils::BuildSystem>(d->m_ui.buildSystemComboBox->currentIndex());
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

Utils::FilePath ROSImportWizardPage::workspaceDirectory() const
{
  return Utils::FilePath::fromString(d->m_ui.pathChooser->filePath().toString());
}

void ROSImportWizardPage::slotProjectNameValidChanged()
{
  validChangedHelper();
}

void ROSImportWizardPage::slotProjectPathValidChanged()
{
  validChangedHelper();
}


void ROSImportWizardPage::validChangedHelper()
{
    const bool newComplete = d->m_ui.pathChooser->isValid() && d->m_ui.nameLineEdit->isValid();
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
    setSupportedProjectTypes({ Constants::ROS_PROJECT_ID });
    setIcon(Utils::Icon({{":rosproject/ros_icon.png", Utils::Theme::PanelTextColorDark}}, Utils::Icon::Tint).icon());
    setDisplayName(tr("ROS Workspace"));
    setId("Z.ROSIndustrial");
    setDescription(tr("Create ROS Workspace"));
    setCategory(QLatin1String(ProjectExplorer::Constants::QT_PROJECT_WIZARD_CATEGORY));
    setDisplayCategory(QLatin1String(ProjectExplorer::Constants::QT_PROJECT_WIZARD_CATEGORY_DISPLAY));
    setFlags(Core::IWizardFactory::PlatformIndependent);
}

Core::BaseFileWizard *ROSProjectWizard::create(QWidget *parent, const Core::WizardDialogParameters &parameters) const
{
    Q_UNUSED(parameters);
    ROSProjectWizardDialog *wizard = new ROSProjectWizardDialog(this, parent);

    for (QWizardPage *p : wizard->extensionPages())
        wizard->addPage(p);

    return wizard;
}

Core::GeneratedFiles ROSProjectWizard::generateFiles(const QWizard *w, QString *errorMessage) const
{
    Q_UNUSED(errorMessage);

    const ROSProjectWizardDialog *wizard = qobject_cast<const ROSProjectWizardDialog *>(w);
    const QDir wsDir(wizard->workspaceDirectory().toString());

    const QString projectName = wizard->projectName();
    const Utils::FilePath workspaceFileName = Utils::FilePath::fromFileInfo(QFileInfo(wsDir, projectName + QLatin1String(".workspace")));
    ROSUtils::ROSProjectFileContent projectFileContent;
    projectFileContent.defaultBuildSystem = wizard->buildSystem();
    projectFileContent.distribution = wizard->distribution();

    ROSUtils::WorkspaceInfo workspaceInfo = ROSUtils::getWorkspaceInfo(wizard->workspaceDirectory(), projectFileContent.defaultBuildSystem, wizard->distribution());

    Core::GeneratedFile generatedWorkspaceFile(workspaceFileName);
    QString content;
    QXmlStreamWriter workspaceXml(&content);
    ROSUtils::generateQtCreatorWorkspaceFile(workspaceXml, projectFileContent);
    generatedWorkspaceFile.setContents(content);
    generatedWorkspaceFile.setAttributes(Core::GeneratedFile::OpenProjectAttribute);

    Core::GeneratedFiles files;
    files.append(generatedWorkspaceFile);

    return files;
}

bool ROSProjectWizard::postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage) const
{
    Q_UNUSED(w);

    bool success = ProjectExplorer::CustomProjectWizard::postGenerateOpen(l, errorMessage);

    ProjectExplorer::Project *project = ProjectExplorer::ProjectTree::currentProject();
    if (!project)
        return success;

    // Set the Cpp code style for the project.
    QSharedPointer<ROSSettings> ros_settings = ROSProjectPlugin::instance()->settings();
    TextEditor::CodeStylePool *code_style_pool = TextEditor::TextEditorSettings::codeStylePool(CppEditor::Constants::CPP_SETTINGS_ID);

    for (const auto& code_style : code_style_pool->codeStyles()) {
        if (ros_settings->default_code_style == code_style->displayName()) {
            ProjectExplorer::EditorConfiguration *editorConfiguration = project->editorConfiguration();
            TextEditor::ICodeStylePreferences *codeStylePreferences = editorConfiguration->codeStyle(CppEditor::Constants::CPP_SETTINGS_ID);
            codeStylePreferences->setCurrentDelegate(code_style->id());
            break;
        }
    }

    return success;
}

} // namespace Internal
} // namespace ROSProjectManager
