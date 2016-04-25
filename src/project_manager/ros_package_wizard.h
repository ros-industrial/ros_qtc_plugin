#ifndef ROSPACKAGEWIZARD_H
#define ROSPACKAGEWIZARD_H

#include <coreplugin/basefilewizard.h>
#include <coreplugin/basefilewizardfactory.h>
#include <utils/wizard.h>
#include <utils/wizardpage.h>
#include <utils/fileutils.h>
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

     /**
     * @brief Set the default path for the wizard path chooser.
     * @param path a QString representing the default path.
     */
    void setPackagePath(const QString &path);

    /**
     * @brief Gets the package name from the wizard.
     * @return a QString.
     */
    QString packageName() const;

    /**
     * @brief Get the path to where the package will be created from the wizard.
     * @return a QString.
     */
    QString packagePath() const;

    /**
     * @brief Get the version number of the package from the wizard.
     * @return a QString.
     */
    QString version() const;

    /**
     * @brief Get the license type of the package from the wizard.
     * @return a QString.
     */
    QString licenses() const;

    /**
     * @brief Get the description of the package from the wizard.
     * @return a QString.
     */
    QString description() const;

    /**
     * @brief Get the authors of the package from the wizard.
     * @return a QStringList.
     */
    QStringList authors() const;

    /**
     * @brief Get the maintainers of the package from the wizard.
     * @return a QStringList.
     */
    QStringList maintainers() const;

    /**
     * @brief Get the catkin dependencies of the package from the wizard.
     * @return a QStringList.
     */
    QStringList catkin_dependencies() const;

    /**
     * @brief Get the system dependencies of the package from the wizard.
     * @return a QStringList.
     */
    QStringList system_dependencies() const;

    /**
     * @brief Get the boost components of the package from the wizard.
     * @return a QStringList.
     */
    QStringList boost_components() const;

private:
    ROSPackageWizardDetailsPage *m_detailsPage; /**< Details page of the wizard.*/
};

class ROSPackageWizardDetailsPage : public Utils::WizardPage
{
    Q_OBJECT

public:
    explicit ROSPackageWizardDetailsPage(QWidget *parent = 0);
    ~ROSPackageWizardDetailsPage();

    bool isComplete() const override;

    /**
    * @brief Set the default path for the wizard path chooser.
    * @param path a QString representing the default path.
    */
   void setPackagePath(const QString &path);

   /**
    * @brief Gets the package name from the wizard.
    * @return a QString.
    */
   QString packageName() const;

   /**
    * @brief Get the path to where the package will be created from the wizard.
    * @return a QString.
    */
   QString packagePath() const;

   /**
    * @brief Get the version number of the package from the wizard.
    * @return a QString.
    */
   QString version() const;

   /**
    * @brief Get the license type of the package from the wizard.
    * @return a QString.
    */
   QString licenses() const;

   /**
    * @brief Get the description of the package from the wizard.
    * @return a QString.
    */
   QString description() const;

   /**
    * @brief Get the authors of the package from the wizard.
    * @return a QStringList.
    */
   QStringList authors() const;

   /**
    * @brief Get the maintainers of the package from the wizard.
    * @return a QStringList.
    */
   QStringList maintainers() const;

   /**
    * @brief Get the catkin dependencies of the package from the wizard.
    * @return a QStringList.
    */
   QStringList catkin_dependencies() const;

   /**
    * @brief Get the system dependencies of the package from the wizard.
    * @return a QStringList.
    */
   QStringList system_dependencies() const;

   /**
    * @brief Get the boost components of the package from the wizard.
    * @return a QStringList.
    */
   QStringList boost_components() const;

private slots:

    /**
     * @brief This slot is called anytime the package name is changed.
     *
     * It checks whether the input was valid by calling validChangedHelper().
     */
    void slotPackageNameValidChanged();

    /**
     * @brief This slot is called anytime the package location is changed.
     *
     * It checks whether the input was valid by calling validChangedHelper().
     */
    void slotPackagePathValidChanged();

private:
    /**
     * @brief This processes a QString and return a QStringList.
     *
     * This process a comma or semicolon delimated string into a QStringList
     * @param text a QString to process
     * @return a QStringList
     */
    QStringList processList(const QString &text) const;

    /**
     * @brief This check if all inputs to the wizard are valid.
     */
    void validChangedHelper();

    ROSPackageWizardDetailsPagePrivate *d; /**< Create package details page UI object. */
};

class ROSPackageWizard : public Core::BaseFileWizardFactory
{
    Q_OBJECT

public:
    ROSPackageWizard();

protected:
    Core::BaseFileWizard *create(QWidget *parent, const Core::WizardDialogParameters &parameters) const override;

    Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const override;

    bool writeFiles(const Core::GeneratedFiles &files, QString *errorMessage) const override;

    bool postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l,
                           QString *errorMessage) const override;

private:
    mutable ROSPackageWizardDialog *m_wizard; /**< Create package dialog wizard object. */
};

} // namespace Internal
} // namespace ROSProjectManager
#endif
