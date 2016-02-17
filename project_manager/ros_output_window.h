#ifndef ROSOUTPUTWINDOW_H
#define ROSOUTPUTWINDOW_H

#include <coreplugin/ioutputpane.h>
#include <coreplugin/outputwindow.h>

namespace ROSProjectManager {
namespace Internal {

class ROSOutputWindow : public Core::IOutputPane
{
  Q_OBJECT

public:
  ROSOutputWindow();
  ~ROSOutputWindow();

  QWidget *outputWidget(QWidget *parent) override;
  QList<QWidget *> toolBarWidgets() const override;
  QString displayName() const override;

  int priorityInStatusBar() const override;

  void clearContents() override;
  void visibilityChanged(bool visible) override;

  void setFocus() override;
  bool hasFocus() const override;
  bool canFocus() const override;

  bool canNavigate() const override;
  bool canNext() const override;
  bool canPrevious() const override;
  void goToNext() override;
  void goToPrev() override;

  void append(const QString &text);

private:
  Core::OutputWindow *m_widget;
};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSOUTPUTWINDOW_H
