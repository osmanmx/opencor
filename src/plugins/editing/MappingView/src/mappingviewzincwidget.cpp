/*******************************************************************************

Copyright (C) The University of Auckland

OpenCOR is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

OpenCOR is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://gnu.org/licenses>.

*******************************************************************************/

//==============================================================================
// Zinc widget
//==============================================================================

#include "mappingviewzincwidget.h"
#include "corecliutils.h"
#include "filemanager.h"
#include "cellmlfilemanager.h"

//==============================================================================

#include <QMouseEvent>
#include <QOpenGLContext>
#include <QTimer>
#include <QDir>

//==============================================================================

#include <array>

//==============================================================================

#include "zincbegin.h"
    #include "opencmiss/zinc/context.hpp"
    #include "opencmiss/zinc/result.hpp"
    #include "opencmiss/zinc/scenefilter.hpp"
    #include "opencmiss/zinc/field.hpp"
    #include "opencmiss/zinc/graphics.hpp"
#include "zincend.h"

//==============================================================================

namespace OpenCOR {
namespace MappingView {

//==============================================================================

MappingViewZincWidget::MappingViewZincWidget(QWidget *pParent, const QString &pMainFileName) :
    ZincWidget::ZincWidget(pParent),
    mMainFileName(pMainFileName),
    mAuxFileName(pMainFileName),
    mNodeSize(nodeSizeOrigin)
{
    // Keep track of our current scene viewer's description
    //TODO usefull ?
    mZincSceneViewerDescription = sceneViewer().writeDescription();

    //seek and read .exelem file

    mAuxFileName.remove(".exnode");
    mAuxFileName.append(".exelem");

    QFileInfo check_file;
    check_file.setFile(mAuxFileName);

    if (!check_file.exists()) {
        mAuxFileName = "";
    }

    // Create and set our Zinc context

    mZincContext = new OpenCMISS::Zinc::Context("MappingViewZincContext");

    mZincContext->getMaterialmodule().defineStandardMaterials();
    mZincContext->getGlyphmodule().defineStandardGlyphs();

    setContext(mZincContext);

    //Create and initialize region and scene

    mRegion = new OpenCMISS::Zinc::Region(mZincContext->getDefaultRegion());
    mScene = new OpenCMISS::Zinc::Scene(mRegion->getScene());

    // Create and initialize scenePicker with a filter (improvable)

    mScenePicker = new OpenCMISS::Zinc::Scenepicker(mScene->createScenepicker());

    OpenCMISS::Zinc::Scenefiltermodule sceneFilterModule = mScene->getScenefiltermodule();
    OpenCMISS::Zinc::Scenefilter nodeFilter =
            sceneFilterModule.createScenefilterFieldDomainType(OpenCMISS::Zinc::Field::DOMAIN_TYPE_NODES);
    mScenePicker->setScenefilter(nodeFilter);

    // Allow for things to be dropped on us

    //TODO remove this ?
    //setAcceptDrops(true);
}

//==============================================================================

MappingViewZincWidget::~MappingViewZincWidget()
{
    delete mZincContext;
    delete mScene;
    delete mRegion;
}

//==============================================================================

void MappingViewZincWidget::initializeGL()
{
    ZincWidget::initializeGL();

    //mSceneViewer.readDescription(mZincSceneViewerDescription);

    // background colour

    std::array<double, 4> backgroundColor = { 0.85, 0.85, 0.85, 1.0 };

    mSceneViewer.setBackgroundColourRGBA(backgroundColor.data());

    mSceneViewer.setScene(mRegion->getScene());

    //read files
    mRegion->readFile(qPrintable(mMainFileName));

    if (mAuxFileName!="") {
        mRegion->readFile(qPrintable(mAuxFileName));
    }

    mFieldModule = mRegion->getFieldmodule();

    mFieldModule.beginChange();
        OpenCMISS::Zinc::Field coordinates = mFieldModule.findFieldByName("Coordinates");
    mFieldModule.endChange();

    mScene->beginChange();
        OpenCMISS::Zinc::Materialmodule materialModule = mScene->getMaterialmodule();

        //Black lines

        OpenCMISS::Zinc::GraphicsLines lines = mScene->createGraphicsLines();

        lines.setCoordinateField(coordinates);
        lines.setMaterial(materialModule.findMaterialByName("red"));

        // Green spheres limiting our scene

        OpenCMISS::Zinc::GraphicsPoints nodePoints = mScene->createGraphicsPoints();

        nodePoints.setCoordinateField(coordinates);
        nodePoints.setFieldDomainType(OpenCMISS::Zinc::Field::DOMAIN_TYPE_NODES);
        nodePoints.setMaterial(materialModule.findMaterialByName("green"));

        // Size of our green spheres

        OpenCMISS::Zinc::Graphicspointattributes pointAttr = nodePoints.getGraphicspointattributes();

        pointAttr.setBaseSize(1, &mNodeSize);
        pointAttr.setGlyphShapeType(OpenCMISS::Zinc::Glyph::SHAPE_TYPE_SPHERE);

    mScene->endChange();

    // adding view all command

    mSceneViewer.viewAll();
}

//==============================================================================

void MappingViewZincWidget::mouseMoveEvent(QMouseEvent *pEvent)
{
    ZincWidget::mouseMoveEvent(pEvent);
}

//==============================================================================

void MappingViewZincWidget::mousePressEvent(QMouseEvent *pEvent)
{
    ZincWidget::mousePressEvent(pEvent);
    mMousePosClick = pEvent->pos();
}

//==============================================================================

void MappingViewZincWidget::mouseReleaseEvent(QMouseEvent *pEvent)
{
    ZincWidget::mouseReleaseEvent(pEvent);

    if (mMousePosClick==pEvent->pos()) {
        click(pEvent);
    }
}

//==============================================================================

void MappingViewZincWidget::wheelEvent(QWheelEvent *pEvent)
{
    ZincWidget::wheelEvent(pEvent);
}

//==============================================================================

void MappingViewZincWidget::click(QMouseEvent *pEvent)
{
    mScenePicker->setSceneviewerRectangle(mSceneViewer, OpenCMISS::Zinc::SCENECOORDINATESYSTEM_WINDOW_PIXEL_TOP_LEFT,
                                          pEvent->x() - mSizeSelection,
                                          pEvent->y() - mSizeSelection,
                                          pEvent->x() + mSizeSelection,
                                          pEvent->y() + mSizeSelection);
    OpenCMISS::Zinc::Node node = mScenePicker->getNearestNode();

    int id = node.getIdentifier();
    emit nodeSelection(id);

    // select the node to highlight graphics

    mFieldModule.beginChange();

        OpenCMISS::Zinc::FieldGroup selectionGroup = mScene->getSelectionField().castGroup();

        if (node.isValid()){
            if (!selectionGroup.isValid()) {
                selectionGroup = mFieldModule.createFieldGroup();
                selectionGroup.setName("Selection");
                mScene->setSelectionField(selectionGroup);
            }

            OpenCMISS::Zinc::Nodeset nodes = mFieldModule.findNodesetByFieldDomainType(OpenCMISS::Zinc::Field::DOMAIN_TYPE_NODES);
            OpenCMISS::Zinc::FieldNodeGroup nodeGroupField = selectionGroup.getFieldNodeGroup(nodes);

            if (!nodeGroupField.isValid()) {
                nodeGroupField = selectionGroup.createFieldNodeGroup(nodes);
            }

            OpenCMISS::Zinc::NodesetGroup nodesetGroup = nodeGroupField.getNodesetGroup();
            nodesetGroup.removeAllNodes();
            nodesetGroup.addNode(node);

        } else {

            if (selectionGroup.isValid()) {
                mScene->setSelectionField(OpenCMISS::Zinc::Field());
            }
        }
    mFieldModule.endChange();
}

//==============================================================================

void MappingViewZincWidget::setNodeSizes(int pSize) {
     mNodeSize = pSize;
    //TODO
    // draw or modifiate
}

} // namespace ZincWidget
} // namespace OpenCOR

//==============================================================================
// End of file
//==============================================================================
