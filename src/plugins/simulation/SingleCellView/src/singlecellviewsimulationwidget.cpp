/*******************************************************************************

Licensed to the OpenCOR team under one or more contributor license agreements.
See the NOTICE.txt file distributed with this work for additional information
regarding copyright ownership. The OpenCOR team licenses this file to you under
the Apache License, Version 2.0 (the "License"); you may not use this file
except in compliance with the License. You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.

*******************************************************************************/

//==============================================================================
// Single cell view simulation widget
//==============================================================================

#include "cellmlfilemanager.h"
#include "combinearchive.h"
#include "combinesupportplugin.h"
#include "coreguiutils.h"
#include "filemanager.h"
#include "progressbarwidget.h"
#include "sedmlfile.h"
#include "sedmlsupportplugin.h"
#include "singlecellviewcontentswidget.h"
#include "singlecellviewgraphpanelswidget.h"
#include "singlecellviewinformationgraphswidget.h"
#include "singlecellviewinformationparameterswidget.h"
#include "singlecellviewinformationsimulationwidget.h"
#include "singlecellviewinformationsolverswidget.h"
#include "singlecellviewinformationwidget.h"
#include "singlecellviewplugin.h"
#include "singlecellviewsimulationwidget.h"
#include "singlecellviewwidget.h"
#include "toolbarwidget.h"
#include "usermessagewidget.h"

//==============================================================================

#include "ui_singlecellviewsimulationwidget.h"

//==============================================================================

#include <QDesktopWidget>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QSplitter>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>

//==============================================================================

#include "qwt_wheel.h"

//==============================================================================

#include "sedmlapidisablewarnings.h"
    #include "sedml/SedTypes.h"
#include "sedmlapienablewarnings.h"

//==============================================================================

