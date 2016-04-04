#ifndef ROSPROJECTWIZARD_H
#define ROSPROJECTWIZARD_H

#include <coreplugin/basefilewizard.h>
#include <coreplugin/basefilewizardfactory.h>
#include <utils/wizard.h>
#include <utils/wizardpage.h>
#include <utils/fileutils.h>

#include <QProcess>

namespace Utils {
class FileName;
} // namespace Utils

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

    void setWorkspaceDirectory(const QString &path);

    QString projectName() const;
    QString distribution() const;
    Utils::FileName workspaceDirectory() const;
    Utils::FileName develDirectory() const;
    Utils::FileName sourceDirectory() const;
    Utils::FileName buildDirectory() const;

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

    void setWorkspaceDirectory(const QString &path);

    QString projectName() const;
    QString distribution() const;
    Utils::FileName workspaceDirectory() const;
    Utils::FileName develDirectory() const;
    Utils::FileName sourceDirectory() const;
    Utils::FileName buildDirectory() const;

    // Validate a base name entry field (potentially containing extension)
    static bool validateBaseName(const QString &name, QString *errorMessage = 0);

signals:
    void activated();

private slots:
    void slotProjectNameValidChanged();
    void slotProjectPathValidChanged();
    void slotProjectPathChanged(const QString &path);
    void slotActivated();
    void slotGenerateCodeBlocksProjectFile();
    void slotUpdateStdError();
    void slotUpdateStdText();

private:
    ROSImportWizardPagePrivate *d;
    QProcess *m_runCmake;
    Utils::FileName m_wsDir;
    Utils::FileName m_bldDir;
    Utils::FileName m_srcDir;
    Utils::FileName m_devDir;
    bool m_hasValidCodeBlocksProjectFile;

    void validChangedHelper();
};


class ROSProjectWizard : public Core::BaseFileWizardFactory
{
    Q_OBJECT

public:
    ROSProjectWizard();

protected:
    Core::BaseFileWizard *create(QWidget *parent, const Core::WizardDialogParameters &parameters) const;
    Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const override;
    bool postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l,
                           QString *errorMessage) const override;
};

} // namespace Internal
} // namespace ROSProjectManager
#endif
