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
