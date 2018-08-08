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
// BioSignalML data store plugin
//==============================================================================

#include "biosignalmldatastoredata.h"
#include "biosignalmldatastoredialog.h"
#include "biosignalmldatastoreexporter.h"
#include "biosignalmldatastoreplugin.h"
#include "corecliutils.h"
#include "coreguiutils.h"

//==============================================================================

#include <QMainWindow>

//==============================================================================

namespace OpenCOR {
namespace BioSignalMLDataStore {

//==============================================================================

PLUGININFO_FUNC BioSignalMLDataStorePluginInfo()
{
    Descriptions descriptions;

    descriptions.insert("en", QString::fromUtf8("a BioSignalML specific data store plugin."));
    descriptions.insert("fr", QString::fromUtf8("une extension de magasin de données spécifique à BioSignalML."));

    return new PluginInfo(PluginInfo::DataStore, true, false,
                          QStringList() << "BioSignalML" << "DataStore",
                          descriptions);
}

//==============================================================================
// Data store interface
//==============================================================================

QString BioSignalMLDataStorePlugin::dataStoreName() const
{
    // Return the name of the data store

    return "BioSignalML";
}

//==============================================================================

DataStore::DataStoreData * BioSignalMLDataStorePlugin::getData(const QString &pFileName,
                                                               DataStore::DataStore *pDataStore,
                                                               const QMap<int, QIcon> &pIcons) const
{
    // Ask which data should be exported, as well as some other information

    BiosignalmlDataStoreDialog biosignalmlDataStoreDialog(pDataStore, pIcons, Core::mainWindow());

    if (biosignalmlDataStoreDialog.exec()) {
        // Now that we have the information we need, we can ask for the name of
        // the BioSignalML file where to do the export

        QString biosignalmlFilter = tr("BioSignalML File")+" (*.biosignalml)";
        QString fileName = Core::getSaveFileName(tr("Export To BioSignalML"),
                                                 Core::newFileName(pFileName, tr("Data"), false, "biosignalml"),
                                                 QStringList() << biosignalmlFilter,
                                                 &biosignalmlFilter);

        if (!fileName.isEmpty()) {
            return new BiosignalmlDataStoreData(fileName,
                                                biosignalmlDataStoreDialog.name(),
                                                biosignalmlDataStoreDialog.author(),
                                                biosignalmlDataStoreDialog.description(),
                                                tr("Generated by %1 at %2 from %3.").arg(Core::version())
                                                                                    .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODate))
                                                                                    .arg(pDataStore->uri()),
                                                pDataStore,
                                                biosignalmlDataStoreDialog.selectedData());
        }
    }

    return nullptr;
}

//==============================================================================

DataStore::DataStoreExporter * BioSignalMLDataStorePlugin::dataStoreExporterInstance(DataStore::DataStoreData *pDataStoreData) const
{
    // Return an instance of our BioSignalML data store exporter

    return new BiosignalmlDataStoreExporter(pDataStoreData);
}

//==============================================================================
// I18n interface
//==============================================================================

void BioSignalMLDataStorePlugin::retranslateUi()
{
    // We don't handle this interface...
    // Note: even though we don't handle this interface, we still want to
    //       support it since some other aspects of our plugin are
    //       multilingual...
}

//==============================================================================

}   // namespace BioSignalMLDataStore
}   // namespace OpenCOR

//==============================================================================
// End of file
//==============================================================================