namespace OpenCOR {
namespace SingleCellView {

//==============================================================================

SingleCellViewSimulationWidget::SingleCellViewSimulationWidget(SingleCellViewPlugin *pPlugin,
                                                               const QString &pFileName,
                                                               QWidget *pParent) :
    Widget(pParent),
    mGui(new Ui::SingleCellViewSimulationWidget),
    mPlugin(pPlugin),
    mViewWidget(pPlugin->viewWidget()),
    mFileName(pFileName),
    mDataStoreInterfaces(QMap<QAction *, DataStoreInterface *>()),
    mProgress(-1),
    mLockedDevelopmentMode(false),
    mRunActionEnabled(true),
    mGraphPanelsPlots(QMap<SingleCellViewGraphPanelWidget *, SingleCellViewGraphPanelPlotWidget *>()),
    mPlots(SingleCellViewGraphPanelPlotWidgets()),
    mPlotsViewports(QMap<SingleCellViewGraphPanelPlotWidget *, QRectF>()),
    mCanUpdatePlotsForUpdatedGraphs(true),
    mNeedReloadView(false),
    mNeedUpdatePlots(false),
    mOldDataSizes(QMap<SingleCellViewGraphPanelPlotGraph *, qulonglong>())
{
    // Set up and customsise the GUI

    mGui->setupUi(this);

    mGui->actionDevelopmentMode->setEnabled(Core::FileManager::instance()->isReadableAndWritable(mFileName));

    // Create a wheel (and a label to show its value) to specify the delay (in
    // milliseconds) between the output of two data points

    mDelayWidget = new QwtWheel(this);
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    QWidget *delaySpaceWidget = new QWidget(this);
#endif
    mDelayValueWidget = new QLabel(this);

    mDelayWidget->setBorderWidth(0);
    mDelayWidget->setFixedSize(0.07*qApp->desktop()->screenGeometry().width(),
                               0.5*mDelayWidget->height());
    mDelayWidget->setFocusPolicy(Qt::NoFocus);
    mDelayWidget->setRange(0.0, 55.0);
    mDelayWidget->setWheelBorderWidth(0);

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    delaySpaceWidget->setFixedWidth(4);
#endif

    connect(mDelayWidget, SIGNAL(valueChanged(double)),
            this, SLOT(updateDelayValue(const double &)));

    mDelayWidget->setValue(0.0);

    // Create a tool bar widget with different buttons
    // Note: not sure why, but if we create a tool bar widget with ourselves as
    //       a parent then the first time round it will be properly styled, but
    //       not the subsequent times while it works as expected if we use our
    //       own parent as a parent...

    mToolBarWidget = new Core::ToolBarWidget(pParent);

    QToolButton *removeGraphPanelToolButton = new QToolButton(mToolBarWidget);
    QMenu *removeGraphPanelDropDownMenu = new QMenu(removeGraphPanelToolButton);

    removeGraphPanelDropDownMenu->addAction(mGui->actionRemoveCurrentGraphPanel);
    removeGraphPanelDropDownMenu->addAction(mGui->actionRemoveAllGraphPanels);

    removeGraphPanelToolButton->setDefaultAction(mGui->actionRemoveGraphPanel);
    removeGraphPanelToolButton->setMenu(removeGraphPanelDropDownMenu);
    removeGraphPanelToolButton->setPopupMode(QToolButton::MenuButtonPopup);

    QToolButton *sedmlExportToolButton = new QToolButton(mToolBarWidget);
    QMenu *sedmlExportDropDownMenu = new QMenu(sedmlExportToolButton);

    sedmlExportToolButton->setDefaultAction(mGui->actionSedmlExport);
    sedmlExportToolButton->setMenu(sedmlExportDropDownMenu);
    sedmlExportToolButton->setPopupMode(QToolButton::InstantPopup);

    sedmlExportDropDownMenu->addAction(mGui->actionSedmlExportSedmlFile);
    sedmlExportDropDownMenu->addAction(mGui->actionSedmlExportCombineArchive);

    QToolButton *simulationDataExportToolButton = new QToolButton(mToolBarWidget);

    mSimulationDataExportDropDownMenu = new QMenu(simulationDataExportToolButton);

    simulationDataExportToolButton->setDefaultAction(mGui->actionSimulationDataExport);
    simulationDataExportToolButton->setMenu(mSimulationDataExportDropDownMenu);
    simulationDataExportToolButton->setPopupMode(QToolButton::InstantPopup);

    mToolBarWidget->addAction(mGui->actionRunPauseResumeSimulation);
    mToolBarWidget->addAction(mGui->actionStopSimulation);
    mToolBarWidget->addSeparator();
    mToolBarWidget->addAction(mGui->actionResetModelParameters);
    mToolBarWidget->addAction(mGui->actionClearSimulationData);
    mToolBarWidget->addSeparator();
    mToolBarWidget->addWidget(mDelayWidget);
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    mToolBarWidget->addWidget(delaySpaceWidget);
#endif
    mToolBarWidget->addWidget(mDelayValueWidget);
    mToolBarWidget->addSeparator();
    mToolBarWidget->addAction(mGui->actionDevelopmentMode);
    mToolBarWidget->addSeparator();
    mToolBarWidget->addAction(mGui->actionAddGraphPanel);
    mToolBarWidget->addWidget(removeGraphPanelToolButton);
    mToolBarWidget->addSeparator();
    mToolBarWidget->addWidget(sedmlExportToolButton);
    mToolBarWidget->addSeparator();
    mToolBarWidget->addWidget(simulationDataExportToolButton);

    mTopSeparator = Core::newLineWidget(this);

    mGui->layout->addWidget(mToolBarWidget);
    mGui->layout->addWidget(mTopSeparator);

    // Populate our simulation data export drop-down menu with the given data
    // store interfaces

    foreach (DataStoreInterface *dataStoreInterface, pPlugin->dataStoreInterfaces()) {
        QString dataStoreName = dataStoreInterface->dataStoreName();

        QAction *action = mSimulationDataExportDropDownMenu->addAction(dataStoreName+"...");

        action->setToolTip(dataStoreName);

        mDataStoreInterfaces.insert(action, dataStoreInterface);

        connect(action, SIGNAL(triggered()),
                this, SLOT(simulationDataExport()));
    }

    // Update our data store actions

    updateDataStoreActions();

    // Create and add our invalid simulation message widget

    mInvalidModelMessageWidget = new Core::UserMessageWidget(":/oxygen/actions/help-about.png", this);

    mGui->layout->addWidget(mInvalidModelMessageWidget);

    // Create our splitter widget and keep track of its movement
    // Note: we need to keep track of its movement so that saveSettings() can
    //       work fine even when mContentsWidget is not visible (which happens
    //       when a CellML file cannot be run for some reason or another)...

    mSplitterWidget = new QSplitter(Qt::Vertical, this);

    connect(mSplitterWidget, SIGNAL(splitterMoved(int, int)),
            this, SLOT(emitSplitterMoved()));

    // Create our contents widget

    mContentsWidget = new SingleCellViewContentsWidget(pPlugin, this);

    mContentsWidget->setObjectName("Contents");

    // Keep track of changes to some of our simulation and solvers properties

    SingleCellViewInformationWidget *informationWidget = mContentsWidget->informationWidget();

    connect(informationWidget->simulationWidget(), SIGNAL(propertyChanged(Core::Property *)),
            this, SLOT(simulationPropertyChanged(Core::Property *)));
    connect(informationWidget->solversWidget(), SIGNAL(propertyChanged(Core::Property *)),
            this, SLOT(solversPropertyChanged(Core::Property *)));

    // Keep track of whether we can remove graph panels

    SingleCellViewGraphPanelsWidget *graphPanelsWidget = mContentsWidget->graphPanelsWidget();

    connect(graphPanelsWidget, SIGNAL(removeGraphPanelsEnabled(const bool &)),
            mGui->actionRemoveGraphPanel, SLOT(setEnabled(bool)));

    // Keep track of the addition and removal of a graph panel

    SingleCellViewInformationGraphsWidget *graphsWidget = informationWidget->graphsWidget();

    connect(graphPanelsWidget, SIGNAL(graphPanelAdded(SingleCellViewGraphPanelWidget *)),
            graphsWidget, SLOT(initialize(SingleCellViewGraphPanelWidget *)));
    connect(graphPanelsWidget, SIGNAL(graphPanelRemoved(SingleCellViewGraphPanelWidget *)),
            graphsWidget, SLOT(finalize(SingleCellViewGraphPanelWidget *)));

    connect(graphPanelsWidget, SIGNAL(graphPanelAdded(SingleCellViewGraphPanelWidget *)),
            this, SLOT(graphPanelAdded(SingleCellViewGraphPanelWidget *)));
    connect(graphPanelsWidget, SIGNAL(graphPanelRemoved(SingleCellViewGraphPanelWidget *)),
            this, SLOT(graphPanelRemoved(SingleCellViewGraphPanelWidget *)));

    // Keep track of whether a graph panel has been activated

    connect(graphPanelsWidget, SIGNAL(graphPanelActivated(SingleCellViewGraphPanelWidget *)),
            graphsWidget, SLOT(initialize(SingleCellViewGraphPanelWidget *)));

    // Keep track of a graph being required

    connect(informationWidget->parametersWidget(), SIGNAL(graphRequired(CellMLSupport::CellmlFileRuntimeParameter *, CellMLSupport::CellmlFileRuntimeParameter *)),
            this, SLOT(addGraph(CellMLSupport::CellmlFileRuntimeParameter *, CellMLSupport::CellmlFileRuntimeParameter *)));

    // Keep track of the addition and removal of a graph

    connect(graphPanelsWidget, SIGNAL(graphAdded(SingleCellViewGraphPanelPlotWidget *, SingleCellViewGraphPanelPlotGraph *)),
            graphsWidget, SLOT(addGraph(SingleCellViewGraphPanelPlotWidget *, SingleCellViewGraphPanelPlotGraph *)));
    connect(graphPanelsWidget, SIGNAL(graphsRemoved(SingleCellViewGraphPanelPlotWidget *, const SingleCellViewGraphPanelPlotGraphs &)),
            graphsWidget, SLOT(removeGraphs(SingleCellViewGraphPanelPlotWidget *, const SingleCellViewGraphPanelPlotGraphs &)));

    connect(graphPanelsWidget, SIGNAL(graphAdded(SingleCellViewGraphPanelPlotWidget *, SingleCellViewGraphPanelPlotGraph *)),
            this, SLOT(graphAdded(SingleCellViewGraphPanelPlotWidget *, SingleCellViewGraphPanelPlotGraph *)));
    connect(graphPanelsWidget, SIGNAL(graphsRemoved(SingleCellViewGraphPanelPlotWidget *, const SingleCellViewGraphPanelPlotGraphs &)),
            this, SLOT(graphsRemoved(SingleCellViewGraphPanelPlotWidget *, const SingleCellViewGraphPanelPlotGraphs &)));

    // Keep track of the updating of a graph
    // Note: ideally, this would, as for the addition and removal of a graph
    //       (see above), be done through graphPanelsWidget (i.e. a graph would
    //       let people know that it has been updated and the graph panel with
    //       which it is associated would forward the signal to
    //       graphPanelsWidget), but this may result in too many graphsUpdated()
    //       signals being emitted. For example, say that you change the model
    //       with which a graph is associated, then both the X and Y parameters
    //       will get updated, and for each of those updates a graphsUpdated()
    //       signal would be emitted by the graph, hence we would end up with
    //       two signals when only one would have sufficed. Even worse is that
    //       after having updated the X parameter, the graph would have its X
    //       parameter coming from the 'new' model while its Y parameter from
    //       the 'old' model, which could mess things up quite a bit from a
    //       plotting viewpoint. So, instead, the updating is done through our
    //       graphs property editor...

    connect(graphsWidget, SIGNAL(graphsUpdated(SingleCellViewGraphPanelPlotWidget *, const SingleCellViewGraphPanelPlotGraphs &)),
            this, SLOT(graphsUpdated(SingleCellViewGraphPanelPlotWidget *, const SingleCellViewGraphPanelPlotGraphs &)));

    // Create our simulation output widget with a layout on which we put a
    // separating line and our simulation output list view
    // Note: the separating line is because we remove, for aesthetical reasons,
    //       the border of our simulation output list view...

    QWidget *simulationOutputWidget = new QWidget(this);
    QVBoxLayout *simulationOutputLayout= new QVBoxLayout(simulationOutputWidget);

    simulationOutputLayout->setMargin(0);
    simulationOutputLayout->setSpacing(0);

    simulationOutputWidget->setLayout(simulationOutputLayout);

    mOutputWidget = new QTextEdit(this);

    mOutputWidget->setAcceptDrops(false);
    mOutputWidget->setFrameStyle(QFrame::NoFrame);
    mOutputWidget->setReadOnly(true);

    simulationOutputLayout->addWidget(Core::newLineWidget(this));
    simulationOutputLayout->addWidget(mOutputWidget);

    // Populate our splitter and use as much space as possible for it by asking
    // for its height to be that of the desktop's, and then add our splitter to
    // our single cell view widget

    mSplitterWidget->addWidget(mContentsWidget);
    mSplitterWidget->addWidget(simulationOutputWidget);

    mSplitterWidget->setSizes(QIntList() << qApp->desktop()->screenGeometry().height() << 1);

    mGui->layout->addWidget(mSplitterWidget);

    // Create our (thin) simulation progress widget

    mBottomSeparator = Core::newLineWidget(this);
    mProgressBarWidget = new Core::ProgressBarWidget(this);

    mProgressBarWidget->setFixedHeight(3);
    mProgressBarWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    mGui->layout->addWidget(mBottomSeparator);
    mGui->layout->addWidget(mProgressBarWidget);

    // Make our contents widget our focus proxy

    setFocusProxy(mContentsWidget);

    // Create our simulation object and a few connections for it

    mSimulation = new SingleCellViewSimulation(CellMLSupport::CellmlFileManager::instance()->cellmlFile(pFileName)->runtime(),
                                               pPlugin->solverInterfaces());

    connect(mSimulation, SIGNAL(running(const bool &)),
            this, SLOT(simulationRunning(const bool &)));
    connect(mSimulation, SIGNAL(paused()),
            this, SLOT(simulationPaused()));
    connect(mSimulation, SIGNAL(stopped(const qint64 &)),
            this, SLOT(simulationStopped(const qint64 &)));

    connect(mSimulation, SIGNAL(error(const QString &)),
            this, SLOT(simulationError(const QString &)));

    connect(mSimulation->data(), SIGNAL(modified(const bool &)),
            this, SLOT(simulationDataModified(const bool &)));

    // Some further initialisations that are done as part of retranslating the
    // GUI (so that they can be updated when changing languages)

    retranslateUi();
}

//==============================================================================

SingleCellViewSimulationWidget::~SingleCellViewSimulationWidget()
{
    // Delete some internal objects

    delete mSimulation;

    // Delete the GUI

    delete mGui;
}

//==============================================================================

void SingleCellViewSimulationWidget::retranslateUi()
{
    // Retranslate our GUI

    mGui->retranslateUi(this);

    // Retranslate our delay and delay value widgets

    updateDelayValue(mDelayWidget->value());
    // Note: we do this because we want to display the delay using digit
    //       grouping, this respecting the current locale...

    mDelayWidget->setToolTip(tr("Simulation Delay"));
    mDelayValueWidget->setToolTip(mDelayWidget->toolTip());

    mDelayWidget->setStatusTip(tr("Delay between two data points"));
    mDelayValueWidget->setStatusTip(mDelayWidget->statusTip());

    // Retranslate our run/pause action

    updateRunPauseAction(mRunActionEnabled);

    // Retranslate our data store actions

    updateDataStoreActions();

    // Retranslate our invalid model message

    updateInvalidModelMessageWidget();

    // Retranslate our contents widget

    mContentsWidget->retranslateUi();
}

//==============================================================================

void SingleCellViewSimulationWidget::loadSettings(QSettings *pSettings)
{
    // Retrieve the settings of our contents widget

    pSettings->beginGroup(mContentsWidget->objectName());
        mContentsWidget->loadSettings(pSettings, mFileName);
    pSettings->endGroup();
}

//==============================================================================

void SingleCellViewSimulationWidget::saveSettings(QSettings *pSettings) const
{
    // Keep track of the settings of our contents widget

    pSettings->beginGroup(mContentsWidget->objectName());
        mContentsWidget->saveSettings(pSettings, mFileName);
    pSettings->endGroup();
}

//==============================================================================

void SingleCellViewSimulationWidget::updateDataStoreActions()
{
    // Update our data store actions

    foreach (QAction *action, mDataStoreInterfaces.keys())
        action->setStatusTip(tr("Export the simulation data to %1").arg(mDataStoreInterfaces.value(action)->dataStoreName()));
}

//==============================================================================

void SingleCellViewSimulationWidget::output(const QString &pMessage)
{
    // Move to the end of the output
    // Note: this is just in case the user clicked somewhere in the output and
    //       we are therefore not at the end of it anymore...

    mOutputWidget->moveCursor(QTextCursor::End);

    // Output the message and make sure that it's visible

    mOutputWidget->insertHtml(pMessage);
    mOutputWidget->ensureCursorVisible();
}

//==============================================================================

void SingleCellViewSimulationWidget::updateSimulationMode()
{
    // Update our run/pause action

    updateRunPauseAction(!mSimulation->isRunning() || mSimulation->isPaused());

    // Enable/disable our stop action

    bool simulationModeEnabled = mSimulation->isRunning() || mSimulation->isPaused();

    mGui->actionStopSimulation->setEnabled(simulationModeEnabled);

    // Enable or disable our simulation and solvers widgets

    mContentsWidget->informationWidget()->simulationWidget()->setEnabled(!simulationModeEnabled);
    mContentsWidget->informationWidget()->solversWidget()->setEnabled(!simulationModeEnabled);

    // Enable/disable some actions

    mGui->actionClearSimulationData->setEnabled(    mSimulation->results()->size()
                                                && !simulationModeEnabled);
    mGui->actionSimulationDataExport->setEnabled(    mSimulationDataExportDropDownMenu->actions().count()
                                                 &&  mSimulation->results()->size()
                                                 && !simulationModeEnabled);
    mGui->actionSedmlExport->setEnabled(    mSimulation->results()->size()
                                        && !simulationModeEnabled);

    // Give the focus to our focus proxy, in case we leave our simulation mode
    // (so that the user can modify simulation data, etc.)

    if (!simulationModeEnabled)
        focusProxy()->setFocus();
}

//==============================================================================

void SingleCellViewSimulationWidget::updateRunPauseAction(const bool &pRunActionEnabled)
{
    // Update our various actions

    static const QIcon StartIcon = QIcon(":/oxygen/actions/media-playback-start.png");
    static const QIcon PauseIcon = QIcon(":/oxygen/actions/media-playback-pause.png");

    mRunActionEnabled = pRunActionEnabled;

    mGui->actionRunPauseResumeSimulation->setIcon(pRunActionEnabled?StartIcon:PauseIcon);

    bool simulationPaused = mSimulation && mSimulation->isPaused();

    mGui->actionRunPauseResumeSimulation->setIconText(pRunActionEnabled?
                                                          simulationPaused?
                                                              tr("Resume Simulation"):
                                                              tr("Run Simulation"):
                                                          tr("Pause Simulation"));
    mGui->actionRunPauseResumeSimulation->setStatusTip(pRunActionEnabled?
                                                           simulationPaused?
                                                               tr("Resume the simulation"):
                                                               tr("Run the simulation"):
                                                           tr("Pause the simulation"));
    mGui->actionRunPauseResumeSimulation->setText(pRunActionEnabled?
                                                      simulationPaused?
                                                          tr("Resume Simulation"):
                                                          tr("Run Simulation"):
                                                      tr("Pause Simulation"));
    mGui->actionRunPauseResumeSimulation->setToolTip(pRunActionEnabled?
                                                         simulationPaused?
                                                             tr("Resume Simulation"):
                                                             tr("Run Simulation"):
                                                         tr("Pause Simulation"));
}

//==============================================================================

void SingleCellViewSimulationWidget::updateInvalidModelMessageWidget()
{
    // Update our invalid model message

    mInvalidModelMessageWidget->setMessage((mErrorType == InvalidCellmlFile)?
                                               tr("The <strong>%1</strong> view requires a valid CellML file to work...").arg(mPlugin->viewName()):
                                               tr("The <strong>%1</strong> view requires a valid simulation environment to work...").arg(mPlugin->viewName()),
                                           tr("See below for more information."));
}

//==============================================================================

static const auto OutputTab  = QStringLiteral("&nbsp;&nbsp;&nbsp;&nbsp;");
static const auto OutputGood = QStringLiteral(" style=\"color: green;\"");
static const auto OutputInfo = QStringLiteral(" style=\"color: navy;\"");
static const auto OutputBad  = QStringLiteral(" style=\"color: maroon;\"");
static const auto OutputBrLn = QStringLiteral("<br/>\n");

//==============================================================================

void SingleCellViewSimulationWidget::initialize(const bool &pReloadingView)
{
    // Stop keeping track of certain things (so that updatePlot() doesn't get
    // called unnecessarily)
    // Note: see the corresponding code at the end of this method...

    SingleCellViewInformationWidget *informationWidget = mContentsWidget->informationWidget();
    SingleCellViewInformationSimulationWidget *simulationWidget = informationWidget->simulationWidget();

    disconnect(simulationWidget, SIGNAL(propertyChanged(Core::Property *)),
               this, SLOT(simulationPropertyChanged(Core::Property *)));

    // Reset our progress

    mProgress = -1;

    // Update our simulation object, if needed

    CellMLSupport::CellmlFile *cellmlFile = CellMLSupport::CellmlFileManager::instance()->cellmlFile(mFileName);
    CellMLSupport::CellmlFileRuntime *cellmlFileRuntime = cellmlFile->runtime();

    if (pReloadingView)
        mSimulation->update(cellmlFileRuntime);

    // Retrieve our variable of integration, if possible

    bool validCellmlFileRuntime = cellmlFileRuntime && cellmlFileRuntime->isValid();

    CellMLSupport::CellmlFileRuntimeParameter *variableOfIntegration = validCellmlFileRuntime?cellmlFileRuntime->variableOfIntegration():0;

    // Clean up our output, if needed

    if (pReloadingView)
        mOutputWidget->document()->clear();

    // Output some information about our CellML file

    Core::FileManager *fileManagerInstance = Core::FileManager::instance();
    QString fileName = fileManagerInstance->isNew(mFileName)?
                           tr("File")+" #"+QString::number(fileManagerInstance->newIndex(mFileName)):
                           fileManagerInstance->isRemote(mFileName)?
                               fileManagerInstance->url(mFileName):
                               mFileName;
    QString information =  "<strong>"+fileName+"</strong>"+OutputBrLn
                          +OutputTab+"<strong>"+tr("Runtime:")+"</strong> ";

    if (variableOfIntegration) {
        // A variable of integration could be retrieved for our CellML file, so
        // we can also output the model type

        QString additionalInformation = QString();

        if (cellmlFileRuntime->needNlaSolver())
            additionalInformation = " + "+tr("NLA system(s)");

        information += "<span"+OutputGood+">"+tr("valid")+"</span>."+OutputBrLn;
        information += QString(OutputTab+"<strong>"+tr("Model type:")+"</strong> <span"+OutputInfo+">%1%2</span>."+OutputBrLn).arg((cellmlFileRuntime->modelType() == CellMLSupport::CellmlFileRuntime::Ode)?tr("ODE"):tr("DAE"),
                                                                                                                                   additionalInformation);
    } else {
        // We couldn't retrieve a variable a variable of integration, which
        // means that we either don't have a runtime or we have one, but it's
        // not valid or it's valid but we really don't have a variable of
        // integration
        // Note: in the case of a valid runtime and no variable of integration,
        //       we really shouldn't consider the runtime to be valid, hence we
        //       handle this case here...

        mErrorType = InvalidCellmlFile;

        updateInvalidModelMessageWidget();

        information += "<span"+OutputBad+">"+(cellmlFileRuntime?tr("invalid"):tr("none"))+"</span>."+OutputBrLn;

        if (validCellmlFileRuntime) {
            // We have a valid runtime, but no variable of integration, which
            // means that the model doesn't contain any ODE or DAE

            information += OutputTab+"<span"+OutputBad+"><strong>"+tr("Error:")+"</strong> "+tr("the model must have at least one ODE or DAE")+".</span>"+OutputBrLn;
        } else {
            // We don't have a valid runtime, so either there are some problems
            // with the CellML file or its runtime

            foreach (const CellMLSupport::CellmlFileIssue &issue,
                     cellmlFileRuntime?cellmlFileRuntime->issues():cellmlFile->issues()) {
                information += QString(OutputTab+"<span"+OutputBad+"><strong>%1</strong> %2.</span>"+OutputBrLn).arg((issue.type() == CellMLSupport::CellmlFileIssue::Error)?tr("Error:"):tr("Warning:"),
                                                                                                                     issue.message());
            }
        }
    }

    output(information);

    // Enable/disable our run/pause action depending on whether we have a
    // variable of integration

    mGui->actionRunPauseResumeSimulation->setEnabled(variableOfIntegration);

    // Update our simulation mode

    updateSimulationMode();

    // Initialise our contents widget and make sure that we have the required
    // type(s) of solvers

    bool validSimulationEnvironment = false;
    SingleCellViewInformationSolversWidget *solversWidget = informationWidget->solversWidget();

    if (variableOfIntegration) {
        // Show our contents widget in case it got previously hidden
        // Note: indeed, if it was to remain hidden then some initialisations
        //       wouldn't work (e.g. the solvers widget has a property editor,
        //       which all properties need to be removed and if the contents
        //       widget is not visible, then upon repopulating the property
        //       editor, scrollbars will be shown even though they are not
        //       needed)...

        mContentsWidget->setVisible(true);

        // Check whether we have at least one ODE or DAE solver and, if needed,
        // at least one NLA solver

        if (cellmlFileRuntime->needNlaSolver()) {
            if (solversWidget->nlaSolvers().isEmpty()) {
                if (cellmlFileRuntime->needOdeSolver()) {
                    if (solversWidget->odeSolvers().isEmpty()) {
                        simulationError(tr("the model needs both an ODE and an NLA solver, but none are available"),
                                        InvalidSimulationEnvironment);
                    } else {
                        simulationError(tr("the model needs both an ODE and an NLA solver, but no NLA solver is available"),
                                        InvalidSimulationEnvironment);
                    }
                } else {
                    if (solversWidget->daeSolvers().isEmpty()) {
                        simulationError(tr("the model needs both a DAE and an NLA solver, but none are available"),
                                        InvalidSimulationEnvironment);
                    } else {
                        simulationError(tr("the model needs both a DAE and an NLA solver, but no NLA solver is available"),
                                        InvalidSimulationEnvironment);
                    }
                }
            } else if (   cellmlFileRuntime->needOdeSolver()
                       && solversWidget->odeSolvers().isEmpty()) {
                simulationError(tr("the model needs both an ODE and an NLA solver, but no ODE solver is available"),
                                InvalidSimulationEnvironment);
            } else if (   cellmlFileRuntime->needDaeSolver()
                       && solversWidget->daeSolvers().isEmpty()) {
                    simulationError(tr("the model needs both a DAE and an NLA solver, but no DAE solver is available"),
                                    InvalidSimulationEnvironment);
            } else {
                validSimulationEnvironment = true;
            }
        } else if (   cellmlFileRuntime->needOdeSolver()
                   && solversWidget->odeSolvers().isEmpty()) {
            simulationError(tr("the model needs an ODE solver, but none is available"),
                            InvalidSimulationEnvironment);
        } else if (   cellmlFileRuntime->needDaeSolver()
                   && solversWidget->daeSolvers().isEmpty()) {
            simulationError(tr("the model needs a DAE solver, but none is available"),
                            InvalidSimulationEnvironment);
        } else {
            validSimulationEnvironment = true;
        }
    }

    // Show/hide some widgets depending on whether we have a valid simulation
    // environment

    bool prevValidSimulationEnvironment = mInvalidModelMessageWidget->isHidden();

    mToolBarWidget->setVisible(validSimulationEnvironment);
    mTopSeparator->setVisible(validSimulationEnvironment);

    mContentsWidget->setVisible(validSimulationEnvironment);
    mInvalidModelMessageWidget->setVisible(!validSimulationEnvironment);

    mBottomSeparator->setVisible(validSimulationEnvironment);
    mProgressBarWidget->setVisible(validSimulationEnvironment);

    // Make sure that the last output message is visible
    // Note: indeed, to (re)show some widgets (see above) might change the
    //       height of our output widget, messing up the vertical scroll bar a
    //       bit (if visible), resulting in the output being shifted a bit...

    if (prevValidSimulationEnvironment != validSimulationEnvironment) {
        QCoreApplication::processEvents();

        mOutputWidget->ensureCursorVisible();
    }

    // Some additional initialisations in case we have a valid simulation
    // environment

    if (validSimulationEnvironment) {
        // Initialise our GUI's simulation, solvers, graphs and graph panels
        // widgets, but not parameters widget
        // Note #1: this will also initialise some of our simulation data (i.e.
        //          our simulation's starting point and simulation's NLA
        //          solver's properties), which is needed since we want to be
        //          able to reset our simulation below...
        // Note #2: to initialise our graphs widget will result in some graphs
        //          being shown/hidden and, therefore, in graphsUpdated() being
        //          called. Yet, we don't want graphsUpdated() to update our
        //          plots. Indeed, if it did, then all of our plots' axes'
        //          values would be reset while we want to keep the ones we just
        //          retrieved (thus making it possible for the user to have
        //          different views for different files). So, for this to work
        //          we use mCanUpdatePlotsForUpdatedGraphs, and then 'manually'
        //          replot our plots, once we know which graphs are to be
        //          shown/hidden. We could do the initialisation before the
        //          setting of the plots' axes' values, but then we could see
        //          the graphs being plotted twice. Once after the plots' axes'
        //          values have been reset following the call to graphsUpdated()
        //          and another after we update our plots' axes' values. This is
        //          clearly not neat, hence the current solution...
        // Note #3: to initialise our parameters widget now would result in some
        //          default (in the computer science sense, i.e. wrong) values
        //          being visible for a split second before they get properly
        //          updated. So, instead, we initialise whatever needs
        //          initialising (e.g. NLA solver) so that we can safely compute
        //          our model parameters before showing their values...

        simulationWidget->initialize(mSimulation);
        solversWidget->initialize(mSimulation);

        mCanUpdatePlotsForUpdatedGraphs = false;
            informationWidget->graphsWidget()->initialize(mFileName, mSimulation);
        mCanUpdatePlotsForUpdatedGraphs = true;

        mContentsWidget->graphPanelsWidget()->initialize();

        // Reset both the simulation's data and results (well, initialise in the
        // case of its data)

        mSimulation->data()->reset();
        mSimulation->results()->reset(false);

        // Retrieve our simulation and solvers properties since they may have
        // an effect on our parameter values (as well as result in some solver
        // properties being shown/hidden)

        updateSimulationProperties();
        updateSolversProperties();

        // Now, we can safely update our parameters widget since our model
        // parameters have been computed

        informationWidget->parametersWidget()->initialize(mSimulation, pReloadingView);
    }

    // Resume the tracking of certain things
    // Note: see the corresponding code at the beginning of this method...

    connect(simulationWidget, SIGNAL(propertyChanged(Core::Property *)),
            this, SLOT(simulationPropertyChanged(Core::Property *)));
}

//==============================================================================

void SingleCellViewSimulationWidget::finalize()
{
    // Finalize/backup a few things in our GUI's solvers, graphs, parameters and
    // graph panels widgets

    SingleCellViewInformationWidget *informationWidget = mContentsWidget->informationWidget();

    informationWidget->graphsWidget()->finalize(mFileName);
    informationWidget->parametersWidget()->finalize();
}

//==============================================================================

void SingleCellViewSimulationWidget::setSizes(const QIntList &pSizes)
{
    // Set the sizes of our spliter widget, but only if there effectively are
    // some

    if (pSizes.count())
        mSplitterWidget->setSizes(pSizes);
}

//==============================================================================

SingleCellViewContentsWidget * SingleCellViewSimulationWidget::contentsWidget() const
{
    // Return our contents widget

    return mContentsWidget;
}

//==============================================================================

int SingleCellViewSimulationWidget::tabBarPixmapSize() const
{
    // Return the size of a file tab icon

    return style()->pixelMetric(QStyle::PM_TabBarIconSize, 0, this);
}

//==============================================================================

QIcon SingleCellViewSimulationWidget::fileTabIcon() const
{
    // Return a file tab icon that shows the given file's simulation progress

    if (!isVisible() && (mProgress != -1)) {
        // Create an image that shows the progress of our simulation

        QPixmap tabBarPixmap = QPixmap(tabBarPixmapSize(),
                                       mProgressBarWidget->height()+2);
        QPainter tabBarPixmapPainter(&tabBarPixmap);

        tabBarPixmapPainter.setBrush(QBrush(Core::windowColor()));
        tabBarPixmapPainter.setPen(QPen(Core::borderColor()));

        tabBarPixmapPainter.drawRect(0, 0, tabBarPixmap.width()-1, tabBarPixmap.height()-1);
        tabBarPixmapPainter.fillRect(1, 1, mProgress, tabBarPixmap.height()-2,
                                    Core::highlightColor());

        return QIcon(tabBarPixmap);
    } else {
        // No simulation object currently exists for the model, so return a null
        // icon

        static const QIcon NoIcon = QIcon();

        return NoIcon;
    }
}

//==============================================================================

bool SingleCellViewSimulationWidget::saveFile(const QString &pOldFileName,
                                              const QString &pNewFileName)
{
    // Retrieve all the state and constant parameters which value has changed
    // and update our CellML object with their 'new' values, unless they are
    // imported, in which case we let the user know that their 'new' values
    // cannot be saved

    CellMLSupport::CellmlFile *cellmlFile = CellMLSupport::CellmlFileManager::instance()->cellmlFile(pOldFileName);
    ObjRef<iface::cellml_api::CellMLComponentSet> components = cellmlFile->model()->localComponents();
    QMap<Core::Property *, CellMLSupport::CellmlFileRuntimeParameter *> parameters = mContentsWidget->informationWidget()->parametersWidget()->parameters();
    QString importedParameters = QString();

    foreach (Core::Property *property, parameters.keys()) {
        CellMLSupport::CellmlFileRuntimeParameter *parameter = parameters.value(property);

        if (   (parameter->type() == CellMLSupport::CellmlFileRuntimeParameter::State)
            || (parameter->type() == CellMLSupport::CellmlFileRuntimeParameter::Constant)) {
            ObjRef<iface::cellml_api::CellMLComponent> component = components->getComponent(parameter->componentHierarchy().last().toStdWString());
            ObjRef<iface::cellml_api::CellMLVariableSet>  variables = component->variables();
            ObjRef<iface::cellml_api::CellMLVariable> variable = variables->getVariable(property->name().toStdWString());
            ObjRef<iface::cellml_api::CellMLVariable> sourceVariable = variable->sourceVariable();

            if (variable == sourceVariable)
                variable->initialValue(property->value().toStdWString());
            else
                importedParameters += "\n - "+QString::fromStdWString(component->name())+" | "+QString::fromStdWString(variable->name());
        }
    }

    // Now, we can effectively save our given file and let the user know if some
    // parameter values couldn't be saved

    bool res = cellmlFile->save(pNewFileName);

    if (res && !importedParameters.isEmpty()) {
        QMessageBox::information(Core::mainWindow(),
                                 tr("Save File"),
                                 tr("The following parameters are imported and cannot therefore be saved:")+importedParameters,
                                 QMessageBox::Ok);
    }

    return res;
}

//==============================================================================

void SingleCellViewSimulationWidget::fileOpened(const QString &pFileName)
{
    // Let our graphs widget know that the given file has been opened

    mContentsWidget->informationWidget()->graphsWidget()->fileOpened(pFileName);
}

//==============================================================================

void SingleCellViewSimulationWidget::filePermissionsChanged()
{
    // The given file has been un/locked, so enable/disable the development mode
    // and keep track of its checked status or recheck it, as necessary

     if (Core::FileManager::instance()->isReadableAndWritable(mFileName)) {
         mGui->actionDevelopmentMode->setEnabled(true);
         mGui->actionDevelopmentMode->setChecked(mLockedDevelopmentMode);
     } else {
         mLockedDevelopmentMode = mGui->actionDevelopmentMode->isChecked();

         mGui->actionDevelopmentMode->setChecked(false);
         mGui->actionDevelopmentMode->setEnabled(false);
     }
}

//==============================================================================

void SingleCellViewSimulationWidget::fileModified(const QString &pFileName)
{
    // Update our reset action

    mGui->actionResetModelParameters->setEnabled(Core::FileManager::instance()->isModified(pFileName));
}

//==============================================================================

void SingleCellViewSimulationWidget::reloadView()
{
    // Reload ourselves, i.e. finalise and (re)initialise ourselves, meaning
    // that the given file will have effectively been closed and (re)opened
    // Note: we don't want to call fileClosed() between finalize() and
    //       initialize() since it will trigger the description of existing
    //       graphs to be updated, which will result in them being temporarily
    //       shown as invalid even though they may actually be valid (since we
    //       have finalised the simulation)...

    finalize();
    initialize(true);

    fileOpened(mFileName);

    mNeedReloadView = false;
}

//==============================================================================

void SingleCellViewSimulationWidget::fileReloaded(const QString &pFileName)
{
    // The given file has been reloaded, so stop its current simulation

    bool needReloadView = CellMLSupport::CellmlFileManager::instance()->isCellmlFile(pFileName);

    mNeedReloadView = true;

    if (mSimulation->stop()) {
        needReloadView = false;
        // Note: we don't need to reload ourselves since stopping the simulation
        //       will result in the stopped() signal being received and,
        //       therefore, the simulationStopped() slot being called, which is
        //       where we should reload ourselves since we cannot tell how long
        //       the signal/slot mechanism is going to take...
    }

    // Reload ourselves, if needed

    if (needReloadView)
        reloadView();
}

//==============================================================================

void SingleCellViewSimulationWidget::fileRenamed(const QString &pOldFileName,
                                                 const QString &pNewFileName)
{
    // Let our graphs widget know that the given file has been renamed

    mContentsWidget->informationWidget()->graphsWidget()->fileRenamed(pOldFileName, pNewFileName);
}

//==============================================================================

void SingleCellViewSimulationWidget::fileClosed(const QString &pFileName)
{
    // Let our graphs widget know that the given file has been closed

    mContentsWidget->informationWidget()->graphsWidget()->fileClosed(pFileName);
}

//==============================================================================

QString SingleCellViewSimulationWidget::fileName() const
{
    // Return our file name

    return mFileName;
}

//==============================================================================

SingleCellViewSimulation * SingleCellViewSimulationWidget::simulation() const
{
    // Return our simulation

    return mSimulation;
}

//==============================================================================

QVariant SingleCellViewSimulationWidget::value(Core::Property *pProperty) const
{
    switch (pProperty->type()) {
    case Core::Property::Integer:
        return pProperty->integerValue();
    case Core::Property::Double:
        return pProperty->doubleValue();
    case Core::Property::List:
        return pProperty->listValue();
    case Core::Property::Boolean:
        return pProperty->booleanValue();
    default:
        // Not a property type in which we are interested

        return QVariant();
    }
}

//==============================================================================

void SingleCellViewSimulationWidget::on_actionRunPauseResumeSimulation_triggered()
{
    // Run/resume our simulation or pause it

    if (mRunActionEnabled) {
        // Protect ourselves against two successive (and very) quick attempts at
        // trying to run a simulation

        static bool handlingAction = false;

        if (!mSimulation->isPaused()) {
            if (handlingAction || mSimulation->isRunning())
                return;

            handlingAction = true;
        }

        // Finish any editing of our simulation information, and update our
        // simulation and solvers properties before running/resuming it

        mContentsWidget->informationWidget()->finishEditing(mSimulation->isPaused());

        if (!mSimulation->isPaused()) {
            updateSimulationProperties();
            updateSolversProperties();
        }

        // Run or resume our simulation

        if (mSimulation->isPaused()) {
            mSimulation->resume();
        } else {
            // Check that we have enough memory to run our simulation

            bool runSimulation = true;

            double freeMemory = Core::freeMemory();
            double requiredMemory = mSimulation->requiredMemory();

            if (requiredMemory > freeMemory) {
                QMessageBox::warning(Core::mainWindow(), tr("Run Simulation"),
                                     tr("The simulation requires %1 of memory and you have only %2 left.").arg(Core::sizeAsString(requiredMemory), Core::sizeAsString(freeMemory)));
            } else {
                // Theoretically speaking, we have enough memory to run the
                // simulation, so try to allocate all the memory we need for the
                // simulation by resetting its settings

                runSimulation = mSimulation->results()->reset();

                mViewWidget->checkSimulationResults(mFileName, true);
                // Note: this will, among other things, clear our plots...

                // Effectively run our simulation in case we were able to
                // allocate all the memory we need to run the simulation

                if (runSimulation) {
                    mSimulation->run();
                } else {
                    QMessageBox::warning(Core::mainWindow(), tr("Run Simulation"),
                                         tr("We could not allocate the %1 of memory required for the simulation.").arg(Core::sizeAsString(requiredMemory)));
                }
            }

            handlingAction = false;
        }
    } else {
        mSimulation->pause();
    }
}

//==============================================================================

void SingleCellViewSimulationWidget::on_actionStopSimulation_triggered()
{
    // Stop our simulation

    mSimulation->stop();
}

//==============================================================================

void SingleCellViewSimulationWidget::on_actionResetModelParameters_triggered()
{
    // Reset our model parameters

    mSimulation->reset();
}

//==============================================================================

void SingleCellViewSimulationWidget::on_actionClearSimulationData_triggered()
{
    // Clear our simulation data

    mSimulation->results()->reset(false);

    // Update our simulation mode and check for results

    updateSimulationMode();

    mViewWidget->checkSimulationResults(mFileName, true);
}

//==============================================================================

void SingleCellViewSimulationWidget::on_actionDevelopmentMode_triggered()
{
    // The development mode has just been enabled/disabled, so update the
    // modified state of our current file accordingly

    if (!mGui->actionDevelopmentMode->isChecked())
        Core::FileManager::instance()->setModified(mFileName, false);

    checkSimulationDataModified(mSimulation->data()->isModified());
    // Note: to call checkSimulationDataModified() will, in the case the
    //       development mode has just been disabled, ensure that the reset
    //       button is properly enabled/disabled...
}

//==============================================================================

void SingleCellViewSimulationWidget::on_actionAddGraphPanel_triggered()
{
    // Ask our graph panels widget to add a new graph panel

    mContentsWidget->graphPanelsWidget()->addGraphPanel();
}

//==============================================================================

void SingleCellViewSimulationWidget::on_actionRemoveGraphPanel_triggered()
{
    // Default action for our removing of graph panel, i.e. remove the current
    // graph panel

    on_actionRemoveCurrentGraphPanel_triggered();
}

//==============================================================================

void SingleCellViewSimulationWidget::on_actionRemoveCurrentGraphPanel_triggered()
{
    // Ask our graph panels widget to remove the current graph panel

    mContentsWidget->graphPanelsWidget()->removeCurrentGraphPanel();
}

//==============================================================================

void SingleCellViewSimulationWidget::on_actionRemoveAllGraphPanels_triggered()
{
    // Ask our graph panels widget to remove the current graph panel

    mContentsWidget->graphPanelsWidget()->removeAllGraphPanels();
}

//==============================================================================

void SingleCellViewSimulationWidget::addSedmlSimulation(libsedml::SedDocument *pSedmlDocument,
                                                        libsedml::SedModel *pSedmlModel,
                                                        libsedml::SedRepeatedTask *pSedmlRepeatedTask,
                                                        libsedml::SedSimulation *pSedmlSimulation,
                                                        const int &pOrder)
{
    // Create, customise and add an algorithm (i.e. an ODE or DAE solver) to our
    // given SED-ML simulation
    // Note: the algorithm parameters require the use of KiSAO ids, so if none
    //       exists for an algorithm parameter then we set the algorithm
    //       parameter using an annotation...

    libsedml::SedAlgorithm *sedmlAlgorithm = pSedmlSimulation->createAlgorithm();
    SolverInterface *solverInterface = mSimulation->runtime()->needOdeSolver()?
                                           mSimulation->data()->odeSolverInterface():
                                           mSimulation->data()->daeSolverInterface();
    QString solverName = mSimulation->runtime()->needOdeSolver()?
                             mSimulation->data()->odeSolverName():
                             mSimulation->data()->daeSolverName();
    Solver::Solver::Properties solverProperties = mSimulation->runtime()->needOdeSolver()?
                                                      mSimulation->data()->odeSolverProperties():
                                                      mSimulation->data()->daeSolverProperties();
    QString voiSolverProperties = QString();

    sedmlAlgorithm->setKisaoID(solverInterface->kisaoId(solverName).toStdString());

    foreach (const QString &solverProperty, solverProperties.keys()) {
        QString kisaoId = solverInterface->kisaoId(solverProperty);

        if (kisaoId.isEmpty()) {
            voiSolverProperties += QString("<solverProperty id=\"%1\" value=\"%2\"/>").arg(solverProperty,
                                                                                           solverProperties.value(solverProperty).toString());
        } else {
            libsedml::SedAlgorithmParameter *sedmlAlgorithmParameter = sedmlAlgorithm->createAlgorithmParameter();

            sedmlAlgorithmParameter->setKisaoID(kisaoId.toStdString());
            sedmlAlgorithmParameter->setValue(solverProperties.value(solverProperty).toString().toStdString());
        }
    }

    if (!voiSolverProperties.isEmpty()) {
        sedmlAlgorithm->appendAnnotation(QString("<solverProperties xmlns=\"%1\">%2</solverProperties>").arg(SEDMLSupport::OpencorNamespace,
                                                                                                             voiSolverProperties).toStdString());
    }

    // Check whether the simulation required an NLA solver and, if so, let our
    // SED-ML simulation known about it through an annotation (since we cannot
    // have more than one SED-ML algorithm per SED-ML simulation)

    if (mSimulation->runtime()->needNlaSolver()) {
        QString nlaSolverProperties = QString();

        foreach (const QString &solverProperty, mSimulation->data()->nlaSolverProperties().keys()) {
            nlaSolverProperties += QString("<solverProperty id=\"%1\" value=\"%2\"/>").arg(solverProperty,
                                                                                           solverProperties.value(solverProperty).toString());
        }

        pSedmlSimulation->appendAnnotation(QString("<nlaSolver xmlns=\"%1\" name=\"%2\">%3</nlaSolver>").arg(SEDMLSupport::OpencorNamespace,
                                                                                                             mSimulation->data()->nlaSolverName(),
                                                                                                             nlaSolverProperties).toStdString());
    }

    // Create and customise a task for our given SED-ML simulation

    libsedml::SedTask *sedmlTask = pSedmlDocument->createTask();

    sedmlTask->setId(QString("task%1").arg(pOrder).toStdString());
    sedmlTask->setModelReference(pSedmlModel->getId());
    sedmlTask->setSimulationReference(pSedmlSimulation->getId());

    // Create and customise the corresponding sub-task for our given SED-ML
    // simulation

    libsedml::SedSubTask *sedmlSubTask = pSedmlRepeatedTask->createSubTask();

    sedmlSubTask->setTask(sedmlTask->getId());
    sedmlSubTask->setOrder(pOrder);
}

//==============================================================================

void SingleCellViewSimulationWidget::addSedmlVariableTarget(libsedml::SedVariable *pSedmlVariable,
                                                            const QString &pComponent,
                                                            const QString &pVariable)
{
    // Set the target for the given SED-ML variable

    static const QString Target = "/cellml:model/cellml:component[@name='%1']/cellml:variable[@name='%2']";

    QString variable = pVariable;
    int variableDegree = variable.size();

    // Determine the degree of our variable, if any

    variable.replace("'", QString());

    variableDegree -= variable.size();

    // Set the target itself, as we as its degree, if any

    pSedmlVariable->setTarget(Target.arg(pComponent, variable).toStdString());

    if (variableDegree) {
        pSedmlVariable->appendAnnotation(QString("<variableDegree xmlns=\"%1\">%2</variableDegree>").arg(SEDMLSupport::OpencorNamespace,
                                                                                                         QString::number(variableDegree)).toStdString());
    }
}

//==============================================================================

void SingleCellViewSimulationWidget::createSedmlFile(const QString &pFileName,
                                                     const QString &pModelSource)
{
    // Create a SED-ML document and add the CellML namespace to it

    SEDMLSupport::SedmlFile sedmlFile(pFileName, true);
    libsedml::SedDocument *sedmlDocument = sedmlFile.sedmlDocument();
    XMLNamespaces *namespaces = sedmlDocument->getNamespaces();
    CellMLSupport::CellmlFile::Version cellmlVersion = CellMLSupport::CellmlFile::version(mFileName);

    namespaces->add((cellmlVersion == CellMLSupport::CellmlFile::Cellml_1_1)?
                        CellMLSupport::Cellml_1_1_Namespace.toStdString():
                        CellMLSupport::Cellml_1_0_Namespace.toStdString(),
                    "cellml");

    // Create and customise a model

    libsedml::SedModel *sedmlModel = sedmlDocument->createModel();

    sedmlModel->setId("model");
    sedmlModel->setLanguage((cellmlVersion == CellMLSupport::CellmlFile::Cellml_1_1)?
                                SEDMLSupport::Language::Cellml_1_1.toStdString():
                                SEDMLSupport::Language::Cellml_1_0.toStdString());
    sedmlModel->setSource(Core::nativeCanonicalFileName(pModelSource).toStdString());

    // Apply some parameter changes, if any, to our SED-ML model
//---GRY--- TO BE DONE...

    // Create and customise a repeated task containing a uniform time course
    // simulation followed by a one-step simulation, if needed
    // Note: a uniform time course simulation would be enough if we can
    //       retrieve our point interval from the number of points and if we
    //       run a simulation without modifying any of the model parameters,
    //       but this would be a special case while here, using a repeated
    //       task, we can handle everything that we can do using the GUI...

    libsedml::SedRepeatedTask *sedmlRepeatedTask = sedmlDocument->createRepeatedTask();

    sedmlRepeatedTask->setId("repeatedTask");
    sedmlRepeatedTask->setRangeId("once");
    sedmlRepeatedTask->setResetModel(true);

    // Make our SED-ML repeated task non repeatable

    libsedml::SedVectorRange *sedmlVectorRange = sedmlRepeatedTask->createVectorRange();

    sedmlVectorRange->setId("vectorRange");
    sedmlVectorRange->addValue(1);

    // Create and customise a uniform time course simulation

    int simulationNumber = 0;
    double startingPoint = mSimulation->data()->startingPoint();
    double endingPoint = mSimulation->data()->endingPoint();
    double pointInterval = mSimulation->data()->pointInterval();
    int nbOfPoints = ceil((endingPoint-startingPoint)/pointInterval);
    bool needOneStepTask = (endingPoint-startingPoint)/nbOfPoints != pointInterval;

    libsedml::SedUniformTimeCourse *sedmlUniformTimeCourse = sedmlDocument->createUniformTimeCourse();

    ++simulationNumber;

    if (needOneStepTask)
        --nbOfPoints;

    sedmlUniformTimeCourse->setId(QString("simulation%1").arg(simulationNumber).toStdString());
    sedmlUniformTimeCourse->setInitialTime(startingPoint);
    sedmlUniformTimeCourse->setOutputStartTime(startingPoint);
    sedmlUniformTimeCourse->setOutputEndTime(nbOfPoints*pointInterval);
    sedmlUniformTimeCourse->setNumberOfPoints(nbOfPoints);

    addSedmlSimulation(sedmlDocument, sedmlModel, sedmlRepeatedTask,
                       sedmlUniformTimeCourse, simulationNumber);

    // Complete our simulation with a one-step simulation, if needed

    if (needOneStepTask) {
        libsedml::SedOneStep *sedmlOneStep = sedmlDocument->createOneStep();

        ++simulationNumber;

        sedmlOneStep->setId(QString("simulation%1").arg(simulationNumber).toStdString());
        sedmlOneStep->setStep(endingPoint-sedmlUniformTimeCourse->getOutputEndTime());

        addSedmlSimulation(sedmlDocument, sedmlModel, sedmlRepeatedTask,
                           sedmlOneStep, simulationNumber);
    }

    // Retrieve all the graphs that are to be plotted, if any

    QList<Core::Properties> graphsList = QList<Core::Properties>();
    SingleCellViewInformationGraphsWidget *graphsWidget = mContentsWidget->informationWidget()->graphsWidget();

    foreach (SingleCellViewGraphPanelWidget *graphPanel,
             mContentsWidget->graphPanelsWidget()->graphPanels()) {
        Core::Properties graphs = graphsWidget->graphProperties(graphPanel, mFileName);

        if (!graphs.isEmpty())
            graphsList << graphs;
    }

    // Create and customise 2D plot outputs and data generators for all the
    // graphs that are to be plotted, if any

    if (!graphsList.isEmpty()) {
        int graphPlotCounter = 0;

        foreach (Core::Properties graphs, graphsList) {
            ++graphPlotCounter;

            int graphCounter = 0;
            libsedml::SedPlot2D *sedmlPlot2d = sedmlDocument->createPlot2D();

            sedmlPlot2d->setId(QString("plot%1").arg(QString::number(graphPlotCounter)).toStdString());

            foreach (Core::Property *property, graphs) {
                ++graphCounter;

                // Create two data generators for the X and Y parameters of
                // our current graph

                libsedml::SedDataGenerator *sedmlDataGeneratorX = sedmlDocument->createDataGenerator();
                libsedml::SedDataGenerator *sedmlDataGeneratorY = sedmlDocument->createDataGenerator();
                std::string sedmlDataGeneratorIdX = QString("xDataGenerator%1_%2").arg(QString::number(graphPlotCounter),
                                                                                       QString::number(graphCounter)).toStdString();
                std::string sedmlDataGeneratorIdY = QString("yDataGenerator%1_%2").arg(QString::number(graphPlotCounter),
                                                                                       QString::number(graphCounter)).toStdString();

                sedmlDataGeneratorX->setId(sedmlDataGeneratorIdX);
                sedmlDataGeneratorY->setId(sedmlDataGeneratorIdY);

                libsedml::SedVariable *sedmlVariableX = sedmlDataGeneratorX->createVariable();
                libsedml::SedVariable *sedmlVariableY = sedmlDataGeneratorY->createVariable();
                QStringList propertyX = property->properties()[1]->value().split(".");
                QStringList propertyY = property->properties()[2]->value().split(".");

                sedmlVariableX->setId(QString("xVariable%1_%2").arg(QString::number(graphPlotCounter),
                                                                    QString::number(graphCounter)).toStdString());
                sedmlVariableX->setTaskReference(sedmlRepeatedTask->getId());
                addSedmlVariableTarget(sedmlVariableX, propertyX.first(), propertyX.last());

                sedmlVariableY->setId(QString("yVariable%1_%2").arg(QString::number(graphPlotCounter),
                                                                    QString::number(graphCounter)).toStdString());
                sedmlVariableY->setTaskReference(sedmlRepeatedTask->getId());
                addSedmlVariableTarget(sedmlVariableY, propertyY.first(), propertyY.last());

                sedmlDataGeneratorX->setMath(SBML_parseFormula(sedmlVariableX->getId().c_str()));
                sedmlDataGeneratorY->setMath(SBML_parseFormula(sedmlVariableY->getId().c_str()));

                // Create a curve for our current graph

                libsedml::SedCurve *sedmlCurve = sedmlPlot2d->createCurve();

                sedmlCurve->setId(QString("curve%1_%2").arg(QString::number(graphPlotCounter),
                                                            QString::number(graphCounter)).toStdString());

                sedmlCurve->setXDataReference(sedmlDataGeneratorIdX);
                sedmlCurve->setLogX(false);

                sedmlCurve->setYDataReference(sedmlDataGeneratorIdY);
                sedmlCurve->setLogY(false);
            }
        }
    }

    // Our SED-ML document is ready, so write it to our SED-ML file, after
    // having reformatted it

    QDomDocument domDocument;

    domDocument.setContent(QString(libsedml::writeSedMLToString(sedmlDocument)));

    Core::writeTextToFile(pFileName, qDomDocumentToString(domDocument));
}

//==============================================================================

void SingleCellViewSimulationWidget::on_actionSedmlExportSedmlFile_triggered()
{
    // Export ourselves to SED-ML using a SED-ML file, but first get a file name

    Core::FileManager *fileManagerInstance = Core::FileManager::instance();
    bool remoteFile = fileManagerInstance->isRemote(mFileName);
    QString cellmlFileName = remoteFile?fileManagerInstance->url(mFileName):mFileName;
    QString cellmlFileCompleteSuffix = QFileInfo(cellmlFileName).completeSuffix();
    QString sedmlFileName = cellmlFileName;

    if (!cellmlFileCompleteSuffix.isEmpty()) {
        sedmlFileName.replace(QRegularExpression(QRegularExpression::escape(cellmlFileCompleteSuffix)+"$"),
                              SEDMLSupport::SedmlFileExtension);
    } else {
        sedmlFileName += "."+SEDMLSupport::SedmlFileExtension;
    }

    sedmlFileName = Core::getSaveFileName(QObject::tr("Export To SED-ML File"),
                                          sedmlFileName,
                                          Core::fileTypes(mPlugin->sedmlFileTypes()));

    // Create a SED-ML file using the SED-ML file name that has been provided

    if (!sedmlFileName.isEmpty()) {
        QString modelSource = cellmlFileName;

        if (!remoteFile) {
            // We are dealing with a local CellML file, so refer to it
            // relatively to the directory where we are going to save our SED-ML
            // file
            // Note: normally, we would use QFileInfo::canonicalPath(), but this
            //       requires an existing file, so we use QFileInfo::path()
            //       instead...

            QDir sedmlFileDir = QFileInfo(sedmlFileName).path();

            modelSource = sedmlFileDir.relativeFilePath(modelSource);
        }

        createSedmlFile(sedmlFileName, modelSource);
    }
}

//==============================================================================

void SingleCellViewSimulationWidget::on_actionSedmlExportCombineArchive_triggered()
{
    // Export ourselves to SED-ML using a COMBINE archive, but first get a file
    // name

    Core::FileManager *fileManagerInstance = Core::FileManager::instance();
    bool remoteFile = fileManagerInstance->isRemote(mFileName);
    QString cellmlFileName = remoteFile?fileManagerInstance->url(mFileName):mFileName;
    QString cellmlFileCompleteSuffix = QFileInfo(cellmlFileName).completeSuffix();
    QString combineArchiveName = cellmlFileName;

    if (!cellmlFileCompleteSuffix.isEmpty()) {
        combineArchiveName.replace(QRegularExpression(QRegularExpression::escape(cellmlFileCompleteSuffix)+"$"),
                                   COMBINESupport::CombineFileExtension);
    } else {
        combineArchiveName += "."+COMBINESupport::CombineFileExtension;
    }

    combineArchiveName = Core::getSaveFileName(QObject::tr("Export To COMBINE Archive"),
                                               combineArchiveName,
                                               Core::fileTypes(mPlugin->combineFileTypes()));

    // Effectively export ourselves to a COMBINE archive, if a COMBINE archive
    // name has been provided

    if (!combineArchiveName.isEmpty()) {
        // Determine the path that is common to our main and, if any, imported
        // CellML files, as well as get a copy of our imported CellML files,
        // should they be remote ones

#if defined(Q_OS_WIN)
        static const QRegularExpression FileNameRegEx = QRegularExpression("\\\\[^\\\\]*$");
#elif defined(Q_OS_LINUX) || defined(Q_OS_MAC)
        static const QRegularExpression FileNameRegEx = QRegularExpression("/[^/]*$");
#else
    #error Unsupported platform
#endif

        CellMLSupport::CellmlFile *cellmlFile = CellMLSupport::CellmlFileManager::instance()->cellmlFile(mFileName);
        QString commonPath = remoteFile?
                                 QString(cellmlFileName).remove(FileNameRegEx)+"/":
                                 QFileInfo(mFileName).canonicalPath()+QDir::separator();
        QMap<QString, QString> remoteImportedFileNames = QMap<QString, QString>();

        foreach (const QString &importedFileName, cellmlFile->importedFileNames()) {
            // Check for the common path

            QString importedFilePath = remoteFile?
                                           QString(importedFileName).remove(FileNameRegEx)+"/":
                                           QFileInfo(importedFileName).canonicalPath()+QDir::separator();

            for (int i = 0, iMax = qMin(commonPath.length(), importedFilePath.length()); i < iMax; ++i) {
                if (commonPath[i] != importedFilePath[i]) {
                    commonPath = commonPath.left(i);

                    break;
                }
            }

            commonPath = commonPath.left(qMin(commonPath.length(), importedFilePath.length()));

            // Get a copy of the imported CellML file, if it is a remote one,
            // and keep track of it

            if (remoteFile) {
                QString localImportedFileName = Core::temporaryFileName();

                Core::writeTextToFile(localImportedFileName,
                                      cellmlFile->importedFileContents(importedFileName));

                remoteImportedFileNames.insert(importedFileName, localImportedFileName);
            }
        }

        // Determine the location of our main CellML file

        QString modelSource = remoteFile?
                                  QString(cellmlFileName).remove(commonPath):
                                  QString(mFileName).remove(Core::nativeCanonicalDirName(commonPath)+QDir::separator());

        // Create a copy of the SED-ML file that will be the master file in our
        // COMBINE archive

        QString sedmlFileName = Core::temporaryFileName();

        createSedmlFile(sedmlFileName, modelSource);

        // Create our COMBINE archive after having added all our files to it

        COMBINESupport::CombineArchive combineArchive(combineArchiveName);
        QFileInfo combineArchiveInfo = QFileInfo(combineArchiveName);
        QString sedmlFileLocation = combineArchiveInfo.fileName();

        sedmlFileLocation.replace(QRegularExpression(QRegularExpression::escape(combineArchiveInfo.completeSuffix())+"$"),
                                  SEDMLSupport::SedmlFileExtension);

        combineArchive.addFile(sedmlFileName, sedmlFileLocation,
                               COMBINESupport::CombineArchiveFile::Sedml, true);

        combineArchive.addFile(mFileName, modelSource,
                               (CellMLSupport::CellmlFile::version(cellmlFile) == CellMLSupport::CellmlFile::Cellml_1_1)?
                                   COMBINESupport::CombineArchiveFile::Cellml_1_1:
                                   COMBINESupport::CombineArchiveFile::Cellml_1_0);

        foreach (const QString &importedFileName, cellmlFile->importedFileNames()) {
            combineArchive.addFile(remoteFile?
                                       remoteImportedFileNames.value(importedFileName):
                                       importedFileName,
                                   QString(importedFileName).remove(commonPath),
                                   COMBINESupport::CombineArchiveFile::Cellml);
        }

        combineArchive.save();

        // Remove the local copy of our remote imported CellML files, if any

        foreach (const QString &localImportedFileName, remoteImportedFileNames.values())
            QFile::remove(localImportedFileName);
    }
}

//==============================================================================

void SingleCellViewSimulationWidget::updateSimulationProperties(Core::Property *pProperty)
{
    // Update all the properties, or a particular property (if it exists), of
    // our simulation

    SingleCellViewInformationSimulationWidget *simulationWidget = mContentsWidget->informationWidget()->simulationWidget();

    if (!pProperty || (pProperty == simulationWidget->startingPointProperty())) {
        mSimulation->data()->setStartingPoint(simulationWidget->startingPointProperty()->doubleValue());

        if (pProperty)
            return;
    }

    if (!pProperty || (pProperty == simulationWidget->endingPointProperty())) {
        mSimulation->data()->setEndingPoint(simulationWidget->endingPointProperty()->doubleValue());

        if (pProperty)
            return;
    }

    if (!pProperty || (pProperty == simulationWidget->pointIntervalProperty())) {
        mSimulation->data()->setPointInterval(simulationWidget->pointIntervalProperty()->doubleValue());

        if (pProperty)
            return;
    }
}

//==============================================================================

void SingleCellViewSimulationWidget::updateSolversProperties(Core::Property *pProperty)
{
    // Update all of our solver(s) properties (and solvers widget) or a
    // particular solver property (and the corresponding GUI for that solver)

    SingleCellViewInformationSolversWidget *solversWidget = mContentsWidget->informationWidget()->solversWidget();

    // ODE solver properties

    bool needOdeSolverGuiUpdate = false;

    if (solversWidget->odeSolverData()) {
        if (!pProperty || (pProperty == solversWidget->odeSolverData()->solversListProperty())) {
            mSimulation->data()->setOdeSolverName(solversWidget->odeSolverData()->solversListProperty()->value());

            needOdeSolverGuiUpdate = true;
        }

        if (!pProperty || !needOdeSolverGuiUpdate) {
            foreach (Core::Property *property, solversWidget->odeSolverData()->solversProperties().value(mSimulation->data()->odeSolverName())) {
                if (!pProperty || (pProperty == property)) {
                    mSimulation->data()->addOdeSolverProperty(property->id(), value(property));

                    needOdeSolverGuiUpdate = true;

                    if (pProperty)
                        break;
                }
            }
        }
    }

    if (needOdeSolverGuiUpdate) {
        mContentsWidget->informationWidget()->solversWidget()->updateGui(solversWidget->odeSolverData());

        if (pProperty)
            return;
    }

    // DAE solver properties

    bool needDaeSolverGuiUpdate = false;

    if (solversWidget->daeSolverData()) {
        if (!pProperty || (pProperty == solversWidget->daeSolverData()->solversListProperty())) {
            mSimulation->data()->setDaeSolverName(solversWidget->daeSolverData()->solversListProperty()->value());

            needDaeSolverGuiUpdate = true;
        }

        if (!pProperty || !needDaeSolverGuiUpdate) {
            foreach (Core::Property *property, solversWidget->daeSolverData()->solversProperties().value(mSimulation->data()->daeSolverName())) {
                if (!pProperty || (pProperty == property)) {
                    mSimulation->data()->addDaeSolverProperty(property->id(), value(property));

                    needDaeSolverGuiUpdate = true;

                    if (pProperty)
                        break;
                }
            }
        }
    }

    if (needDaeSolverGuiUpdate) {
        mContentsWidget->informationWidget()->solversWidget()->updateGui(solversWidget->daeSolverData());

        if (pProperty)
            return;
    }

    // NLA solver properties

    bool needNlaSolverGuiUpdate = false;

    if (solversWidget->nlaSolverData()) {
        if (!pProperty || (pProperty == solversWidget->nlaSolverData()->solversListProperty())) {
            mSimulation->data()->setNlaSolverName(solversWidget->nlaSolverData()->solversListProperty()->value());

            needNlaSolverGuiUpdate = true;
        }

        if (!pProperty || !needNlaSolverGuiUpdate) {
            foreach (Core::Property *property, solversWidget->nlaSolverData()->solversProperties().value(mSimulation->data()->nlaSolverName())) {
                if (!pProperty || (pProperty == property)) {
                    mSimulation->data()->addNlaSolverProperty(property->id(), value(property));

                    needNlaSolverGuiUpdate = true;

                    if (pProperty)
                        break;
                }
            }
        }
    }

    if (needNlaSolverGuiUpdate) {
        mContentsWidget->informationWidget()->solversWidget()->updateGui(solversWidget->nlaSolverData());

        if (pProperty)
            return;
    }
}

//==============================================================================

void SingleCellViewSimulationWidget::emitSplitterMoved()
{
    // Let people know that our splitter has been moved

    emit splitterMoved(mSplitterWidget->sizes());
}

//==============================================================================

void SingleCellViewSimulationWidget::simulationDataExport()
{
    // Export our simulation data results

    setEnabled(false);
    mViewWidget->showBusyWidget(this, true);

    DataStoreInterface *dataStoreInterface = mDataStoreInterfaces.value(qobject_cast<QAction *>(sender()));
    DataStore::DataStoreExporter *dataStoreExporter = dataStoreInterface->newDataStoreExporterInstance();

    dataStoreExporter->execute(mFileName, mSimulation->results()->dataStore());

    dataStoreInterface->deleteDataStoreExporterInstance(dataStoreExporter);

    mViewWidget->hideBusyWidget();
    setEnabled(true);
}

//==============================================================================

void SingleCellViewSimulationWidget::updateDelayValue(const double &pDelayValue)
{
    // Update our delay value widget

    int delay = 0;
    int increment = 1;
    int multiple = 10;

    for (int i = 0, iMax = pDelayValue; i < iMax; ++i) {
        delay += increment;

        if (delay % multiple == 0) {
            increment *= 10;
            multiple *= 10;
        }
    }

    mDelayValueWidget->setText(QLocale().toString(delay));

    // Also update our simulation object

    mSimulation->setDelay(delay);
}

//==============================================================================

void SingleCellViewSimulationWidget::simulationRunning(const bool &pIsResuming)
{
    Q_UNUSED(pIsResuming);

    // Our simulation is running, so update our simulation mode and check for
    // results

    updateSimulationMode();

    mViewWidget->checkSimulationResults(mFileName);
}

//==============================================================================

void SingleCellViewSimulationWidget::simulationPaused()
{
    // Our simulation is paused, so update our simulation mode and parameters,
    // and check for results

    updateSimulationMode();

    mContentsWidget->informationWidget()->parametersWidget()->updateParameters(mSimulation->currentPoint(), true);

    mViewWidget->checkSimulationResults(mFileName);
}

//==============================================================================

void SingleCellViewSimulationWidget::simulationStopped(const qint64 &pElapsedTime)
{
    // Output the given elapsed time, if valid

    if (pElapsedTime != -1) {
        QString solversInformation = QString();

        if (!mSimulation->data()->odeSolverName().isEmpty())
            solversInformation += mSimulation->data()->odeSolverName();
        else
            solversInformation += mSimulation->data()->daeSolverName();

        if (!mSimulation->data()->nlaSolverName().isEmpty())
            solversInformation += "+"+mSimulation->data()->nlaSolverName();

        output(QString(OutputTab+"<strong>"+tr("Simulation time:")+"</strong> <span"+OutputInfo+">"+tr("%1 s using %2").arg(QString::number(0.001*pElapsedTime, 'g', 3), solversInformation)+"</span>."+OutputBrLn));
    }

    // Update our parameters and simulation mode

    updateSimulationMode();

    mContentsWidget->informationWidget()->parametersWidget()->updateParameters(mSimulation->currentPoint(), true);

    // Stop keeping track of our simulation progress

    mProgress = -1;

    // Reset our progress bar or tab icon, in case we are not visible, and this
    // with a short delay
    // Note #1: we check that we are not visible in case the user has selected a
    //          file that cannot be handled by us, meaning that our central
    //          widget would show a message rather than us...
    // Note #2: we want a short delay before resetting our progress bar or tab
    //          icon, so that the user can really see when our simulation has
    //          completed, but this is only is we don't need to reload
    //          ourselves. Indeed, if we need to reload ourselves (see
    //          fileReloaded()), we want things to be done as quickly as
    //          possible. Not only that, but we don't want to risk problems with
    //          our simulation being used while it has already been deleted due
    //          to threading issues...

    enum {
        ResetDelay = 169
    };

    if (isVisible()) {
        if (mNeedReloadView)
            resetProgressBar();
        else
            QTimer::singleShot(ResetDelay, this, SLOT(resetProgressBar()));
    } else {
        if (mNeedReloadView)
            resetFileTabIcon();
        else
            QTimer::singleShot(ResetDelay, this, SLOT(resetFileTabIcon()));
    }

    // Reload ourselves, if needed (see fileReloaded())

    if (mNeedReloadView)
        reloadView();
}

//==============================================================================

void SingleCellViewSimulationWidget::resetProgressBar()
{
    // Reset our progress bar

    mProgressBarWidget->setValue(0.0);
}

//==============================================================================

void SingleCellViewSimulationWidget::resetFileTabIcon()
{
    // Stop tracking our simulation progress and let people know that our file
    // tab icon should be reset

    static const QIcon NoIcon = QIcon();

    emit mViewWidget->updateFileTabIcon(mPlugin->viewName(), mFileName, NoIcon);
}

//==============================================================================

void SingleCellViewSimulationWidget::simulationError(const QString &pMessage,
                                                     const ErrorType &pErrorType)
{
    // Output the simulation error

    mErrorType = pErrorType;

    updateInvalidModelMessageWidget();

    output(OutputTab+"<span"+OutputBad+"><strong>"+tr("Error:")+"</strong> "+pMessage+".</span>"+OutputBrLn);
}

//==============================================================================

void SingleCellViewSimulationWidget::checkSimulationDataModified(const bool &pIsModified)
{
    // We are dealing with the current simulation

    if (mGui->actionDevelopmentMode->isChecked())
        Core::FileManager::instance()->setModified(mFileName, pIsModified);
    else
        mGui->actionResetModelParameters->setEnabled(pIsModified);
}

//==============================================================================

void SingleCellViewSimulationWidget::simulationDataModified(const bool &pIsModified)
{
    // Update our modified state

    checkSimulationDataModified(pIsModified);
}

//==============================================================================

void SingleCellViewSimulationWidget::simulationPropertyChanged(Core::Property *pProperty)
{
    // Update our simulation properties, as well as our plots, if it's not the
    // point interval property that has been updated

    updateSimulationProperties(pProperty);

    SingleCellViewInformationSimulationWidget *simulationWidget = mContentsWidget->informationWidget()->simulationWidget();

    if (pProperty != simulationWidget->pointIntervalProperty()) {
        foreach (SingleCellViewGraphPanelPlotWidget *plot, mPlots)
            updatePlot(plot);
    }
}

//==============================================================================

void SingleCellViewSimulationWidget::solversPropertyChanged(Core::Property *pProperty)
{
    // Update our solvers properties

    updateSolversProperties(pProperty);
}

//==============================================================================

void SingleCellViewSimulationWidget::graphPanelAdded(SingleCellViewGraphPanelWidget *pGraphPanel)
{
    // Keep track of the graph panel's plot

    mGraphPanelsPlots.insert(pGraphPanel, pGraphPanel->plot());
}

//==============================================================================

void SingleCellViewSimulationWidget::graphPanelRemoved(SingleCellViewGraphPanelWidget *pGraphPanel)
{
    // A graph panel has been removed, so stop tracking its plot

    mPlots.removeOne(mGraphPanelsPlots.value(pGraphPanel));

    mGraphPanelsPlots.remove(pGraphPanel);
}

//==============================================================================

void SingleCellViewSimulationWidget::addGraph(CellMLSupport::CellmlFileRuntimeParameter *pParameterX,
                                              CellMLSupport::CellmlFileRuntimeParameter *pParameterY)
{
    // Ask the current graph panel to add a new graph for the given parameters

    mContentsWidget->graphPanelsWidget()->activeGraphPanel()->addGraph(new SingleCellViewGraphPanelPlotGraph(pParameterX, pParameterY));
}

//==============================================================================

void SingleCellViewSimulationWidget::graphAdded(SingleCellViewGraphPanelPlotWidget *pPlot,
                                                SingleCellViewGraphPanelPlotGraph *pGraph)
{
    // A new graph has been added, so keep track of it and update its plot
    // Note: updating the plot will, if needed, update the plot's axes and, as
    //       a result, replot the graphs including our new one. On the other
    //       hand, if the plot's axes don't get updated, we need to draw our new
    //       graph...

    updateGraphData(pGraph, mSimulation->results()->size());

    if (!updatePlot(pPlot))
        pPlot->drawGraphFrom(pGraph, 0);

    // Keep track of the plot itself, if needed

    if (!mPlots.contains(pPlot))
        mPlots << pPlot;
}

//==============================================================================

void SingleCellViewSimulationWidget::graphsRemoved(SingleCellViewGraphPanelPlotWidget *pPlot,
                                                   const SingleCellViewGraphPanelPlotGraphs &pGraphs)
{
    Q_UNUSED(pGraphs);

    // One or several graphs have been removed, so update and stop tracking (if
    // needed) their corresponding plot
    // Note: even if the axes' values of the plot haven't changed, we still want
    //       to replot the plot since at least one of its graphs has been
    //       removed...

    updatePlot(pPlot, true);

    if (pPlot->graphs().isEmpty())
        mPlots.removeOne(pPlot);
}

//==============================================================================

void SingleCellViewSimulationWidget::graphsUpdated(SingleCellViewGraphPanelPlotWidget *pPlot,
                                                   const SingleCellViewGraphPanelPlotGraphs &pGraphs)
{
    Q_UNUSED(pPlot);

    // One or several graphs have been updated, so make sure that their
    // corresponding plots are up to date

    SingleCellViewGraphPanelPlotWidgets plots = SingleCellViewGraphPanelPlotWidgets();

    foreach (SingleCellViewGraphPanelPlotGraph *graph, pGraphs) {
        // Show/hide the graph

        graph->setVisible(graph->isValid() && graph->isSelected());

        // Update the graph's data
        // Note: we need to check that we have a simulation since our graph may
        //       indeed refer to a file that has not yet been activated and
        //       therefore doesn't yet have a simulation associated with it...

        SingleCellViewSimulation *simulation = mViewWidget->simulation(graph->fileName());

        updateGraphData(graph, simulation?simulation->results()->size():0);

        // Keep track of the plot that needs to be updated and replotted

        plots << qobject_cast<SingleCellViewGraphPanelPlotWidget *>(graph->plot());
    }

    // Update and replot our various plots, if allowed

    if (mCanUpdatePlotsForUpdatedGraphs) {
        foreach (SingleCellViewGraphPanelPlotWidget *plot, plots) {
            updatePlot(plot, true);
            // Note: even if the axes' values of the plot haven't changed, we
            //       still want to replot the plot since at least one of its
            //       graphs has been updated...
        }
    }
}

//==============================================================================

void SingleCellViewSimulationWidget::checkAxisValue(double &pValue,
                                                    const double &pOrigValue,
                                                    const QList<double> &pTestValues)
{
    // Check whether pOrigValue is equal to one of the values in pTestValues and
    // if so then update pValue with pOrigValue

    foreach (const double &testValue, pTestValues) {
        if (pOrigValue == testValue) {
            pValue = pOrigValue;

            break;
        }
    }
}

//==============================================================================

bool SingleCellViewSimulationWidget::updatePlot(SingleCellViewGraphPanelPlotWidget *pPlot,
                                                const bool &pForceReplot)
{
    // Retrieve the current axes' values or use some default ones, if none are
    // available

    bool hasAxesValues = false;

    double minX = DefMinAxis;
    double maxX = DefMaxAxis;
    double minY = DefMinAxis;
    double maxY = DefMaxAxis;

    QRectF dataRect = pPlot->dataRect();

    if (dataRect != QRectF()) {
        minX = dataRect.left();
        maxX = minX+dataRect.width();
        minY = dataRect.top();
        maxY = minY+dataRect.height();

        hasAxesValues = true;
    }

    // Check all the graphs associated with the given plot and see whether any
    // of them uses the variable of integration as parameter X and/or Y, and if
    // so then asks the plot to use the starting/ending points as the
    // minimum/maximum values for the X and/or Y axes

    bool needInitialisationX = true;
    bool needInitialisationY = true;

    bool canOptimiseAxisX = true;
    bool canOptimiseAxisY = true;

    QList<double> startingPoints = QList<double>();
    QList<double> endingPoints = QList<double>();

    foreach (SingleCellViewGraphPanelPlotGraph *graph, pPlot->graphs()) {
        if (graph->isValid() && graph->isSelected()) {
            SingleCellViewSimulation *simulation = mViewWidget->simulation(graph->fileName());
            double startingPoint = simulation->data()->startingPoint();
            double endingPoint = simulation->data()->endingPoint();

            startingPoints << startingPoint;
            endingPoints << endingPoint;

            if (startingPoint > endingPoint) {
                // The starting point is greater than the ending point, so swap
                // the two of them

                startingPoint = simulation->data()->endingPoint();
                endingPoint = simulation->data()->startingPoint();
            }

            if (graph->parameterX()->type() == CellMLSupport::CellmlFileRuntimeParameter::Voi) {
                if (!hasAxesValues && needInitialisationX) {
                    minX = startingPoint;
                    maxX = endingPoint;

                    needInitialisationX = false;
                } else {
                    minX = qMin(minX, startingPoint);
                    maxX = qMax(maxX, endingPoint);
                }

                canOptimiseAxisX = false;
            }

            if (graph->parameterY()->type() == CellMLSupport::CellmlFileRuntimeParameter::Voi)
            {
                if (!hasAxesValues && needInitialisationY) {
                    minY = startingPoint;
                    maxY = endingPoint;

                    needInitialisationY = false;
                } else {
                    minY = qMin(minY, startingPoint);
                    maxY = qMax(maxY, endingPoint);
                }

                canOptimiseAxisY = false;
            }
        }
    }

    // Optimise our axes' values, if possible

    double origMinX = minX;
    double origMaxX = maxX;
    double origMinY = minY;
    double origMaxY = maxY;

    pPlot->optimiseAxisX(minX, maxX);
    pPlot->optimiseAxisY(minY, maxY);

    if (!canOptimiseAxisX) {
        checkAxisValue(minX, origMinX, startingPoints);
        checkAxisValue(maxX, origMaxX, endingPoints);
    }

    if (!canOptimiseAxisY) {
        checkAxisValue(minY, origMinY, startingPoints);
        checkAxisValue(maxY, origMaxY, endingPoints);
    }

    // Set our axes' values and replot the plot, if needed

    if (pPlot->setAxes(minX, maxX, minY, maxY)) {
        return true;
    } else if (pForceReplot) {
        pPlot->replotNow();

        return true;
    } else {
        return false;
    }
}

//==============================================================================

double * SingleCellViewSimulationWidget::dataPoints(SingleCellViewSimulation *pSimulation,
                                                    CellMLSupport::CellmlFileRuntimeParameter *pParameter) const
{
    // Return the array of data points associated with the given parameter

    switch (pParameter->type()) {
    case CellMLSupport::CellmlFileRuntimeParameter::Constant:
    case CellMLSupport::CellmlFileRuntimeParameter::ComputedConstant:
        return pSimulation->results()->constants(pParameter->index());
    case CellMLSupport::CellmlFileRuntimeParameter::Rate:
        return pSimulation->results()->rates(pParameter->index());
    case CellMLSupport::CellmlFileRuntimeParameter::State:
        return pSimulation->results()->states(pParameter->index());
    case CellMLSupport::CellmlFileRuntimeParameter::Algebraic:
        return pSimulation->results()->algebraic(pParameter->index());
    default:
        // CellMLSupport::CellmlFileRuntimeParameter::Voi

        return pSimulation->results()->points();
    }
}

//==============================================================================

void SingleCellViewSimulationWidget::updateGraphData(SingleCellViewGraphPanelPlotGraph *pGraph,
                                                     const qulonglong &pSize)
{
    // Update our graph's data

    if (pGraph->isValid()) {
        SingleCellViewSimulation *simulation = mViewWidget->simulation(pGraph->fileName());

        pGraph->setRawSamples(dataPoints(simulation, pGraph->parameterX()),
                              dataPoints(simulation, pGraph->parameterY()),
                              pSize);
    }
}

//==============================================================================

void SingleCellViewSimulationWidget::updateGui()
{
    // Make sure that our graphs widget's GUI is up to date

    mContentsWidget->informationWidget()->graphsWidget()->updateGui();

    // Make sure that our plots are up to date
    // Note: indeed, say that we start a simulation with some graphs and switch
    //       to another file before the simulation is complete. Now, if we
    //       switch back to the file after the simulation has completed, the
    //       plots won't be up to date (since the simulation widget was not
    //       visible and therefore didn't update its plots), so we need to do it
    //       here...

    if (mNeedUpdatePlots) {
        mNeedUpdatePlots = false;

        foreach (SingleCellViewGraphPanelPlotWidget *plot, mPlots)
            updatePlot(plot, true);
    }

    // Make sure that our progress bar is up to date

    mProgressBarWidget->setValue(mViewWidget->simulationResultsSize(mFileName)/mSimulation->size());
}

//==============================================================================

void SingleCellViewSimulationWidget::updateSimulationResults(SingleCellViewSimulationWidget *pSimulationWidget,
                                                             const qulonglong &pSimulationResultsSize)
{
    // Update the modified state of our simulation's corresponding file, if
    // needed
    // Note: normally, our simulation worker would, for each point interval,
    //       call SingleCellViewSimulationData::checkForModifications(), but
    //       this would result in a signal being emitted (and then handled by
    //       SingleCellViewSimulationWidget::simulationDataModified()),
    //       resulting in some time overhead, so we check things here
    //       instead...

    SingleCellViewSimulation *simulation = pSimulationWidget->simulation();

    if (simulation == mSimulation)
        checkSimulationDataModified(simulation->data()->isModified());

    // Update all the graphs of all our plots, but only if we are visible

    bool visible = isVisible();

    foreach (SingleCellViewGraphPanelPlotWidget *plot, mPlots) {
        bool needUpdatePlot = false;

        double plotMinX = plot->minX();
        double plotMaxX = plot->maxX();
        double plotMinY = plot->minY();
        double plotMaxY = plot->maxY();

        QRectF plotViewport = QRectF(plotMinX, plotMinY,
                                     plotMaxX-plotMinX, plotMaxY-plotMinY);

        foreach (SingleCellViewGraphPanelPlotGraph *graph, plot->graphs()) {
            if (!graph->fileName().compare(pSimulationWidget->fileName())) {
                // Update our graph's data and keep track of our new old data
                // size, if we are visible
                // Note: indeed, to update our graph's old data size if we are
                //       not visible means that when we come back to this file,
                //       part of the graphs will be missing...

                qulonglong oldDataSize = graph->dataSize();

                if (visible)
                    mOldDataSizes.insert(graph, oldDataSize);

                updateGraphData(graph, pSimulationResultsSize);

                // We need to update our plot, if we are drawing this graph's
                // first segment or if we were invisible at some point during
                // the simulation

                qulonglong realOldDataSize = mOldDataSizes.value(graph);

                needUpdatePlot =    needUpdatePlot || !realOldDataSize
                                 || (oldDataSize != realOldDataSize);

                // Draw the graph's new segment, but only if we and our graph
                // are visible,and that there is no need to update the plot and
                // that there is some data to plot

                if (    visible && graph->isVisible()
                    && !needUpdatePlot && pSimulationResultsSize) {
                    // Check that our graph segment can fit within our plot's
                    // current viewport, but only if the user hasn't changed the
                    // plot's viewport since we last came here

                    if (mPlotsViewports.value(plot) == plotViewport) {
                        double minX = plotMinX;
                        double maxX = plotMaxX;
                        double minY = plotMinY;
                        double maxY = plotMaxY;

                        for (qulonglong i = qMin(0ULL, oldDataSize-1);
                             i < pSimulationResultsSize; ++i) {
                            double valX = graph->data()->sample(i).x();
                            double valY = graph->data()->sample(i).y();

                            minX = qMin(minX, valX);
                            maxX = qMax(maxX, valX);
                            minY = qMin(minY, valY);
                            maxY = qMax(maxY, valY);
                        }

                        // Update pour plot, if our graph segment cannot fit
                        // within our plot's current viewport

                        needUpdatePlot =    (minX < plotMinX) || (maxX > plotMaxX)
                                         || (minY < plotMinY) || (maxY > plotMaxY);
                    }

                    if (!needUpdatePlot)
                        plot->drawGraphFrom(graph, realOldDataSize-1);
                }
            }
        }

        // Check whether we need to update/replot our plot, but only if we are
        // visible

        if (visible) {
            if (needUpdatePlot) {
                // We are either drawing a graph's first segment or its new
                // segment doesn't fit within the plot's current viewport, in
                // which case we need to update our plot

                updatePlot(plot, true);

                // Keep track of our plot's new viewport

                plotMinX = plot->minX();
                plotMinY = plot->minY();

                mPlotsViewports.insert(plot,
                                       QRectF(plotMinX, plotMinY,
                                              plot->maxX()-plotMinX, plot->maxY()-plotMinY));
            } else if (!pSimulationResultsSize) {
                // We came here as a result of starting a simulation or clearing
                // our plot, so simply replot it (rather than update it)
                // Note: we don't want to update our plot since this is going to
                //       reset its axes' values and therefore result in some
                //       (expected) flickering, if some data is to be drawn
                //       straightaway (e.g. when we start a simulation)...

                plot->replotNow();
            }
        } else if (needUpdatePlot || !pSimulationResultsSize) {
            // We would normally update our plot, but we are not visible, so no
            // point in doing it, so instead we keep track of the fact that we
            // will need to update our plots the next time we become visible

            mNeedUpdatePlots = true;
        }
    }

    // Update our progress bar or our tab icon, if needed

    if (simulation == mSimulation) {
        double simulationProgress = mViewWidget->simulationResultsSize(mFileName)/simulation->size();

        if (visible) {
            mProgressBarWidget->setValue(simulationProgress);
        } else {
            // We are not visible, so create an icon that shows our simulation's
            // progress and let people know about it

            int newProgress = (tabBarPixmapSize()-2)*simulationProgress;
            // Note: tabBarPixmapSize()-2 because we want a one-pixel wide
            //       border...

            if (newProgress != mProgress) {
                // The progress has changed, so keep track of its new value and
                // update our file tab icon

                mProgress = newProgress;

                // Let people know about the file tab icon to be used for the
                // model

                emit mViewWidget->updateFileTabIcon(mPlugin->viewName(), mFileName, fileTabIcon());
            }
        }
    }
}

//==============================================================================

QIcon SingleCellViewSimulationWidget::parameterIcon(const CellMLSupport::CellmlFileRuntimeParameter::ParameterType &pParameterType)
{
    // Return an icon that illustrates the type of a parameter

    static const QIcon VoiIcon              = QIcon(":/voi.png");
    static const QIcon ConstantIcon         = QIcon(":/constant.png");
    static const QIcon ComputedConstantIcon = QIcon(":/computedConstant.png");
    static const QIcon RateIcon             = QIcon(":/rate.png");
    static const QIcon StateIcon            = QIcon(":/state.png");
    static const QIcon AlgebraicIcon        = QIcon(":/algebraic.png");
    static const QIcon ErrorNodeIcon        = QIcon(":CellMLSupport_errorNode");

    switch (pParameterType) {
    case CellMLSupport::CellmlFileRuntimeParameter::Voi:
        return VoiIcon;
    case CellMLSupport::CellmlFileRuntimeParameter::Constant:
        return ConstantIcon;
    case CellMLSupport::CellmlFileRuntimeParameter::ComputedConstant:
        return ComputedConstantIcon;
    case CellMLSupport::CellmlFileRuntimeParameter::Rate:
        return RateIcon;
    case CellMLSupport::CellmlFileRuntimeParameter::State:
        return StateIcon;
    case CellMLSupport::CellmlFileRuntimeParameter::Algebraic:
        return AlgebraicIcon;
    default:
        // We are dealing with a type of parameter that is of no interest to us
        // Note: we should never reach this point...

        return ErrorNodeIcon;
    }
}

//==============================================================================

}   // namespace SingleCellView
}   // namespace OpenCOR

//==============================================================================
// End of file
//==============================================================================