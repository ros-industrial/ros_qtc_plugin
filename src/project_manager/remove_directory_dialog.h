#ifndef REMOVE_DIRECTORY_DIALOG_H
#define REMOVE_DIRECTORY_DIALOG_H

#include <coreplugin/core_global.h>

#include <QDialog>

namespace Core {
namespace Ui { class RemoveDirectoryDialog;}

/**
 * @brief This a remove directory dialog for deleting directories.
 */
class CORE_EXPORT RemoveDirectoryDialog : public QDialog
{
  Q_OBJECT
public:
  explicit RemoveDirectoryDialog(const QString &filePath, QWidget *parent = 0);
  virtual ~RemoveDirectoryDialog();

  /**
   * @brief Set whether to display the checkbox allowing the user to permanently delete the directory/
   * @param visible a bool.
   */
  void setDeleteDirectoryVisible(bool visible);

  /**
   * @brief Get the status of the permanent delete checkbox.
   * @return a bool.
   */
  bool isDeleteDirectoryChecked() const;

private:
  Ui::RemoveDirectoryDialog *m_ui; /**< The remove directory Ui */

};

}

#endif // REMOVE_DIRECTORY_DIALOG_H
