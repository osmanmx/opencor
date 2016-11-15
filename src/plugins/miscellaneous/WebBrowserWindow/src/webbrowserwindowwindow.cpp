/*******************************************************************************

Copyright The University of Auckland

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************/

//==============================================================================
// Web Browser window
//==============================================================================

#include "borderedwidget.h"
#include "coreguiutils.h"
#include "progressbarwidget.h"
#include "toolbarwidget.h"
#include "webbrowserwindowwidget.h"
#include "webbrowserwindowwindow.h"

//==============================================================================

#include "ui_webbrowserwindowwindow.h"

//==============================================================================

#include <QClipboard>
#include <QLineEdit>
#include <QMenu>
#include <QPrintDialog>
#include <QPrinter>
#include <QPrinterInfo>
#include <QSettings>
#include <QTimer>
#include <QWebHistory>

//==============================================================================

namespace OpenCOR {
namespace WebBrowserWindow {

//==============================================================================

WebBrowserWindowWindow::WebBrowserWindowWindow(QWidget *pParent) :
    Core::WindowWidget(pParent),
    mGui(new Ui::WebBrowserWindowWindow),
    mUrl(QString())
{
    // Set up the GUI

    mGui->setupUi(this);

    // Initially, we cannot go backward/forward

    mGui->actionBack->setEnabled(false);
    mGui->actionForward->setEnabled(false);

    // Create a tool bar widget with a URL value and refresh button
    // Note: the spacer is a little trick to improve the rendering of our tool
    //       bar widget...

    Core::ToolBarWidget *topToolBarWidget = new Core::ToolBarWidget(this);
    QWidget *spacer = new QWidget(topToolBarWidget);

    spacer->setMinimumSize(0, 0);
    spacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    mUrlValue = new QLineEdit(topToolBarWidget);

#ifdef Q_OS_MAC
    mUrlValue->setAttribute(Qt::WA_MacShowFocusRect, false);
    // Note: the above removes the focus border since it messes up the look of
    //       our URL value widget...
#endif

    connect(mUrlValue, SIGNAL(returnPressed()),
            this, SLOT(returnPressed()));

    topToolBarWidget->addWidget(spacer);
    topToolBarWidget->addWidget(mUrlValue);
    topToolBarWidget->addAction(mGui->actionReload);

    mGui->layout->addWidget(topToolBarWidget);

    // Make the URL value our focus proxy

    setFocusProxy(mUrlValue);

    // Create a tool bar widget with different buttons

    Core::ToolBarWidget *bottomToolBarWidget = new Core::ToolBarWidget(this);

    bottomToolBarWidget->addAction(mGui->actionClear);
    bottomToolBarWidget->addSeparator();
    bottomToolBarWidget->addAction(mGui->actionBack);
    bottomToolBarWidget->addAction(mGui->actionForward);
    bottomToolBarWidget->addSeparator();
    bottomToolBarWidget->addAction(mGui->actionCopy);
    bottomToolBarWidget->addSeparator();
    bottomToolBarWidget->addAction(mGui->actionNormalSize);
    bottomToolBarWidget->addSeparator();
    bottomToolBarWidget->addAction(mGui->actionZoomIn);
    bottomToolBarWidget->addAction(mGui->actionZoomOut);
    bottomToolBarWidget->addSeparator();
    bottomToolBarWidget->addAction(mGui->actionPrint);

    mGui->layout->addWidget(Core::newLineWidget(this));
    mGui->layout->addWidget(bottomToolBarWidget);

    // Create and add a web browser window widget

    mWebBrowserWindowWidget = new WebBrowserWindowWidget(this);

    mWebBrowserWindowWidget->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    mGui->layout->addWidget(new Core::BorderedWidget(mWebBrowserWidget,
                                                     true, true, true, true));
#elif defined(Q_OS_MAC)
    mGui->layout->addWidget(new Core::BorderedWidget(mWebBrowserWindowWidget,
                                                     true, false, false, false));
#else
    #error Unsupported platform
#endif

    // Create our (thin) simulation progress widget

    mProgressBarWidget = new Core::ProgressBarWidget(this);

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    Core::BorderedWidget *progressBarBorderedWidget = new Core::BorderedWidget(mProgressBarWidget,
                                                                               false, true, true, true);
#elif defined(Q_OS_MAC)
    Core::BorderedWidget *progressBarBorderedWidget = new Core::BorderedWidget(mProgressBarWidget,
                                                                               true, false, false, false);
#else
    #error Unsupported platform
#endif

    progressBarBorderedWidget->setFixedHeight(4);
    progressBarBorderedWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    mGui->layout->addWidget(progressBarBorderedWidget);

    // Various connections to handle our web browser window widget

    connect(mWebBrowserWindowWidget, SIGNAL(urlChanged(const QUrl &)),
            this, SLOT(urlChanged(const QUrl &)));

    connect(mWebBrowserWindowWidget->pageAction(QWebPage::Back), SIGNAL(changed()),
            this, SLOT(documentChanged()));
    connect(mWebBrowserWindowWidget->pageAction(QWebPage::Forward), SIGNAL(changed()),
            this, SLOT(documentChanged()));

    connect(mWebBrowserWindowWidget->page(), SIGNAL(selectionChanged()),
            this, SLOT(updateActions()));

    connect(mWebBrowserWindowWidget, SIGNAL(loadProgress(int)),
            this, SLOT(loadProgress(const int &)));
    connect(mWebBrowserWindowWidget, SIGNAL(loadFinished(bool)),
            this, SLOT(loadFinished()));

    // Start with a clear web browser widget

    on_actionClear_triggered();

    // Create and populate our context menu

    mContextMenu = new QMenu(this);

    mContextMenu->addAction(mGui->actionClear);
    mContextMenu->addSeparator();
    mContextMenu->addAction(mGui->actionBack);
    mContextMenu->addAction(mGui->actionForward);
    mContextMenu->addSeparator();
    mContextMenu->addAction(mGui->actionCopy);
    mContextMenu->addSeparator();
    mContextMenu->addAction(mGui->actionNormalSize);
    mContextMenu->addSeparator();
    mContextMenu->addAction(mGui->actionZoomIn);
    mContextMenu->addAction(mGui->actionZoomOut);
    mContextMenu->addSeparator();
    mContextMenu->addAction(mGui->actionPrint);
    mContextMenu->addSeparator();
    mContextMenu->addAction(mGui->actionInspect);

    // Some connections to update the enabled state of our various actions

    connect(mWebBrowserWindowWidget, SIGNAL(defaultZoomLevel(const bool &)),
            mGui->actionNormalSize, SLOT(setDisabled(bool)));
    connect(mWebBrowserWindowWidget, SIGNAL(zoomingOutEnabled(const bool &)),
            mGui->actionZoomOut, SLOT(setEnabled(bool)));

    // We want our own context menu for our Web browser window widget (indeed,
    // we don't want the default one, which has the reload menu item and not the
    // other actions that we have in our tool bar widget)

    mWebBrowserWindowWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(mWebBrowserWindowWidget, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showCustomContextMenu()));

