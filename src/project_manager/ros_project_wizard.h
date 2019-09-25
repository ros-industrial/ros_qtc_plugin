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
#ifndef ROSPROJECTWIZARD_H
#define ROSPROJECTWIZARD_H

#include "ros_utils.h"

#include <coreplugin/basefilewizard.h>
#include <coreplugin/basefilewizardfactory.h>
#include <utils/wizard.h>
#include <utils/wizardpage.h>
#include <utils/fileutils.h>

#include <QProcess>

namespace ROSProjectManager {
namespace Internal {
class ROSImportWizardPage;
class ROSImportWizardPagePrivate;
namespace Ui{ class ROSImportWizardPage;}
class ROSProjectWizardDialog : public Core::BaseFileWizard
{
    Q_OBJECT

public:
    explicit ROSProjectWizardDialog(const Core::BaseFileWizardFactory *factory, QWidget *parent = 0);

    QString projectName() const;
    Utils::FilePath distribution() const;
    ROSUtils::BuildSystem buildSystem() const;
    Utils::FilePath workspaceDirectory() const;

    ROSImportWizardPage *m_firstPage;
};

class ROSImportWizardPage : public Utils::WizardPage
{
    Q_OBJECT

public:
    explicit ROSImportWizardPage(QWidget *parent = 0);
    ~ROSImportWizardPage();

    bool isComplete() const;

    bool forceFirstCapitalLetterForFileName() const;
    void setForceFirstCapitalLetterForFileName(bool b);

    QString projectName() const;
    Utils::FilePath distribution() const;
    ROSUtils::BuildSystem buildSystem() const;
    Utils::FilePath workspaceDirectory() const;

    // Validate a base name entry field (potentially containing extension)
    static bool validateBaseName(const QString &name, QString *errorMessage = 0);

signals:
    void activated();

private slots:
    void slotProjectNameValidChanged();
    void slotProjectPathValidChanged();
    void slotActivated();
//    void slotUpdateStdError();
//    void slotUpdateStdText();

private:
    ROSImportWizardPagePrivate *d;

    void validChangedHelper();
};


class ROSProjectWizard : public Core::BaseFileWizardFactory
{
    Q_OBJECT

public:
    ROSProjectWizard();

protected:
    Core::BaseFileWizard *create(QWidget *parent, const Core::WizardDialogParameters &parameters) const override;
    Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const override;
    bool postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l,
                           QString *errorMessage) const override;
};

} // namespace Internal
} // namespace ROSProjectManager
#endif
