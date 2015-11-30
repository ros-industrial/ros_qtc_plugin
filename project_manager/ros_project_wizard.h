#ifndef ROSPROJECTWIZARD_H
#define ROSPROJECTWIZARD_H

#include <coreplugin/basefilewizard.h>
#include <coreplugin/basefilewizardfactory.h>
#include <utils/wizard.h>

namespace Utils { class FileWizardPage; }

namespace ROSProjectManager {
namespace Internal {

class ROSProjectWizardDialog : public Core::BaseFileWizard
{
    Q_OBJECT

public:
    explicit ROSProjectWizardDialog(const Core::BaseFileWizardFactory *factory, QWidget *parent = 0);

    QString path() const;
    void setPath(const QString &path);
    QStringList selectedFiles() const;
    QStringList selectedPaths() const;

    QString projectName() const;

    Utils::FileWizardPage *m_firstPage;
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
