#ifndef Q_DECL_CONSTRUCTOR_DEPRECATED
#  define Q_DECL_CONSTRUCTOR_DEPRECATED Q_DECL_DEPRECATED
#endif

#ifndef TERMINAL_TAB_WIDGET_H
#define TERMINAL_TAB_WIDGET_H

#include <KParts/ReadOnlyPart>
#include <QMainWindow>
#include <KService>
#include <QTabWidget>
#include <QDir>

class TerminalTabWidget : public QWidget
{
  Q_OBJECT
public:
  TerminalTabWidget(QWidget *parent = 0);
  ~TerminalTabWidget();



  int createTerminal(const QString &name, const QDir &workingDirectory = QDir());
  int createTerminal(const QDir &workingDirectory);
  int createTerminal();

  void sendInput(const QString &text);
  void sendInput(const int &id, const QString &text);

  /**
   * @brief Stop active terminal's process by sending a CTRL+C.
   */
  void stopTerminalProcess();

  /**
   * @brief Stope terminal with ID process by sending a CTRL+C
   * @param id
   */
  void stopTerminalProcess(int id);

  int currentTerminalId() const;
  void setCurrentTerminal(int id);

  int currentIndex() const;
  void setCurrentIndex(int index);

  int count() const;

public slots:
  void closeTerminal(const int &id);

signals:
  void terminalClosed(const int &id);
  void terminalCreated(const int &id);

private:
  KService::Ptr m_service;
  QTabWidget *m_tabWidget;
  int m_counter;
  QHash<int, KParts::ReadOnlyPart*> m_terminals;
};

#endif // TERMINAL_TAB_WIDGET_H