    // En/disable the printing action, depending on whether printers are
    // available

    mGui->actionPrint->setEnabled(QPrinterInfo::availablePrinterNames().count());
}

//==============================================================================

WebBrowserWindowWindow::~WebBrowserWindowWindow()
{
    // Delete the GUI

    delete mGui;
}

//==============================================================================

void WebBrowserWindowWindow::retranslateUi()
{
    // Retranslate our whole window

    mGui->retranslateUi(this);
}

//==============================================================================

void WebBrowserWindowWindow::loadSettings(QSettings *pSettings)
{
    // Retrieve the settings of our Web browser window widget

    pSettings->beginGroup(mWebBrowserWindowWidget->objectName());
        mWebBrowserWindowWidget->loadSettings(pSettings);
    pSettings->endGroup();
}

//==============================================================================

void WebBrowserWindowWindow::saveSettings(QSettings *pSettings) const
{
    // Keep track of the settings of our Web browser window widget

    pSettings->beginGroup(mWebBrowserWindowWidget->objectName());
        mWebBrowserWindowWidget->saveSettings(pSettings);
    pSettings->endGroup();
}

//==============================================================================

void WebBrowserWindowWindow::resizeEvent(QResizeEvent *pEvent)
{
    // Default handling of the event

    Core::WindowWidget::resizeEvent(pEvent);
}

//==============================================================================

void WebBrowserWindowWindow::updateActions()
{
    // Update the enabled state of our various actions

    mGui->actionClear->setEnabled(!mUrlValue->text().isEmpty());

    mGui->actionCopy->setEnabled(!mWebBrowserWindowWidget->page()->selectedText().isEmpty());
}

//==============================================================================

static const auto AboutBlank = QStringLiteral("about:blank");

//==============================================================================

void WebBrowserWindowWindow::urlChanged(const QUrl &pUrl)
{
    // The URL has changed, so update our URL value

    QString url = pUrl.toString();

    mUrlValue->setText(url.compare(AboutBlank)?url:QString());

    updateActions();
}

//==============================================================================

void WebBrowserWindowWindow::documentChanged()
{
    // A new page has been loaded, resulting in the previous or next page
    // becoming either available or not

    QAction *action = qobject_cast<QAction *>(sender());

    if (action == mWebBrowserWindowWidget->pageAction(QWebPage::Back))
        mGui->actionBack->setEnabled(action->isEnabled());
    else if (action == mWebBrowserWindowWidget->pageAction(QWebPage::Forward))
        mGui->actionForward->setEnabled(action->isEnabled());
}

//==============================================================================

void WebBrowserWindowWindow::on_actionClear_triggered()
{
    // Go to the home page, i.e. a blank page
    // Note: to set a blank page will make our web page completely white, which
    //       looks better than the default grey background...

    mUrlValue->setText(AboutBlank);

    returnPressed();

    mWebBrowserWindowWidget->history()->clear();
}

//==============================================================================

void WebBrowserWindowWindow::on_actionBack_triggered()
{
    // Go to the previous page

    mWebBrowserWindowWidget->back();
}

//==============================================================================

void WebBrowserWindowWindow::on_actionForward_triggered()
{
    // Go to the next page

    mWebBrowserWindowWidget->forward();
}

//==============================================================================

void WebBrowserWindowWindow::on_actionCopy_triggered()
{
    // Copy the current slection to the clipboard

    QApplication::clipboard()->setText(mWebBrowserWindowWidget->selectedText());
}

//==============================================================================

void WebBrowserWindowWindow::on_actionNormalSize_triggered()
{
    // Reset the zoom level of the page contents

    mWebBrowserWindowWidget->resetZoom();
}

//==============================================================================

void WebBrowserWindowWindow::on_actionZoomIn_triggered()
{
    // Zoom in the page contents

    mWebBrowserWindowWidget->zoomIn();
}

//==============================================================================

void WebBrowserWindowWindow::on_actionZoomOut_triggered()
{
    // Zoom out the page contents

    mWebBrowserWindowWidget->zoomOut();
}

//==============================================================================

void WebBrowserWindowWindow::on_actionPrint_triggered()
{
    // Retrieve the printer with which the user wants to print the page and
    // print it, should s/he still want to go ahead with the printing

    QPrinter printer;
    QPrintDialog printDialog(&printer);

    if (printDialog.exec() == QDialog::Accepted)
        mWebBrowserWindowWidget->print(&printer);
}

//==============================================================================

void WebBrowserWindowWindow::on_actionInspect_triggered()
{
    // Inspect the current page

    mWebBrowserWindowWidget->pageAction(QWebPage::InspectElement)->trigger();
}

//==============================================================================

void WebBrowserWindowWindow::on_actionReload_triggered()
{
    // Reload the URL

    mWebBrowserWindowWidget->reload();
}

//==============================================================================

void WebBrowserWindowWindow::returnPressed()
{
    // Load the URL
    // Note: we keep track of the URL since, in loadProgress(), the initial
    //       value of mWebBrowserWidget->url() will be that of the previous
    //       URL, meaning that we would, in the case of a blank page, start
    //       showing the progress while we clearly shouldn't be...

    mUrl = mUrlValue->text();

    mWebBrowserWindowWidget->load(mUrl);

    updateActions();
}

//==============================================================================

void WebBrowserWindowWindow::showCustomContextMenu() const
{
    // Show our context menu which items match the contents of our tool bar
    // widget

    mContextMenu->exec(QCursor::pos());
}

//==============================================================================

void WebBrowserWindowWindow::loadProgress(const int &pProgress)
{
    // Update the value of our progress bar, but only if we are not dealing with
    // a blank page

    if (mUrl.compare(AboutBlank))
        mProgressBarWidget->setValue(0.01*pProgress);
}

//==============================================================================

void WebBrowserWindowWindow::loadFinished()
{
    // The loading is finished, so reset our progress bar, but only if we are
    // not dealing with a blank page and with a slight delay (it looks better
    // that way)

    enum {
        ResetDelay = 169
    };

    if (mUrl.compare(AboutBlank))
        QTimer::singleShot(ResetDelay, this, SLOT(resetProgressBar()));
}

//==============================================================================

void WebBrowserWindowWindow::resetProgressBar()
{
    // Reset our progress bar

    mProgressBarWidget->setValue(0.0);
}

//==============================================================================

}   // namespace WebBrowserWindow
}   // namespace OpenCOR

//==============================================================================
// End of file
//==============================================================================
