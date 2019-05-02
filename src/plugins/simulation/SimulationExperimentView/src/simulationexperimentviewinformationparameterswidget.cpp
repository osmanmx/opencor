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
// Simulation Experiment view information parameters widget
//==============================================================================

#include "cellmlfileruntime.h"
#include "simulation.h"
#include "simulationexperimentviewinformationparameterswidget.h"
#include "simulationexperimentviewsimulationwidget.h"

//==============================================================================

#include <QContextMenuEvent>
#include <QMenu>

//==============================================================================

namespace OpenCOR {
namespace SimulationExperimentView {

//==============================================================================

SimulationExperimentViewInformationParametersWidget::SimulationExperimentViewInformationParametersWidget(QWidget *pParent) :
    PropertyEditorWidget(false, pParent),
    mPlotAgainstVoiMenuAction(nullptr),
    mPlotAgainstMenu(nullptr),
    mParameters(QMap<Core::Property *, CellMLSupport::CellmlFileRuntimeParameter *>()),
    mParameterActions(QMap<QAction *, CellMLSupport::CellmlFileRuntimeParameter *>()),
    mSimulation(nullptr),
    mNeedClearing(false),
    mVoiAccessible(false),
    mGradientIndices(QSet<int>()),
    mToggleGradientsMenuAction(nullptr),
    mToggleGradientsMenuSeparator(nullptr)
    mImportComponent(nullptr),
    mImportMenu(nullptr)
{
    // Create our context menu

    mContextMenu = new QMenu(this);

    // Keep track of when the user changes a property value

    connect(this, &Core::PropertyEditorWidget::propertyChanged,
            this, &SimulationExperimentViewInformationParametersWidget::propertyChanged);
}

//==============================================================================

void SimulationExperimentViewInformationParametersWidget::retranslateContextMenu()
{
    // Retranslate our context menu, if it exists / has been populated

    if (mPlotAgainstVoiMenuAction != nullptr) {
        mPlotAgainstVoiMenuAction->setText(tr("Plot Against Variable Of Integration"));
    }

    if (mPlotAgainstMenu != nullptr) {
        mPlotAgainstMenu->menuAction()->setText(tr("Plot Against"));

    if (mToggleGradientsMenuAction)
        mToggleGradientsMenuAction->setText(tr("Toggle gradient calculation for constant"));
    }
}

//==============================================================================

void SimulationExperimentViewInformationParametersWidget::retranslateUi()
{
    // Default retranslation

    PropertyEditorWidget::retranslateUi();

    // Retranslate our context menu

    retranslateContextMenu();

    // Retranslate the extra info of all our parameters

    updateExtraInfos();
}

//==============================================================================

void SimulationExperimentViewInformationParametersWidget::contextMenuEvent(QContextMenuEvent *pEvent)
{
    // Make sure that we have a current property

    Core::Property *crtProperty = currentProperty();

    if (crtProperty == nullptr) {
        return;
    }

    // Make sure that our current property is not a section

    if (crtProperty->type() == Core::Property::Type::Section) {
        return;
    }

    // Show separator and item to toggle constants having gradients calculated

    CellMLSupport::CellmlFileRuntimeParameter *parameter = mParameters.value(crtProperty);

    bool gradientActionVisible = (parameter && parameter->type() == CellMLSupport::CellmlFileRuntimeParameter::Type::Constant);

    mToggleGradientsMenuSeparator->setVisible(gradientActionVisible);

    mToggleGradientsMenuAction->setVisible(gradientActionVisible);

    // Generate and show the context menu

    mContextMenu->exec(pEvent->globalPos());
}

//==============================================================================

void SimulationExperimentViewInformationParametersWidget::initialize(SimulationSupport::Simulation *pSimulation,
                                                                     bool pReloadingView)
{
    // Keep track of the simulation

    mSimulation = pSimulation;

    // First clear ourselves, if needed

    if (mNeedClearing) {
        clear();

        mNeedClearing = false;
    }

    // Check whether our model's VOI is among our model's parameters (i.e. it is
    // defined in the main CellML file)

    CellMLSupport::CellmlFileRuntime *runtime = pSimulation->runtime();

    mVoiAccessible = runtime->parameters().contains(runtime->voi());

    // Retranslate our core self, if needed
    // Note: part of reloading ourselves consists of finalising ourselves, which
    //       means clearing all of our contents including our header labels. So,
    //       we need to 'retranslate' them otherwise they will read "1", "2",
    //       "3", etc.

    if (pReloadingView) {
        PropertyEditorWidget::retranslateUi();
    }

    // Populate our property editor and context menu

    populateModel(runtime);
    populateContextMenu(runtime);

    // Reset our import information, if we are reloading ourselves

    if (pReloadingView) {
        mImportComponent = nullptr;
    }

    // Keep track of when some of the model's data has changed

    connect(pSimulation->data(), &SimulationSupport::SimulationData::updatedParameters,
            this, &SimulationExperimentViewInformationParametersWidget::updateParameters);
}

//==============================================================================

void SimulationExperimentViewInformationParametersWidget::finalize()
{
    // Clear ourselves, as well as our context menu, parameters and parameter
    // actions

    mNeedClearing = true;

    mContextMenu->clear();

    mParameters.clear();
    mParameterActions.clear();
}

//==============================================================================

void SimulationExperimentViewInformationParametersWidget::importData(DataStore::DataStoreImportData *pImportData)
{
    // Create our general import "component", if needed

    if (mImportComponent == nullptr) {
        mImportComponent = addSectionProperty("imports");
    }

    // Create our import "sub-component"

    Core::Property *importSubComponent = addSectionProperty(pImportData->hierarchy().last(), mImportComponent);

    // Add the given data to our model

    CellMLSupport::CellmlFileRuntimeParameters parameters = mSimulation->runtime()->dataParameters(mSimulation->data()->data(pImportData->importDataStore()));

    for (auto parameter : parameters) {
        Core::Property *property = addDoubleProperty(importSubComponent);

        property->setEditable(false);
        property->setIcon(CellMLSupport::CellmlFileRuntimeParameter::icon(parameter->type()));
        property->setName(parameter->formattedName(), false);

        // Keep track of the link between our property value and parameter

        mParameters.insert(property, parameter);
    }

    // Update (well, set for our imported data) the extra info of all our
    // properties

    updateExtraInfos();

    // Expand our import component and sub-component

    expand(mImportComponent->index());
    expand(importSubComponent->index());

    // Create our general import menu, if needed

    if (mImportMenu == nullptr) {
        mImportMenu = new QMenu("imports", mPlotAgainstMenu);

        mPlotAgainstMenu->addMenu(mImportMenu);
    }

    // Create our import sub-menu

    auto importSubMenu = new QMenu(pImportData->hierarchy().last(), mImportMenu);

    mImportMenu->addMenu(importSubMenu);

    // Populate our import sub-menu with the given data

    for (auto parameter : parameters) {
        QAction *parameterAction = importSubMenu->addAction(CellMLSupport::CellmlFileRuntimeParameter::icon(parameter->type()),
                                                            parameter->formattedName());

        // Create a connection to handle the graph requirement against our
        // parameter

        connect(parameterAction, &QAction::triggered,
                this, &SimulationExperimentViewInformationParametersWidget::emitGraphRequired);

        // Keep track of the parameter associated with our parameter action

        mParameterActions.insert(parameterAction, parameter);
    }
}

//==============================================================================

void SimulationExperimentViewInformationParametersWidget::updateParameters(double pCurrentPoint)
{
    // Update our data

    for (auto property : allProperties()) {
        CellMLSupport::CellmlFileRuntimeParameter *parameter = mParameters.value(property);

        if (parameter != nullptr) {
            CellMLSupport::CellmlFileRuntimeParameter::Type parameterType = parameter->type();

            if (parameterType == CellMLSupport::CellmlFileRuntimeParameter::Type::Voi) {
                property->setDoubleValue(pCurrentPoint, false);
            } else if (   (parameterType == CellMLSupport::CellmlFileRuntimeParameter::Type::Constant)
                       || (parameterType == CellMLSupport::CellmlFileRuntimeParameter::Type::ComputedConstant)) {
                property->setDoubleValue(mSimulation->data()->constants()[parameter->index()], false);
            } else if (parameterType == CellMLSupport::CellmlFileRuntimeParameter::Type::Rate) {
                property->setDoubleValue(mSimulation->data()->rates()[parameter->index()], false);
            } else if (parameterType == CellMLSupport::CellmlFileRuntimeParameter::Type::State) {
                property->setDoubleValue(mSimulation->data()->states()[parameter->index()], false);
            } else if (parameterType == CellMLSupport::CellmlFileRuntimeParameter::Type::Algebraic) {
                property->setDoubleValue(mSimulation->data()->algebraic()[parameter->index()], false);
            } else if (parameterType == CellMLSupport::CellmlFileRuntimeParameter::Type::Data) {
                property->setDoubleValue(parameter->data()[parameter->index()], false);
            }
        }
    }

    // Check whether any of our properties has actually been modified

    mSimulation->data()->checkForModifications();
}

//==============================================================================

void SimulationExperimentViewInformationParametersWidget::propertyChanged(Core::Property *pProperty)
{
    // Update our simulation data

    CellMLSupport::CellmlFileRuntimeParameter *parameter = mParameters.value(pProperty);

    if (parameter != nullptr) {
        CellMLSupport::CellmlFileRuntimeParameter::Type parameterType = parameter->type();

        if (parameterType == CellMLSupport::CellmlFileRuntimeParameter::Type::Constant) {
            mSimulation->data()->constants()[parameter->index()] = pProperty->doubleValue();
        } else if (parameterType == CellMLSupport::CellmlFileRuntimeParameter::Type::State) {
            mSimulation->data()->states()[parameter->index()] = pProperty->doubleValue();
        }
    }

    // Recompute our 'computed constants' and 'variables'
    // Note #1: we would normally call
    //          mSimulation->data()->checkForModifications() after recomputing
    //          our 'computed constants' and 'variables, but the recomputation
    //          will eventually result in updateParameters() above to be called,
    //          which will check for modifications...
    // Note #2: some state variables may be considered as computed constants by
    //          the CellML API. This is fine when we need to initialise things,
    //          but not after the user has modified one or several model
    //          parameters (see issue #234 for more information), hence our
    //          passing false to mSimulation->data()->reset()...

    mSimulation->data()->reset(false);
}

//==============================================================================

QMap<Core::Property *, CellMLSupport::CellmlFileRuntimeParameter *> SimulationExperimentViewInformationParametersWidget::parameters() const
{
    // Return our parameters

    return mParameters;
}

//==============================================================================

void SimulationExperimentViewInformationParametersWidget::populateModel(CellMLSupport::CellmlFileRuntime *pRuntime)
{
    // Populate our property editor with the parameters

    QString componentHierarchy = QString();
    Core::Property *sectionProperty = nullptr;

    for (auto parameter : pRuntime->parameters()) {
        // Check whether the current parameter is in the same component
        // hierarchy as the previous one

        QString crtComponentHierarchy = parameter->formattedComponentHierarchy();

        if (crtComponentHierarchy != componentHierarchy) {
            // The current parameter is in a different component hierarchy, so
            // create a new section hierarchy for our 'new' component, reusing
            // existing sections, whenever possible

            Core::Property *parentSectionProperty = nullptr;

            for (const auto &component : parameter->componentHierarchy()) {
                // Check whether we already have a section for our current
                // component

                sectionProperty = nullptr;

                if (parentSectionProperty != nullptr) {
                    // We have a parent section, so go through its children and
                    // retrieve the one for our current component

                    for (auto object : parentSectionProperty->children()) {
                        auto property = qobject_cast<Core::Property *>(object);

                        if (   (property != nullptr)
                            && (property->type() == Core::Property::Type::Section)
                            && (property->name() == component)) {
                            sectionProperty = property;

                            break;
                        }
                    }
                } else {
                    // We don't have a parent section, so go through our
                    // properties and retrieve the one for our current component

                    for (auto property : properties()) {
                        if (   (property->type() == Core::Property::Type::Section)
                            && (property->name() == component)) {
                            sectionProperty = property;

                            break;
                        }
                    }
                }

                // Create a new section for our current component, if none could
                // be found

                if (sectionProperty == nullptr) {
                    sectionProperty = addSectionProperty(component, parentSectionProperty);
                }

                // Get ready for the next component in our component hierarchy

                parentSectionProperty = sectionProperty;
            }

            // Keep track of the new component hierarchy

            componentHierarchy = crtComponentHierarchy;
        }

        // Add the current parameter to the current section property, after
        // having retrieved its current value

        double propertyValue = 0.0;
        CellMLSupport::CellmlFileRuntimeParameter::Type parameterType = parameter->type();

        if (parameterType == CellMLSupport::CellmlFileRuntimeParameter::Type::Voi) {
            propertyValue = mSimulation->data()->startingPoint();
        } else if (   (parameterType == CellMLSupport::CellmlFileRuntimeParameter::Type::Constant)
                   || (parameterType == CellMLSupport::CellmlFileRuntimeParameter::Type::ComputedConstant)) {
            propertyValue = mSimulation->data()->constants()[parameter->index()];
        } else if (parameterType == CellMLSupport::CellmlFileRuntimeParameter::Type::Rate) {
            propertyValue = mSimulation->data()->rates()[parameter->index()];
        } else if (parameterType == CellMLSupport::CellmlFileRuntimeParameter::Type::State) {
            propertyValue = mSimulation->data()->states()[parameter->index()];
        } else if (parameterType == CellMLSupport::CellmlFileRuntimeParameter::Type::Algebraic) {
            propertyValue = mSimulation->data()->algebraic()[parameter->index()];
        }

        Core::Property *property = addDoubleProperty(propertyValue, sectionProperty);

        property->setEditable(   (parameter->type() == CellMLSupport::CellmlFileRuntimeParameter::Type::Constant)
                              || (parameter->type() == CellMLSupport::CellmlFileRuntimeParameter::Type::State));
        property->setIcon(CellMLSupport::CellmlFileRuntimeParameter::icon(parameter->type()));
        property->setName(parameter->formattedName(), false);
        property->setUnit(parameter->formattedUnit(pRuntime->voi()->unit()), false);

        // Keep track of the link between our property value and parameter

        mParameters.insert(property, parameter);
    }

    // Update (well, set here) the extra info of all our parameters

    updateExtraInfos();

    // Expand all our properties

    expandAll();
}

//==============================================================================

void SimulationExperimentViewInformationParametersWidget::populateContextMenu(CellMLSupport::CellmlFileRuntime *pRuntime)
{
    // Create our two main menu items

    if (mVoiAccessible) {
        mPlotAgainstVoiMenuAction = mContextMenu->addAction(QString());
    }

    mPlotAgainstMenu = new QMenu(mContextMenu);

    mContextMenu->addMenu(mPlotAgainstMenu);

    // Create a hidden menu item to toggle whether constants have gradients calculated

    mToggleGradientsMenuSeparator = mContextMenu->addSeparator();
    mToggleGradientsMenuSeparator->setVisible(false);

    mToggleGradientsMenuAction = mContextMenu->addAction(QString());
    mToggleGradientsMenuAction->setVisible(false);

    connect(mToggleGradientsMenuAction, SIGNAL(triggered(bool)), this, SLOT(toggleGradientFlag()));

    // Initialise our menu items

    retranslateContextMenu();

    // Create a connection to handle the graph requirement against our VOI, and
    // keep track of the parameter associated with our first main menu item

    if (mVoiAccessible) {
        connect(mPlotAgainstVoiMenuAction, &QAction::triggered,
                this, &SimulationExperimentViewInformationParametersWidget::emitGraphRequired);

        mParameterActions.insert(mPlotAgainstVoiMenuAction, pRuntime->voi());
    }

    // Populate our context menu with the parameters

    QString componentHierarchy = QString();
    QMenu *componentMenu = nullptr;

    for (auto parameter : pRuntime->parameters()) {
        // Check whether the current parameter is in the same component
        // hierarchy as the previous one

        QString crtComponentHierarchy = parameter->formattedComponentHierarchy();

        if (crtComponentHierarchy != componentHierarchy) {
            // The current parameter is in a different component hierarchy, so
            // create a new menu hierarchy for our 'new' component, reusing
            // existing menus, whenever possible

            QMenu *menu = mPlotAgainstMenu;

            for (const auto &component : parameter->componentHierarchy()) {
                // Check whether we already have a menu for our current
                // component

                componentMenu = nullptr;

                for (auto object : menu->children()) {
                    auto subMenu = qobject_cast<QMenu *>(object);

                    if (   (subMenu != nullptr)
                        && (subMenu->menuAction()->text() == component)) {
                        componentMenu = subMenu;

                        break;
                    }
                }

                // Create a new menu for our current component, if none could be
                // found

                if (componentMenu == nullptr) {
                    componentMenu = new QMenu(component, menu);

                    menu->addMenu(componentMenu);
                }

                // Get ready for the next component in our component hierarchy

                menu = componentMenu;
            }

            // Keep track of the new component hierarchy

            componentHierarchy = crtComponentHierarchy;
        }

        // Make sure that we have a 'current' component menu
        // Note: this should never happen, but we never know...

        if (componentMenu == nullptr) {
            continue;
        }

        // Add the current parameter to the 'current' component menu

        QAction *parameterAction = componentMenu->addAction(CellMLSupport::CellmlFileRuntimeParameter::icon(parameter->type()),
                                                            parameter->formattedName());

        // Create a connection to handle the graph requirement against our
        // parameter

        connect(parameterAction, &QAction::triggered,
                this, &SimulationExperimentViewInformationParametersWidget::emitGraphRequired);

        // Keep track of the parameter associated with our parameter action

        mParameterActions.insert(parameterAction, parameter);
    }
}

//==============================================================================

void SimulationExperimentViewInformationParametersWidget::updateExtraInfos()
{
    // Update the extra info of all our properties

    for (auto property : allProperties()) {
        CellMLSupport::CellmlFileRuntimeParameter *parameter = mParameters.value(property);

        if (parameter != nullptr) {
            QString extraInfo = QString();
            CellMLSupport::CellmlFileRuntimeParameter::Type parameterType = parameter->type();

            if (parameterType == CellMLSupport::CellmlFileRuntimeParameter::Type::Voi) {
                extraInfo = tr("variable of integration");
            } else if (parameterType == CellMLSupport::CellmlFileRuntimeParameter::Type::Constant) {
                extraInfo = tr("constant");
            } else if (parameterType == CellMLSupport::CellmlFileRuntimeParameter::Type::ComputedConstant) {
                extraInfo = tr("computed constant");
            } else if (parameterType == CellMLSupport::CellmlFileRuntimeParameter::Type::Rate) {
                extraInfo = tr("rate");
            } else if (parameterType == CellMLSupport::CellmlFileRuntimeParameter::Type::State) {
                extraInfo = tr("state");
            } else if (parameterType == CellMLSupport::CellmlFileRuntimeParameter::Type::Algebraic) {
                extraInfo = tr("algebraic");
            } else if (parameterType == CellMLSupport::CellmlFileRuntimeParameter::Type::Data) {
                extraInfo = tr("data");
            }

            property->setExtraInfo(extraInfo);
        }
    }
}

//==============================================================================

void SimulationExperimentViewInformationParametersWidget::emitGraphRequired()
{
    // Let people know that we want to plot the current parameter against
    // another

    emit graphRequired(mParameterActions.value(qobject_cast<QAction *>(sender())),
                       mParameters.value(currentProperty()));
}

//==============================================================================

void SimulationExperimentViewInformationParametersWidget::gradientToggled(
    CellMLSupport::CellmlFileRuntimeParameter *pParameter, const bool &pCalculate)
{
    static const QIcon ConstantWithGradientIcon = QIcon(":/SimulationExperimentView/constantWithGradient.png");

    Core::Property *property = mParameters.key(pParameter);

    if (property) {
        int index = pParameter->index();
        if (mGradientIndices.contains(index)) {
            if (!pCalculate) {
                mGradientIndices.remove(index);
                property->setIcon(CellMLSupport::CellmlFileRuntimeParameter::icon(pParameter->type()));
            }
        } else if (pCalculate) {
            mGradientIndices << index;
            property->setIcon(ConstantWithGradientIcon);
        }
    }
}

//==============================================================================

void SimulationExperimentViewInformationParametersWidget::toggleGradientFlag()
{
    // Make sure that we have a current property

    Core::Property *crtProperty = currentProperty();

    if (!crtProperty)
        return;

    CellMLSupport::CellmlFileRuntimeParameter *parameter = mParameters.value(crtProperty);

    if (parameter->type() == CellMLSupport::CellmlFileRuntimeParameter::Type::Constant)
        gradientToggled(parameter, !mGradientIndices.contains(parameter->index()));

    emit calculateGradients(parameter->index(), mGradientIndices.contains(parameter->index()));
}

//==============================================================================

} // namespace SimulationExperimentView
} // namespace OpenCOR

//==============================================================================
// End of file
//==============================================================================
