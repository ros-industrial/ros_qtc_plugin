#include "remove_directory_dialog.h"
#include "ui_remove_director_dialog.h"
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
