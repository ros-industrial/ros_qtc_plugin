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
#ifndef ROSPACKAGEWIZARD_H
#define ROSPACKAGEWIZARD_H

#include <coreplugin/basefilewizard.h>
#include <coreplugin/basefilewizardfactory.h>
#include <utils/wizard.h>
#include <utils/wizardpage.h>
#include <utils/fileutils.h>
#include <utils/fancylineedit.h>

#include <QProcess>

namespace ROSProjectManager {
namespace Internal {

class ROSPackageWizardDetailsPage;
class ROSPackageWizardDetailsPagePrivate;

namespace Ui{ class ROSPackageWizardDetailsPage;}

class ROSPackageWizardDialog : public Core::BaseFileWizard
{
    Q_OBJECT

public:
    explicit ROSPackageWizardDialog(const Core::BaseFileWizardFactory *factory, QWidget *parent = 0);

    void setPath(const QString &path);
    void setProjectDirectory(const QString &path);

    QString packageName() const;
    QString packagePath() const;
    QString version() const;
    QString licenses() const;
    QString description() const;
    QStringList authors() const;
    QStringList maintainers() const;
    QStringList catkin_dependencies() const;
    QStringList system_dependencies() const;
    QStringList boost_components() const;

private:
    ROSPackageWizardDetailsPage *m_detailsPage;
};

class ROSPackageWizardDetailsPage : public Utils::WizardPage
{
    Q_OBJECT

public:
    explicit ROSPackageWizardDetailsPage(QWidget *parent = 0);
    ~ROSPackageWizardDetailsPage();

    bool isComplete() const override;

    void setPath(const QString &path);
    void setProjectDirectory(const QString &path);

    QString packageName() const;
    QString packagePath() const;
    QString version() const;
    QString licenses() const;
    QString description() const;
    QStringList authors() const;
    QStringList maintainers() const;
    QStringList catkin_dependencies() const;
    QStringList system_dependencies() const;
    QStringList boost_components() const;

signals:
    void activated();

private slots:
    void slotPackageNameValidChanged();
    void slotPackagePathValidChanged();
    void slotActivated();

private:
    bool validateWithValidator(Utils::FancyLineEdit *edit, QString *errorMessage);

    QStringList processList(const QString &text) const;
    ROSPackageWizardDetailsPagePrivate *d;
    void validChangedHelper();
};

class ROSPackageWizard : public Core::BaseFileWizardFactory
{
    Q_OBJECT

public:
    ROSPackageWizard();

protected:
    Core::BaseFileWizard *create(QWidget *parent, const Core::WizardDialogParameters &parameters) const;

    Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const override;

    bool writeFiles(const Core::GeneratedFiles &files, QString *errorMessage) const override;

    bool postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l,
                           QString *errorMessage) const override;

private:
    mutable ROSPackageWizardDialog *m_wizard;
};

} // namespace Internal
} // namespace ROSProjectManager
#endif
