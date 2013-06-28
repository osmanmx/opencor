//==============================================================================
// Single cell view information parameters widget
//==============================================================================

#ifndef SINGLECELLVIEWINFORMATIONPARAMETERSWIDGET_H
#define SINGLECELLVIEWINFORMATIONPARAMETERSWIDGET_H

//==============================================================================

#include "cellmlfileruntime.h"
#include "commonwidget.h"

//==============================================================================

#include <QStackedWidget>

//==============================================================================

class QMenu;

//==============================================================================

namespace OpenCOR {

//==============================================================================

namespace Core {
    class Property;
    class PropertyEditorWidget;
}   // namespace Core

//==============================================================================

namespace CellMLSupport {
    class CellmlFileRuntime;
    class CellmlFileRuntimeParameter;
}   // namespace CellMLSupport

//==============================================================================

namespace SingleCellView {

//==============================================================================

class SingleCellViewSimulationData;

//==============================================================================

class SingleCellViewInformationParametersWidget : public QStackedWidget,
                                                  public Core::CommonWidget
{
    Q_OBJECT

public:
    explicit SingleCellViewInformationParametersWidget(QWidget *pParent = 0);

    virtual void retranslateUi();

    virtual void loadSettings(QSettings *pSettings);
    virtual void saveSettings(QSettings *pSettings) const;

    void initialize(const QString &pFileName,
                    CellMLSupport::CellmlFileRuntime *pRuntime,
                    SingleCellViewSimulationData *pSimulationData);
    void finalize(const QString &pFileName);

    void finishPropertyEditing();

private:
    QMap<QString, Core::PropertyEditorWidget *> mPropertyEditors;
    QMap<Core::PropertyEditorWidget *, QMenu *> mContextMenus;

    QMap<Core::Property *, CellMLSupport::CellmlFileRuntimeParameter *> mParameters;
    QMap<QAction *, CellMLSupport::CellmlFileRuntimeParameter *> mParameterActions;

    QList<int> mColumnWidths;

    SingleCellViewSimulationData *mSimulationData;

    QIcon parameterIcon(const CellMLSupport::CellmlFileRuntimeParameter::ParameterType &pParameterType);

    void populateModel(Core::PropertyEditorWidget *pPropertyEditor,
                       CellMLSupport::CellmlFileRuntime *pRuntime);
    void populateContextMenu(QMenu *pContextMenu,
                             CellMLSupport::CellmlFileRuntime *pRuntime);

    void updateParametersToolTips();

    void retranslateContextMenu(QMenu *pContextMenu);

Q_SIGNALS:
    void traceRequired(CellMLSupport::CellmlFileRuntimeParameter *pParameterX,
                       CellMLSupport::CellmlFileRuntimeParameter *pParameterY);

public Q_SLOTS:
    void updateParameters();

private Q_SLOTS:
    void propertyEditorContextMenu(const QPoint &pPosition) const;
    void propertyEditorSectionResized(const int &pLogicalIndex,
                                      const int &pOldSize, const int &pNewSize);

    void propertyChanged(Core::Property *pProperty);

    void emitTraceRequired();
};

//==============================================================================

}   // namespace SingleCellView
}   // namespace OpenCOR

//==============================================================================

#endif

//==============================================================================
// End of file
//==============================================================================
