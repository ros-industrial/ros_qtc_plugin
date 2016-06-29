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
#include "remove_directory_dialog.h"
#include "ui_remove_directory_dialog.h"
#include <QDir>

using namespace Core;

RemoveDirectoryDialog::RemoveDirectoryDialog(const QString &filePath, QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::RemoveDirectoryDialog)
{
    m_ui->setupUi(this);
    m_ui->directoryNameLabel->setText(QDir::toNativeSeparators(filePath));
    m_ui->deleteDirectoryCheckBox->setChecked(true);

    // TODO
    m_ui->removeVCCheckBox->setVisible(false);
}

RemoveDirectoryDialog::~RemoveDirectoryDialog()
{
    delete m_ui;
}

void RemoveDirectoryDialog::setDeleteDirectoryVisible(bool visible)
{
    m_ui->deleteDirectoryCheckBox->setVisible(visible);
}

bool RemoveDirectoryDialog::isDeleteDirectoryChecked() const
{
    return m_ui->deleteDirectoryCheckBox->isChecked();
}
