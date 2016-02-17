#include "ros_output_window.h"

#include <coreplugin/icontext.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/find/basetextfind.h>

#include <aggregation/aggregate.h>

namespace ROSProjectManager {
namespace Internal {

ROSOutputWindow::ROSOutputWindow()
{
    m_widget = new Core::OutputWindow(Core::Context(Core::Constants::C_GENERAL_OUTPUT_PANE));
    m_widget->setReadOnly(true);
    // Let selected text be colored as if the text edit was editable,
    // otherwise the highlight for searching is too light
    QPalette p = m_widget->palette();
    QColor activeHighlight = p.color(QPalette::Active, QPalette::Highlight);
    p.setColor(QPalette::Highlight, activeHighlight);
    QColor activeHighlightedText = p.color(QPalette::Active, QPalette::HighlightedText);
    p.setColor(QPalette::HighlightedText, activeHighlightedText);
    m_widget->setPalette(p);
    Aggregation::Aggregate *agg = new Aggregation::Aggregate;
    agg->add(m_widget);
    agg->add(new Core::BaseTextFind(m_widget));
}

ROSOutputWindow::~ROSOutputWindow()
{
    delete m_widget;
}

QList<QWidget*> ROSOutputWindow::toolBarWidgets() const
{
  return QList<QWidget *>();
}

bool ROSOutputWindow::hasFocus() const
{
    return m_widget->window()->focusWidget() == m_widget;
}

bool ROSOutputWindow::canFocus() const
{
    return true;
}

void ROSOutputWindow::setFocus()
{
    m_widget->setFocus();
}

void ROSOutputWindow::clearContents()
{
    m_widget->clear();
}

QWidget *ROSOutputWindow::outputWidget(QWidget *parent)
{
    m_widget->setParent(parent);
    return m_widget;
}

QString ROSOutputWindow::displayName() const
{
    return tr("ROS Output");
}

void ROSOutputWindow::visibilityChanged(bool /*b*/)
{
}

void ROSOutputWindow::append(const QString &text)
{
    m_widget->appendText(text);
}

int ROSOutputWindow::priorityInStatusBar() const
{
    return -1;
}

bool ROSOutputWindow::canNext() const
{
    return false;
}

bool ROSOutputWindow::canPrevious() const
{
    return false;
}

void ROSOutputWindow::goToNext()
{

}

void ROSOutputWindow::goToPrev()
{

}

bool ROSOutputWindow::canNavigate() const
{
    return false;
}

} // namespace Internal
} // namespace ROSProjectManager
