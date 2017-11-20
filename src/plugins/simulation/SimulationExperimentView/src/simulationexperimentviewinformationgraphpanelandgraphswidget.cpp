/*******************************************************************************

Copyright (C) The University of Auckland

OpenCOR is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

OpenCOR is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*******************************************************************************/

//==============================================================================
// Simulation Experiment view information graph panel and graphs widget
//==============================================================================

#include "cellmlfileruntime.h"
#include "coreguiutils.h"
#include "filemanager.h"
#include "graphpanelwidget.h"
#include "i18ninterface.h"
#include "sedmlsupport.h"
#include "simulation.h"
#include "simulationexperimentviewinformationgraphpanelandgraphswidget.h"
#include "simulationexperimentviewsimulationwidget.h"
#include "simulationexperimentviewwidget.h"

//==============================================================================

#include <QFileInfo>
#include <QHeaderView>
#include <QMenu>
#include <QScrollBar>
#include <QSettings>

//==============================================================================

#include "qwt_symbol.h"

//==============================================================================

namespace OpenCOR {
namespace SimulationExperimentView {

//==============================================================================

SimulationExperimentViewInformationGraphPanelAndGraphsWidget::SimulationExperimentViewInformationGraphPanelAndGraphsWidget(SimulationExperimentViewWidget *pViewWidget,
                                                                                                                           SimulationExperimentViewSimulationWidget *pSimulationWidget,
                                                                                                                           QWidget *pParent) :
    QStackedWidget(pParent),
    Core::CommonWidget(this),
    mMode(Graphs),
    mViewWidget(pViewWidget),
    mSimulationWidget(pSimulationWidget),
    mGraphPanels(QMap<Core::PropertyEditorWidget *, GraphPanelWidget::GraphPanelWidget *>()),
    mGraphPanelPropertyEditors(QMap<GraphPanelWidget::GraphPanelWidget *, Core::PropertyEditorWidget *>()),
    mGraphsPropertyEditors(QMap<GraphPanelWidget::GraphPanelWidget *, Core::PropertyEditorWidget *>()),
    mGraphPanelPropertyEditor(0),
    mGraphsPropertyEditor(0),
    mGraphs(QMap<Core::Property *, GraphPanelWidget::GraphPanelPlotGraph *>()),
    mGraphProperties(QMap<GraphPanelWidget::GraphPanelPlotGraph *, Core::Property *>()),
    mParameterActions(QMap<QAction *, CellMLSupport::CellmlFileRuntimeParameter *>()),
    mRenamedModelListValues(QMap<QString, QString>())
{
    // Create our graph panel context menu and populate it

    mGraphPanelContextMenu = new QMenu(this);

    mSelectGraphPanelColorAction = Core::newAction(this);

    connect(mSelectGraphPanelColorAction, SIGNAL(triggered(bool)),
            this, SLOT(selectGraphPanelColor()));

    mGraphPanelContextMenu->addAction(mSelectGraphPanelColorAction);

    // Create our graph context menus and populate our main graph context menu

    mGraphContextMenu = new QMenu(this);
    mGraphParametersContextMenu = new QMenu(this);

    mAddGraphAction = Core::newAction(this);
    mRemoveCurrentGraphAction = Core::newAction(this);
    mRemoveAllGraphsAction = Core::newAction(this);
    mSelectAllGraphsAction = Core::newAction(this);
    mUnselectAllGraphsAction = Core::newAction(this);
    mSelectGraphColorAction = Core::newAction(this);

    connect(mAddGraphAction, SIGNAL(triggered(bool)),
            this, SLOT(addGraph()));
    connect(mRemoveCurrentGraphAction, SIGNAL(triggered(bool)),
            this, SLOT(removeCurrentGraph()));
    connect(mRemoveAllGraphsAction, SIGNAL(triggered(bool)),
            this, SLOT(removeAllGraphs()));
    connect(mSelectAllGraphsAction, SIGNAL(triggered(bool)),
            this, SLOT(selectAllGraphs()));
    connect(mUnselectAllGraphsAction, SIGNAL(triggered(bool)),
            this, SLOT(unselectAllGraphs()));
    connect(mSelectGraphColorAction, SIGNAL(triggered(bool)),
            this, SLOT(selectGraphColor()));

    mGraphContextMenu->addAction(mAddGraphAction);
    mGraphContextMenu->addSeparator();
    mGraphContextMenu->addAction(mRemoveCurrentGraphAction);
    mGraphContextMenu->addAction(mRemoveAllGraphsAction);
    mGraphContextMenu->addSeparator();
    mGraphContextMenu->addAction(mSelectAllGraphsAction);
    mGraphContextMenu->addAction(mUnselectAllGraphsAction);
    mGraphContextMenu->addSeparator();
    mGraphContextMenu->addAction(mSelectGraphColorAction);

    // Some further initialisations that are done as part of retranslating the
    // GUI (so that they can be updated when changing languages)

    retranslateUi();
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::retranslateGraphPanelPropertyEditor(Core::PropertyEditorWidget *pGraphPanelPropertyEditor)
{
    // Retranslate the contents of the given graph panel property editor

    Core::Properties graphPanelProperties = pGraphPanelPropertyEditor->properties();

    graphPanelProperties[0]->setName(tr("Background colour"));
    graphPanelProperties[1]->setName(tr("Font size"));
    graphPanelProperties[2]->setName(tr("Foreground colour"));

    // Grid lines

    Core::Properties gridLinesProperties = graphPanelProperties[3]->properties();

    graphPanelProperties[3]->setName(tr("Grid lines"));

    gridLinesProperties[0]->setName(tr("Style"));
    gridLinesProperties[1]->setName(tr("Width"));
    gridLinesProperties[2]->setName(tr("Colour"));

    // Legend

    graphPanelProperties[4]->setName(tr("Legend"));

    // Point coordinates

    Core::Properties pointCoordinatesProperties = graphPanelProperties[5]->properties();

    graphPanelProperties[5]->setName(tr("Point coordinates"));

    pointCoordinatesProperties[0]->setName(tr("Style"));
    pointCoordinatesProperties[1]->setName(tr("Width"));
    pointCoordinatesProperties[2]->setName(tr("Colour"));
    pointCoordinatesProperties[3]->setName(tr("Font colour"));

    // Title

    graphPanelProperties[6]->setName(tr("Title"));

    // X axis

    Core::Properties xAxisProperties = graphPanelProperties[7]->properties();

    graphPanelProperties[7]->setName(tr("X axis"));

    xAxisProperties[0]->setName(tr("Logarithmic scale"));
    xAxisProperties[1]->setName(tr("Title"));

    // Y axis

    Core::Properties yAxisProperties = graphPanelProperties[8]->properties();

    graphPanelProperties[8]->setName(tr("Y axis"));

    yAxisProperties[0]->setName(tr("Logarithmic scale"));
    yAxisProperties[1]->setName(tr("Title"));

    // Zoom region

    Core::Properties zoomRegionProperties = graphPanelProperties[9]->properties();

    graphPanelProperties[9]->setName(tr("Zoom region"));

    zoomRegionProperties[0]->setName(tr("Style"));
    zoomRegionProperties[1]->setName(tr("Width"));
    zoomRegionProperties[2]->setName(tr("Colour"));
    zoomRegionProperties[3]->setName(tr("Font colour"));
    zoomRegionProperties[4]->setName(tr("Filled"));
    zoomRegionProperties[5]->setName(tr("Fill colour"));
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::retranslateUi()
{
    // Retranslate our actions

    I18nInterface::retranslateAction(mSelectGraphPanelColorAction, tr("Select Colour..."),
                                     tr("Select a colour"));

    I18nInterface::retranslateAction(mAddGraphAction, tr("Add Graph"),
                                     tr("Add a graph"));
    I18nInterface::retranslateAction(mRemoveCurrentGraphAction, tr("Remove Current Graph"),
                                     tr("Remove the current graph"));
    I18nInterface::retranslateAction(mRemoveAllGraphsAction, tr("Remove All Graphs"),
                                     tr("Remove all the graphs"));
    I18nInterface::retranslateAction(mSelectAllGraphsAction, tr("Select All Graphs"),
                                     tr("Select all the graphs"));
    I18nInterface::retranslateAction(mUnselectAllGraphsAction, tr("Unselect All Graphs"),
                                     tr("Unselect all the graphs"));
    I18nInterface::retranslateAction(mSelectGraphColorAction, tr("Select Colour..."),
                                     tr("Select a colour"));

    // Retranslate all our property editors

    foreach (Core::PropertyEditorWidget *graphPanelPropertyEditor, mGraphPanelPropertyEditors)
        graphPanelPropertyEditor->retranslateUi();

    foreach (Core::PropertyEditorWidget *graphsPropertyEditor, mGraphsPropertyEditors)
        graphsPropertyEditor->retranslateUi();

    // Retranslate the contents of our graph panel property editors

    foreach (Core::PropertyEditorWidget *graphPanelPropertyEditor, mGraphPanelPropertyEditors)
        retranslateGraphPanelPropertyEditor(graphPanelPropertyEditor);

    // Retranslate the information about our graphs properties
    // Note: no need to do this for all our property editors (i.e. call
    //       updateAllGraphsInfo()) since this will automatically be done when
    //       selecting another graph panel. So, instead, we just do this for our
    //       current graph panel...

    updateGraphsInfo();
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::initialize(SimulationSupport::Simulation *pSimulation)
{
    // Populate our graph parameters context menu

    populateGraphParametersContextMenu(pSimulation->runtime());

    // Update our graphs information

    updateAllGraphsInfo();
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::finalize()
{
    // Clear our graph parameters context menu

    mGraphParametersContextMenu->clear();
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::initialize(OpenCOR::GraphPanelWidget::GraphPanelWidget *pGraphPanel,
                                                                              const bool &pActive)
{
    // Retrieve the graph panel and graphs property editors for the given file
    // name or create some, if none exists

    Core::PropertyEditorWidget *oldGraphPanelPropertyEditor = mGraphPanelPropertyEditor;
    Core::PropertyEditorWidget *oldGraphsPropertyEditor = mGraphsPropertyEditor;

    mGraphPanelPropertyEditor = mGraphPanelPropertyEditors.value(pGraphPanel);
    mGraphsPropertyEditor = mGraphsPropertyEditors.value(pGraphPanel);

    if (!mGraphPanelPropertyEditor && !mGraphsPropertyEditor) {
        // No graph panel and graphs property editors exist for the given graph
        // panel, so create some

        mGraphPanelPropertyEditor = new Core::PropertyEditorWidget(false, false, this);
        mGraphsPropertyEditor = new Core::PropertyEditorWidget(false, false, this);

        // Keep track of the link between our existing graph panel and our new
        // graph panel and graphs property editors

        mGraphPanels.insert(mGraphPanelPropertyEditor, pGraphPanel);
        mGraphPanels.insert(mGraphsPropertyEditor, pGraphPanel);

        mGraphPanelPropertyEditors.insert(pGraphPanel, mGraphPanelPropertyEditor);
        mGraphsPropertyEditors.insert(pGraphPanel, mGraphsPropertyEditor);

        // Populate our graph panel property editor

        populateGraphPanelPropertyEditor();

        // We want our own context menu for our graph panel and graphs property
        // editors

        mGraphPanelPropertyEditor->setContextMenuPolicy(Qt::CustomContextMenu);
        mGraphsPropertyEditor->setContextMenuPolicy(Qt::CustomContextMenu);

        connect(mGraphPanelPropertyEditor, SIGNAL(customContextMenuRequested(const QPoint &)),
                this, SLOT(showGraphPanelContextMenu(const QPoint &)));
        connect(mGraphsPropertyEditor, SIGNAL(customContextMenuRequested(const QPoint &)),
                this, SLOT(showGraphsContextMenu(const QPoint &)));

        // Keep track of changes to columns' width

        connect(mGraphPanelPropertyEditor->header(), SIGNAL(sectionResized(int, int, int)),
                this, SIGNAL(graphPanelHeaderSectionResized(const int &, const int &, const int &)));
        connect(mGraphsPropertyEditor->header(), SIGNAL(sectionResized(int, int, int)),
                this, SIGNAL(graphsHeaderSectionResized(const int &, const int &, const int &)));

        // Keep track of changes to the expanded/collapsed state of a property
        // Note: we only need to do this for our graph panel property editor
        //       since graphs are really unique to a graph panel...

        connect(mGraphPanelPropertyEditor, SIGNAL(expanded(QModelIndex)),
                this, SLOT(graphPanelSectionExpanded(const QModelIndex &)));
        connect(mGraphPanelPropertyEditor, SIGNAL(collapsed(QModelIndex)),
                this, SLOT(graphPanelSectionCollapsed(const QModelIndex &)));

        // Keep track of when the user changes a property value

        connect(mGraphPanelPropertyEditor, SIGNAL(propertyChanged(Core::Property *)),
                this, SLOT(graphPanelPropertyChanged(Core::Property *)));
        connect(mGraphsPropertyEditor, SIGNAL(propertyChanged(Core::Property *)),
                this, SLOT(graphsPropertyChanged(Core::Property *)));

        // Add our new graphs property editor to ourselves

        addWidget(mGraphPanelPropertyEditor);
        addWidget(mGraphsPropertyEditor);
    }

    // Make sure that our 'new' property editors' columns' width is the same as
    // that of our 'old' property editors, and that the expanded state of our
    // graph panel editor's sections is properly set

    if (oldGraphPanelPropertyEditor && oldGraphsPropertyEditor) {
        for (int i = 0, iMax = oldGraphPanelPropertyEditor->header()->count(); i < iMax; ++i)
            mGraphPanelPropertyEditor->setColumnWidth(i, oldGraphPanelPropertyEditor->columnWidth(i));

        for (int i = 0, iMax = oldGraphsPropertyEditor->header()->count(); i < iMax; ++i)
            mGraphsPropertyEditor->setColumnWidth(i, oldGraphsPropertyEditor->columnWidth(i));

        for (int i = 0, iMax = oldGraphPanelPropertyEditor->model()->rowCount(); i < iMax; ++i) {
            mGraphPanelPropertyEditor->setExpanded(mGraphPanelPropertyEditor->model()->index(i, 0),
                                                   oldGraphPanelPropertyEditor->isExpanded(oldGraphPanelPropertyEditor->model()->index(i, 0)));
        }
    }

    // Set our retrieved graph panel or graphs property editor as our current
    // widget, but only if the corresponding graph panel has been been made
    // active

    if (pActive)
        setCurrentWidget((mMode == GraphPanel)?mGraphPanelPropertyEditor:mGraphsPropertyEditor);

    // Update our graphs information
    // Note: this is in case the user changed the locale and then switched to a
    //       different graph panel...

    updateAllGraphsInfo();
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::finalize(OpenCOR::GraphPanelWidget::GraphPanelWidget *pGraphPanel)
{
    // Remove track of the link betwen our graph panel and our graph panel and
    // graphs property editors

    Core::PropertyEditorWidget *graphPanelPropertyEditor = mGraphPanelPropertyEditors.value(pGraphPanel);
    Core::PropertyEditorWidget *graphsPropertyEditor = mGraphsPropertyEditors.value(pGraphPanel);

    if (   (graphPanelPropertyEditor == mGraphPanelPropertyEditor)
        && (graphsPropertyEditor == mGraphsPropertyEditor)) {
        mGraphPanelPropertyEditor = 0;
        mGraphsPropertyEditor = 0;
    }

    mGraphPanels.remove(graphPanelPropertyEditor);
    mGraphPanels.remove(graphsPropertyEditor);

    mGraphPanelPropertyEditors.remove(pGraphPanel);
    mGraphsPropertyEditors.remove(pGraphPanel);

    removeWidget(graphPanelPropertyEditor);
    removeWidget(graphsPropertyEditor);

    delete graphPanelPropertyEditor;
    delete graphsPropertyEditor;
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::addGraph(OpenCOR::GraphPanelWidget::GraphPanelWidget *pGraphPanel,
                                                                            OpenCOR::GraphPanelWidget::GraphPanelPlotGraph *pGraph,
                                                                            const OpenCOR::GraphPanelWidget::GraphPanelPlotGraphProperties &pGraphProperties)
{
    // Make sure that we have a graphs property editor

    Core::PropertyEditorWidget *graphsPropertyEditor = mGraphsPropertyEditors.value(pGraphPanel);

    if (!graphsPropertyEditor)
        return;

    // Create a section for our newly added graph

    Core::Property *graphProperty = graphsPropertyEditor->addSectionProperty();

    graphProperty->setCheckable(true);
    graphProperty->setChecked(true);

    // Keep track of the link between our given graph and our graph property

    mGraphs.insert(graphProperty, pGraph);
    mGraphProperties.insert(pGraph, graphProperty);

    // Create some graph properties
    // Note: to add properties will result in the propertyChanged() signal being
    //       emitted, but we don't want to handle that signal (at least, not
    //       when creating a graph since not everyting may be set yet, so this
    //       might cause more problems than anything), so we must disconnect
    //       ourselves from it before adding the properties (and then reconnect
    //       ourselves to it once we are done)...

    disconnect(graphsPropertyEditor, SIGNAL(propertyChanged(Core::Property *)),
               this, SLOT(graphsPropertyChanged(Core::Property *)));

    graphsPropertyEditor->addListProperty(graphProperty);
    graphsPropertyEditor->addStringProperty(pGraph->parameterX()?
                                                static_cast<CellMLSupport::CellmlFileRuntimeParameter *>(pGraph->parameterX())->fullyFormattedName():
                                                Core::UnknownValue,
                                            graphProperty);
    graphsPropertyEditor->addStringProperty(pGraph->parameterY()?
                                                static_cast<CellMLSupport::CellmlFileRuntimeParameter *>(pGraph->parameterY())->fullyFormattedName():
                                                Core::UnknownValue,
                                            graphProperty);

    // Create some line properties for our graph

    Core::Property *lineProperty = graphsPropertyEditor->addSectionProperty(graphProperty);

    graphsPropertyEditor->addListProperty(SEDMLSupport::lineStyles(),
                                          SEDMLSupport::lineStyleValue((pGraphProperties.lineStyle() > Qt::DashDotDotLine)?
                                                                           Qt::SolidLine:
                                                                           pGraphProperties.lineStyle()),
                                          lineProperty);
    graphsPropertyEditor->addIntegerGt0Property(pGraphProperties.lineWidth(), lineProperty);
    graphsPropertyEditor->addColorProperty(pGraphProperties.lineColor(), lineProperty);

    // Create some symbol properties for our graph

    Core::Property *symbolProperty = graphsPropertyEditor->addSectionProperty(graphProperty);

    graphsPropertyEditor->addListProperty(SEDMLSupport::symbolStyles(),
                                          SEDMLSupport::symbolStyleValue((pGraphProperties.symbolStyle() <= QwtSymbol::DTriangle)?
                                                                             pGraphProperties.symbolStyle()+1:
                                                                             ((pGraphProperties.symbolStyle() >= QwtSymbol::Cross) && (pGraphProperties.symbolStyle() <= QwtSymbol::Star1))?
                                                                                 pGraphProperties.symbolStyle()-2:
                                                                                 QwtSymbol::NoSymbol),
                                          symbolProperty);
    graphsPropertyEditor->addIntegerGt0Property(pGraphProperties.symbolSize(), symbolProperty);
    graphsPropertyEditor->addColorProperty(pGraphProperties.symbolColor(), symbolProperty);
    graphsPropertyEditor->addBooleanProperty(pGraphProperties.symbolFilled(), symbolProperty);
    graphsPropertyEditor->addColorProperty(pGraphProperties.symbolFillColor(), symbolProperty);

    connect(graphsPropertyEditor, SIGNAL(propertyChanged(Core::Property *)),
            this, SLOT(graphsPropertyChanged(Core::Property *)));

    // Update the information about our new graph

    updateGraphsInfo(graphProperty);
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::removeGraphs(OpenCOR::GraphPanelWidget::GraphPanelWidget *pGraphPanel,
                                                                                const OpenCOR::GraphPanelWidget::GraphPanelPlotGraphs &pGraphs)
{
    // Make sure that we have a graphs property editor

    Core::PropertyEditorWidget *graphsPropertyEditor = mGraphsPropertyEditors.value(pGraphPanel);

    if (!graphsPropertyEditor)
        return;

    // Remove the graph properties associated with the given graphs, as well as
    // their trace

    foreach (GraphPanelWidget::GraphPanelPlotGraph *graph, pGraphs) {
        Core::Property *property = mGraphProperties.value(graph);

        graphsPropertyEditor->removeProperty(property);

        mGraphs.remove(property);
        mGraphProperties.remove(graph);
    }
}

//==============================================================================

bool SimulationExperimentViewInformationGraphPanelAndGraphsWidget::rootProperty(Core::Property *pProperty) const
{
    // Return whether the given property is a root property

    return    (pProperty->type() == Core::Property::Section)
           && !pProperty->parentProperty() && pProperty->isCheckable();
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::selectGraphPanelColor()
{
    // Select a colour for the current property

    mGraphPanelPropertyEditor->currentProperty()->setColorValue();
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::addGraph()
{
    // Ask the graph panel associated with our current graphs property editor to
    // add an 'empty' graph

    mGraphPanels.value(mGraphsPropertyEditor)->addGraph(new GraphPanelWidget::GraphPanelPlotGraph());
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::removeCurrentGraph()
{
    // Ask the graph panel associated with our current graphs property editor to
    // remove the current graph

    mGraphPanels.value(mGraphsPropertyEditor)->removeGraphs(GraphPanelWidget::GraphPanelPlotGraphs() << mGraphs.value(mGraphsPropertyEditor->currentProperty()));
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::removeAllGraphs()
{
    // Ask the graph panel associated with our current graphs property editor to
    // remove all the graphs

    mGraphPanels.value(mGraphsPropertyEditor)->removeGraphs(mGraphs.values());
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::selectAllGraphs(const bool &pSelect)
{
    // (Un)select all the graphs
    // Note: normally, we would only update the checked state of our graph
    //       properties, which would in turn update the selected state of our
    //       graphs and let people know that they have been updated. Now, the
    //       problem with this is that every single graph is going to be shown/
    //       hidden indivdually. So, if we have loads of graphs with many data
    //       points, then it's going to be slow. So, instead, we disconnect the
    //       propertyChanged() signal, update the checked state of our graph
    //       properties as well as the selected state of our graphs, and then
    //       let people know that all the graphs have been updated...

    disconnect(mGraphsPropertyEditor, SIGNAL(propertyChanged(Core::Property *)),
               this, SLOT(graphsPropertyChanged(Core::Property *)));

    foreach (Core::Property *property, mGraphProperties)
        property->setChecked(pSelect);

    foreach (GraphPanelWidget::GraphPanelPlotGraph *graph, mGraphs)
        graph->setSelected(pSelect);

    if (mGraphs.count())
        emit graphsUpdated(mGraphs.values());

    connect(mGraphsPropertyEditor, SIGNAL(propertyChanged(Core::Property *)),
            this, SLOT(graphsPropertyChanged(Core::Property *)));
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::selectAllGraphs()
{
    // Select all the graphs

    selectAllGraphs(true);
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::unselectAllGraphs()
{
    // Unselect all the graphs

    selectAllGraphs(false);
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::selectGraphColor()
{
    // Select a colour for the current property

    mGraphsPropertyEditor->currentProperty()->setColorValue();
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::fileRenamed(const QString &pOldFileName,
                                                                               const QString &pNewFileName)
{
    // Keep track of the renamed model list value (so that we can take advantage
    // of it in our calls to updateGraphsInfo() in updateAllGraphsInfo())

    QString oldModelListValue = modelListValue(pOldFileName);

    if (mRenamedModelListValues.values().contains(oldModelListValue)) {
        // A previous model list value has been renamed again, so find its
        // original value and update its new renamed version, if needed

        foreach (const QString &origModelListValue, mRenamedModelListValues.keys()) {
            QString oldModelListValue = mRenamedModelListValues.value(origModelListValue);

            if (!oldModelListValue.compare(oldModelListValue)) {
                QString newModelListValue = modelListValue(pNewFileName);

                if (!origModelListValue.compare(newModelListValue)) {
                    // A model list value has been re-renamed to its original
                    // value, so stop tracking it

                    mRenamedModelListValues.remove(origModelListValue);
                } else {
                    mRenamedModelListValues.insert(origModelListValue, newModelListValue);
                }

                break;
            }
        }
    } else {
        // Not a renamed model list value that we know, so just keep track of it

        mRenamedModelListValues.insert(oldModelListValue, modelListValue(pNewFileName));
    }
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::updateGui()
{
    // Update our graphs information
    // Note: this is useful, for example, if we created a graph for a file that
    //       has not yet been selected, in which case the graph will initially
    //       be invalid, but it should become valid after we have switched to
    //       that file and back...

    updateAllGraphsInfo();
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::finishEditing()
{
    // Make sure that we have a graphs property editor

    if (!mGraphsPropertyEditor)
        return;

    // Ask our graphs property editor to finish editing

    mGraphsPropertyEditor->finishEditing();
}

//==============================================================================

Core::Properties SimulationExperimentViewInformationGraphPanelAndGraphsWidget::graphPanelProperties(GraphPanelWidget::GraphPanelWidget *pGraphPanel) const
{
    // Retrieve and return all the graph panel properties

    return mGraphPanelPropertyEditors.value(pGraphPanel)->properties();
}

//==============================================================================

static const auto PropertySeparator = QStringLiteral(" | ");

//==============================================================================

Core::Properties SimulationExperimentViewInformationGraphPanelAndGraphsWidget::graphProperties(GraphPanelWidget::GraphPanelWidget *pGraphPanel,
                                                                                               const QString &pFileName) const
{
    // Retrieve and return all the graph properties associated with the given
    // graph and file name, if any

    Core::Properties res = Core::Properties();
    Core::PropertyEditorWidget *graphsPropertyEditor = mGraphsPropertyEditors.value(pGraphPanel);

    if (graphsPropertyEditor) {
        foreach (Core::Property *property, graphsPropertyEditor->properties()) {
            // The property should be returned if it is checked (i.e. a selected
            // graph) and have its first sub-property (i.e. to which model the
            // graph applies) has either a value of "Current" or that of the
            // given file name

            if (property->isChecked()) {
                QString modelPropertyValue = property->properties().first()->value();

                if (   !modelPropertyValue.compare(tr("Current"))
                    || !modelPropertyValue.split(PropertySeparator).last().compare(pFileName)) {
                    res << property;
                }
            }
        }
    }

    return res;
}

//==============================================================================

SimulationExperimentViewInformationGraphPanelAndGraphsWidget::Mode SimulationExperimentViewInformationGraphPanelAndGraphsWidget::mode() const
{
    // Return our mode

    return mMode;
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::setMode(const Mode &pMode)
{
    // Set our mode

    if (pMode != mMode) {
        mMode = pMode;

        // Set our graph panel or graphs property editor as our current widget,
        // if we have one (i.e. the model could be compiled)

        Core::PropertyEditorWidget *propertyEditor = (pMode == GraphPanel)?mGraphPanelPropertyEditor:mGraphsPropertyEditor;

        if (propertyEditor)
            setCurrentWidget(propertyEditor);

        // Let people know about our change of modes

        emit graphPanelGraphsModeChanged(pMode);
    }
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::setGraphPanelColumnWidth(const int &pIndex,
                                                                                            const int &pColumnWidth)
{
    // Make sure that we have a graph panel property editor

    if (!mGraphPanelPropertyEditor)
        return;

    // Set the width of the given column

    mGraphPanelPropertyEditor->setColumnWidth(pIndex, pColumnWidth);
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::setGraphsColumnWidth(const int &pIndex,
                                                                                        const int &pColumnWidth)
{
    // Make sure that we have a graphs property editor

    if (!mGraphsPropertyEditor)
        return;

    // Set the width of the given column

    mGraphsPropertyEditor->setColumnWidth(pIndex, pColumnWidth);
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::setGraphPanelSectionExpanded(const int &pSection,
                                                                                                const bool &pExpanded)
{
    // Make sure that we have a graph panel property editor

    if (!mGraphPanelPropertyEditor)
        return;

    // Expand/collapse the given section

    mGraphPanelPropertyEditor->setExpanded(mGraphPanelPropertyEditor->model()->index(pSection, 0), pExpanded);
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::reinitialize(GraphPanelWidget::GraphPanelWidget *pGraphPanel)
{
    // Reinitialise ourselves

    finalize(pGraphPanel);
    initialize(pGraphPanel);
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::showGraphPanelContextMenu(const QPoint &pPosition) const
{
    Q_UNUSED(pPosition);

    // Make sure that we have a graph panel property editor

    if (!mGraphPanelPropertyEditor)
        return;

    // Show the graph panel context menu, or not, depending on the type of
    // property we are dealing with, if any

    Core::Property *crtProperty = mGraphPanelPropertyEditor->currentProperty();

    if (    crtProperty
            && (crtProperty->type() == Core::Property::Color)) {
        mGraphPanelContextMenu->exec(QCursor::pos());
    }
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::showGraphsContextMenu(const QPoint &pPosition) const
{
    Q_UNUSED(pPosition);

    // Make sure that we have a graphs property editor

    if (!mGraphsPropertyEditor)
        return;

    // Update the enabled state of some of our actions

    Core::Property *crtProperty = mGraphsPropertyEditor->currentProperty();

    mRemoveCurrentGraphAction->setEnabled(crtProperty);
    mRemoveAllGraphsAction->setEnabled(!mGraphsPropertyEditor->properties().isEmpty());

    bool canSelectAllGraphs = false;
    bool canUnselectAllGraphs = false;

    foreach (Core::Property *property, mGraphProperties) {
        bool graphSelected = property->isChecked();

        canSelectAllGraphs = canSelectAllGraphs || !graphSelected;
        canUnselectAllGraphs = canUnselectAllGraphs || graphSelected;
    }

    mSelectAllGraphsAction->setEnabled(canSelectAllGraphs);
    mUnselectAllGraphsAction->setEnabled(canUnselectAllGraphs);

    mSelectGraphColorAction->setVisible(    crtProperty
                                        && (crtProperty->type() == Core::Property::Color));

    // Show the graph context menu, or not, depending on the type of property we
    // are dealing with, if any

    if (   crtProperty
        && (   !crtProperty->name().compare(tr("X"))
            || !crtProperty->name().compare(tr("Y")))) {
        mGraphParametersContextMenu->exec(QCursor::pos());
    } else {
        mGraphContextMenu->exec(QCursor::pos());
    }
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::populateGraphPanelPropertyEditor()
{
    // Populate our graph panel property editor

    GraphPanelWidget::GraphPanelPlotWidget *graphPanelPlot = mGraphPanels.value(mGraphPanelPropertyEditor)->plot();

    mGraphPanelPropertyEditor->addColorProperty(graphPanelPlot->backgroundColor());
    mGraphPanelPropertyEditor->addIntegerGt0Property(graphPanelPlot->fontSize());
    mGraphPanelPropertyEditor->addColorProperty(graphPanelPlot->foregroundColor());

    // Grid lines

    Core::Property *gridLinesProperty = mGraphPanelPropertyEditor->addSectionProperty();

    mGraphPanelPropertyEditor->addListProperty(SEDMLSupport::lineStyles(),
                                               SEDMLSupport::lineStyleValue((graphPanelPlot->gridLinesStyle() > Qt::DashDotDotLine)?
                                                                                 Qt::SolidLine:
                                                                                 graphPanelPlot->gridLinesStyle()),
                                               gridLinesProperty);
    mGraphPanelPropertyEditor->addIntegerGt0Property(graphPanelPlot->gridLinesWidth(), gridLinesProperty);
    mGraphPanelPropertyEditor->addColorProperty(graphPanelPlot->gridLinesColor(), gridLinesProperty);

    // Legend

    mGraphPanelPropertyEditor->addBooleanProperty(graphPanelPlot->legend());

    // Point coordinates

    Core::Property *pointCoordinatesProperty = mGraphPanelPropertyEditor->addSectionProperty();

    mGraphPanelPropertyEditor->addListProperty(SEDMLSupport::lineStyles(),
                                               SEDMLSupport::lineStyleValue((graphPanelPlot->pointCoordinatesStyle() > Qt::DashDotDotLine)?
                                                                                 Qt::SolidLine:
                                                                                 graphPanelPlot->pointCoordinatesStyle()),
                                               pointCoordinatesProperty);
    mGraphPanelPropertyEditor->addIntegerGt0Property(graphPanelPlot->pointCoordinatesWidth(), pointCoordinatesProperty);
    mGraphPanelPropertyEditor->addColorProperty(graphPanelPlot->pointCoordinatesColor(), pointCoordinatesProperty);
    mGraphPanelPropertyEditor->addColorProperty(graphPanelPlot->pointCoordinatesFontColor(), pointCoordinatesProperty);

    // Title

    mGraphPanelPropertyEditor->addStringProperty();

    // X axis

    Core::Property *xAxisProperty = mGraphPanelPropertyEditor->addSectionProperty();

    mGraphPanelPropertyEditor->addBooleanProperty(xAxisProperty);
    mGraphPanelPropertyEditor->addStringProperty(xAxisProperty);

    // Y axis

    Core::Property *yAxisProperty = mGraphPanelPropertyEditor->addSectionProperty();

    mGraphPanelPropertyEditor->addBooleanProperty(yAxisProperty);
    mGraphPanelPropertyEditor->addStringProperty(yAxisProperty);

    // Zoom region

    Core::Property *zoomRegionProperty = mGraphPanelPropertyEditor->addSectionProperty();

    mGraphPanelPropertyEditor->addListProperty(SEDMLSupport::lineStyles(),
                                               SEDMLSupport::lineStyleValue((graphPanelPlot->zoomRegionStyle() > Qt::DashDotDotLine)?
                                                                                 Qt::SolidLine:
                                                                                 graphPanelPlot->zoomRegionStyle()),
                                               zoomRegionProperty);
    mGraphPanelPropertyEditor->addIntegerGt0Property(graphPanelPlot->zoomRegionWidth(), zoomRegionProperty);
    mGraphPanelPropertyEditor->addColorProperty(graphPanelPlot->zoomRegionColor(), zoomRegionProperty);
    mGraphPanelPropertyEditor->addColorProperty(graphPanelPlot->zoomRegionFontColor(), zoomRegionProperty);
    mGraphPanelPropertyEditor->addBooleanProperty(graphPanelPlot->zoomRegionFilled(), zoomRegionProperty);
    mGraphPanelPropertyEditor->addColorProperty(graphPanelPlot->zoomRegionFillColor(), zoomRegionProperty);

    // Retranslate the contents of our graph panel property editor

    retranslateGraphPanelPropertyEditor(mGraphPanelPropertyEditor);
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::populateGraphParametersContextMenu(CellMLSupport::CellmlFileRuntime *pRuntime)
{
    // Populate our graph parameters context menu with the contents of our main
    // graph context menu

    mGraphParametersContextMenu->addActions(mGraphContextMenu->actions());
    mGraphParametersContextMenu->addSeparator();

    // Now, add our model parameters to it

    QString componentHierarchy = QString();
    QMenu *componentMenu = 0;

    foreach (CellMLSupport::CellmlFileRuntimeParameter *parameter, pRuntime->parameters()) {
        // Check whether the current parameter is in the same component
        // hierarchy as the previous one

        QString crtComponentHierarchy = parameter->formattedComponentHierarchy();

        if (crtComponentHierarchy.compare(componentHierarchy)) {
            // The current parameter is in a different component hierarchy, so
            // create a new menu hierarchy for our 'new' component, reusing
            // existing menus, whenever possible

            QMenu *parentComponentMenu = mGraphParametersContextMenu;

            foreach (const QString &component, parameter->componentHierarchy()) {
                // Check whether we already have a menu for our current
                // component

                componentMenu = 0;

                foreach (QObject *object, parentComponentMenu->children()) {
                    QMenu *subMenu = qobject_cast<QMenu *>(object);

                    if (subMenu && !subMenu->menuAction()->text().compare(component)) {
                        componentMenu = subMenu;

                        break;
                    }
                }

                // Create a new menu for our current component, if none could be
                // found

                if (!componentMenu) {
                    componentMenu = new QMenu(component, parentComponentMenu);

                    parentComponentMenu->addMenu(componentMenu);
                }

                // Get ready for the next component in our component hierarchy

                parentComponentMenu = componentMenu;
            }

            // Keep track of the new component hierarchy

            componentHierarchy = crtComponentHierarchy;
        }

        // Make sure that we have a 'current' component menu
        // Note: this should never happen, but we never know...

        if (!componentMenu)
            continue;

        // Add the current parameter to the 'current' component menu

        QAction *parameterAction = componentMenu->addAction(parameter->icon(),
                                                            parameter->formattedName());

        // Create a connection to handle the parameter value update

        connect(parameterAction, SIGNAL(triggered(bool)),
                this, SLOT(updateParameterValue()));

        // Keep track of the parameter associated with our model parameter
        // action

        mParameterActions.insert(parameterAction, parameter);
    }
}

//==============================================================================

bool SimulationExperimentViewInformationGraphPanelAndGraphsWidget::checkParameter(CellMLSupport::CellmlFileRuntime *pRuntime,
                                                                                  GraphPanelWidget::GraphPanelPlotGraph *pGraph,
                                                                                  Core::Property *pParameterProperty,
                                                                                  const bool &pParameterX) const
{
    // Check that the information held by the given property corresponds to
    // an existing parameter in our runtime

    CellMLSupport::CellmlFileRuntimeParameter *res = 0;

    if (pRuntime) {
        // Retrieve the component and parameter of the property

        QStringList info = pParameterProperty->value().split('.');
        QStringList componentHierarchy = info;

        componentHierarchy.removeLast();

        // Determine the degree of our parameter, if any

        QString parameterName = info.last();
        int parameterDegree = parameterName.length();

        parameterName.replace('\'', QString());

        parameterDegree -= parameterName.length();

        // Check whether we can find our property among our runtime's parameters

        foreach (CellMLSupport::CellmlFileRuntimeParameter *parameter, pRuntime->parameters()) {
            if (   (parameter->componentHierarchy() == componentHierarchy)
                && !parameter->name().compare(parameterName)
                && (parameter->degree() == parameterDegree)) {
                res = parameter;

                break;
            }
        }
    }

    // Update our parameter property based on whether it corresponds to an
    // existing parameter in our runtime

    static const QIcon BlankIcon   = QIcon(":/SimulationExperimentView/blank.png");
    static const QIcon WarningIcon = QIcon(":/oxygen/status/task-attention.png");

    pParameterProperty->setIcon(res?BlankIcon:WarningIcon);
    pParameterProperty->setExtraInfo(res?
                                         QString():
                                         pRuntime?
                                             tr("does not exist"):
                                             tr("no runtime"));

    // Keep track of the existing parameter, if any, to which our property
    // corresponds

    if (pParameterX)
        pGraph->setParameterX(res);
    else
        pGraph->setParameterY(res);

    return res;
}

//==============================================================================

QString SimulationExperimentViewInformationGraphPanelAndGraphsWidget::modelListValue(const QString &pFileName) const
{
    // Return the model list value for the given file name

    return QFileInfo(pFileName).fileName()+PropertySeparator+pFileName;
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::updateGraphInfo(Core::Property *pProperty)
{
    // Make sure that we have a property

    if (!pProperty)
        return;

    // Update the model property's icon, based on the value of the model
    // property, and determine the file name against which we have to check our
    // X and Y properties

    static const QIcon LockedIcon   = QIcon(":/oxygen/status/object-locked.png");
    static const QIcon UnlockedIcon = QIcon(":/oxygen/status/object-unlocked.png");

    GraphPanelWidget::GraphPanelPlotGraph *graph = mGraphs.value(pProperty);
    Core::Properties properties = pProperty->properties();
    QString propertyFileName = properties[0]->value();
    QString fileName = mSimulationWidget->fileName();

    if (!propertyFileName.compare(tr("Current"))) {
        properties[0]->setIcon(UnlockedIcon);
    } else {
        properties[0]->setIcon(LockedIcon);

        fileName = propertyFileName.split(PropertySeparator).last();
    }

    // Check that the parameters represented by the value of the X and Y
    // properties exist for the current/selected model
    // Note: we absolutely want to check the parameter (so that an icon can be
    //       assigned to its corresponding property) , hence the order of our &&
    //       tests when setting graphOk...

    bool graphOk = true;
    CellMLSupport::CellmlFileRuntime *runtime = mViewWidget->runtime(fileName);
    CellMLSupport::CellmlFileRuntimeParameter *oldParameterX = static_cast<CellMLSupport::CellmlFileRuntimeParameter *>(graph->parameterX());
    CellMLSupport::CellmlFileRuntimeParameter *oldParameterY = static_cast<CellMLSupport::CellmlFileRuntimeParameter *>(graph->parameterY());

    graphOk = checkParameter(runtime, graph, properties[1], true) && graphOk;
    graphOk = checkParameter(runtime, graph, properties[2], false) && graphOk;

    // Update our section's name, if possible
    // Note: indeed, when populating ourselves, updateGraphInfo() gets called
    //       (through graphsPropertyChanged()), yet we don't want to (and can't)
    //       do what follows if not all the properties are available...

    pProperty->setName( properties[1]->value()
                       +PropertySeparator
                       +properties[2]->value());

    // Update the status (i.e. icon) of our (section) property

    static const QIcon BlankIcon   = QIcon(":/SimulationExperimentView/blank.png");
    static const QIcon WarningIcon = QIcon(":/oxygen/status/task-attention.png");

    pProperty->setIcon(graphOk?BlankIcon:WarningIcon);

    // Update the file name with which the graph is associated

    graph->setFileName(fileName);

    // Update the graph line settings

    QPen oldLinePen = graph->pen();
    QPen linePen = oldLinePen;
    Core::Properties lineProperties = properties[3]->properties();

    linePen.setStyle(Qt::PenStyle(lineProperties[0]->listValues().indexOf(lineProperties[0]->listValue())));
    linePen.setWidth(lineProperties[1]->integerValue());
    linePen.setColor(lineProperties[2]->colorValue());

    graph->setPen(linePen);

    // Update the graph symbol settings

    const QwtSymbol *oldGraphSymbol = graph->symbol();
    bool graphSymbolUpdated = !oldGraphSymbol;
    Core::Properties symbolProperties = properties[4]->properties();
    int symbolStyleValue = symbolProperties[0]->listValues().indexOf(symbolProperties[0]->listValue());
    QwtSymbol::Style symbolStyle = QwtSymbol::Style((symbolStyleValue > QwtSymbol::DTriangle+1)?symbolStyleValue+2:symbolStyleValue-1);
    int symbolSize = symbolProperties[1]->integerValue();
    QPen symbolColor = QPen(symbolProperties[2]->colorValue());
    bool symbolFill = symbolProperties[3]->booleanValue();
    QBrush symbolFillColor = symbolFill?QBrush(symbolProperties[4]->colorValue()):QBrush();

    if (oldGraphSymbol) {
        graphSymbolUpdated =    (oldGraphSymbol->style() != symbolStyle)
                             || (oldGraphSymbol->size().width() != symbolSize)
                             || (oldGraphSymbol->pen() != symbolColor)
                             || (oldGraphSymbol->brush() != symbolFillColor);
    }

    graph->setSymbol(new QwtSymbol(symbolStyle, symbolFillColor, symbolColor,
                                   QSize(symbolSize, symbolSize)));

    // Let people know if the X and/or Y parameters of our graph have changed or
    // replot it if its settings have changed

    if (   (oldParameterX != graph->parameterX())
        || (oldParameterY != graph->parameterY())) {
        emit graphUpdated(graph);
    } else if ((oldLinePen != linePen) || graphSymbolUpdated) {
        graph->plot()->replot();

        QCoreApplication::processEvents();
        // Note: this ensures that our plot is updated at once...
    }
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::graphPanelSectionExpanded(const QModelIndex &pIndex)
{
    // Let people know that the given section has been expanded

    emit graphPanelSectionExpanded(pIndex.row(), true);
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::graphPanelSectionCollapsed(const QModelIndex &pIndex)
{
    // Let people know that the given section has been collapsed

    emit graphPanelSectionExpanded(pIndex.row(), false);
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::graphPanelPropertyChanged(Core::Property *pProperty)
{
    // Update our graph panel settings

    Core::PropertyEditorWidget *propertyEditor = pProperty->owner();
    GraphPanelWidget::GraphPanelPlotWidget *graphPanelPlot = mGraphPanels.value(propertyEditor)->plot();
    Core::Properties properties = propertyEditor->properties();

    graphPanelPlot->setUpdatesEnabled(false);

    graphPanelPlot->setBackgroundColor(properties[0]->colorValue());
    graphPanelPlot->setFontSize(properties[1]->integerValue());
    graphPanelPlot->setForegroundColor(properties[2]->colorValue());

    // Grid lines

    Core::Properties gridLinesProperties = properties[3]->properties();

    graphPanelPlot->setGridLinesStyle(Qt::PenStyle(gridLinesProperties[0]->listValues().indexOf(gridLinesProperties[0]->listValue())));
    graphPanelPlot->setGridLinesWidth(gridLinesProperties[1]->integerValue());
    graphPanelPlot->setGridLinesColor(gridLinesProperties[2]->colorValue());

    // Legend

    graphPanelPlot->setLegend(properties[4]->booleanValue());

    // Point coordinates

    Core::Properties pointCoordinatesProperties = properties[5]->properties();

    graphPanelPlot->setPointCoordinatesStyle(Qt::PenStyle(pointCoordinatesProperties[0]->listValues().indexOf(pointCoordinatesProperties[0]->listValue())));
    graphPanelPlot->setPointCoordinatesWidth(pointCoordinatesProperties[1]->integerValue());
    graphPanelPlot->setPointCoordinatesColor(pointCoordinatesProperties[2]->colorValue());
    graphPanelPlot->setPointCoordinatesFontColor(pointCoordinatesProperties[3]->colorValue());

    // Title

    graphPanelPlot->setTitle(properties[6]->value());

    // X axis

    Core::Properties xAxisProperties = properties[7]->properties();

    graphPanelPlot->setLogAxisX(xAxisProperties[0]->booleanValue());
    graphPanelPlot->setTitleAxisX(xAxisProperties[1]->value());

    // Y axis

    Core::Properties yAxisProperties = properties[8]->properties();

    graphPanelPlot->setLogAxisY(yAxisProperties[0]->booleanValue());
    graphPanelPlot->setTitleAxisY(yAxisProperties[1]->value());

    // Zoom region

    Core::Properties zoomRegionProperties = properties[9]->properties();

    graphPanelPlot->setZoomRegionStyle(Qt::PenStyle(zoomRegionProperties[0]->listValues().indexOf(zoomRegionProperties[0]->listValue())));
    graphPanelPlot->setZoomRegionWidth(zoomRegionProperties[1]->integerValue());
    graphPanelPlot->setZoomRegionColor(zoomRegionProperties[2]->colorValue());
    graphPanelPlot->setZoomRegionFontColor(zoomRegionProperties[3]->colorValue());
    graphPanelPlot->setZoomRegionFilled(zoomRegionProperties[4]->booleanValue());
    graphPanelPlot->setZoomRegionFillColor(zoomRegionProperties[5]->colorValue());

    graphPanelPlot->setUpdatesEnabled(true);

    graphPanelPlot->forceAlignWithNeighbors();
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::graphsPropertyChanged(Core::Property *pProperty)
{
    // Our graph has changed, which means that either it has been un/selected or
    // that the value of one of its properties has changed

    if (rootProperty(pProperty)) {
        // The property associated with the graph is our root property, which
        // means that the graph has been un/selected, so update its selected
        // state and let people know that our graph has been updated

        GraphPanelWidget::GraphPanelPlotGraph *graph = mGraphs.value(pProperty);

        if (graph) {
            graph->setSelected(pProperty->isChecked());

            emit graphUpdated(graph);
        }
    } else {
        // One of our graph properties has changed, so update its information
        // Note: updateGraphInfo() will emit the graphUpdated() signal, if
        //       needed...

        Core::Property *graphProperty = pProperty->parentProperty();

        while (graphProperty->parentProperty())
            graphProperty = graphProperty->parentProperty();

        updateGraphInfo(graphProperty);
    }
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::updateGraphsInfo(Core::Property *pSectionProperty)
{
    // Make sure that we have a graphs property editor

    if (!mGraphsPropertyEditor)
        return;

    // Make sure that no editing is in progress

    finishEditing();

    // Use the given section property or retrieve the ones for our current
    // graphs property editor

    Core::Properties graphProperties = Core::Properties();

    if (pSectionProperty) {
        graphProperties << pSectionProperty;
    } else {
        foreach (Core::Property *property, mGraphsPropertyEditor->properties()) {
            if (rootProperty(property))
                graphProperties << property;
        }
    }

    // Determine the model list values, but only if needed, i.e. if we have some
    // graph properties

    QStringList modelListValues = QStringList();

    if (!graphProperties.isEmpty()) {
        foreach (const QString &fileName, mViewWidget->fileNames()) {
            Core::File *file = Core::FileManager::instance()->file(fileName);
            QString fileNameOrUrl = file->isLocal()?fileName:file->url();

            modelListValues << modelListValue(fileNameOrUrl);
        }

        modelListValues.sort();

        modelListValues.prepend(QString());
        modelListValues.prepend(tr("Current"));
    }

    // Go through our graph properties and update (incl. retranslate) their
    // information

    foreach (Core::Property *graphProperty, graphProperties) {
        // Set the label of our graph properties

        Core::Properties graphProperties = graphProperty->properties();

        graphProperties[0]->setName(tr("Model"));
        graphProperties[1]->setName(tr("X"));
        graphProperties[2]->setName(tr("Y"));

        // Set the label of our graph line properties

        Core::Properties lineProperties = graphProperties[3]->properties();

        graphProperties[3]->setName(tr("Line"));

        lineProperties[0]->setName(tr("Style"));
        lineProperties[0]->setListValues(QStringList() << tr("None")
                                                       << tr("Solid")
                                                       << tr("Dash")
                                                       << tr("Dot")
                                                       << tr("DashDot")
                                                       << tr("DashDotDot"),
                                         false);
        lineProperties[1]->setName(tr("Width"));
        lineProperties[2]->setName(tr("Colour"));

        // Set the label of our graph symbol properties

        Core::Properties symbolProperties = graphProperties[4]->properties();

        graphProperties[4]->setName(tr("Symbol"));

        symbolProperties[0]->setName(tr("Style"));
        symbolProperties[0]->setListValues(QStringList() << tr("None")
                                                         << tr("Circle")
                                                         << tr("Square")
                                                         << tr("Diamond")
                                                         << tr("Triangle")
                                                         << tr("Down Triangle")
                                                         << tr("Cross")
                                                         << tr("X Cross")
                                                         << tr("Horizontal Line")
                                                         << tr("Vertical Line")
                                                         << tr("Star"),
                                           false);
        symbolProperties[1]->setName(tr("Size"));
        symbolProperties[2]->setName(tr("Colour"));
        symbolProperties[3]->setName(tr("Filled"));
        symbolProperties[4]->setName(tr("Fill colour"));

        // Keep track of the current model value, so that we can safely update
        // its list values and then select the correct model value back

        QString oldModelValue = graphProperties[0]->value();
        QString newModelValue = oldModelValue;

        graphProperties[0]->setListValues(modelListValues, false);
        // Note: we don't want setListValues() to emit a signal since one will
        //       be emitted as a result of our call to setValue() below...

        if (newModelValue.isEmpty()) {
            // newModelValue is empty, which means that this is the first time
            // we come here, so update our graph info using the current value of
            // our model property

            newModelValue = graphProperties[0]->value();
        } else if (!modelListValues.contains(oldModelValue)) {
            // Our old model value is not in our new model list values anymore,
            // which means that either the value of the model property was
            // "Current" (and the locale got changed) or the current file got
            // renamed

            if (oldModelValue.contains(PropertySeparator)) {
                // The current file got renamed

                newModelValue = mRenamedModelListValues.value(oldModelValue);
            } else {
                // The value of the model property was "Current" (and the locale
                // got changed)

                newModelValue = tr("Current");
            }
        }

        // Set the value of our model property to newModelValue (which will
        // result in updateGraphInfo() being called, so we are all fine)

        graphProperties[0]->setValue(newModelValue, true);
        // Note: we must force the setting of the value since it may very well
        //       be that it's the same as before, yet we want the signal
        //       associated with setValue() to be emitted (so that
        //       updateGraphInfo() can then be called)...
    }
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::updateAllGraphsInfo()
{
    // Update the information about our graphs properties and this for all our
    // property editors, so first keep track of our active graphs property
    // editor

    Core::PropertyEditorWidget *oldGraphsPropertyEditor = mGraphsPropertyEditor;

    // Go through our graphs property editors and update the information about
    // the graph properties they hold

    foreach (Core::PropertyEditorWidget *graphsPropertyEditor, mGraphsPropertyEditors) {
        mGraphsPropertyEditor = graphsPropertyEditor;

        updateGraphsInfo();
    }

    // Our list of renamed model list values was used in our different calls to
    // updateGraphsInfo() above, so it can now be reset

    mRenamedModelListValues.clear();

    // Retrieve our originally active graphs property editor

    mGraphsPropertyEditor = oldGraphsPropertyEditor;
}

//==============================================================================

void SimulationExperimentViewInformationGraphPanelAndGraphsWidget::updateParameterValue()
{
    // Update the current property's value

    mGraphsPropertyEditor->currentProperty()->setValue(mParameterActions.value(qobject_cast<QAction *>(sender()))->fullyFormattedName());
}

//==============================================================================

}   // namespace SimulationExperimentView
}   // namespace OpenCOR

//==============================================================================
// End of file
//==============================================================================
