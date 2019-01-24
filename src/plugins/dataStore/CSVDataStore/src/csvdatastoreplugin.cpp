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
// CSV data store plugin
//==============================================================================

#include "corecliutils.h"
#include "coreguiutils.h"
#include "csvdatastoreexporter.h"
#include "csvdatastoreimporter.h"
#include "csvdatastoreplugin.h"
#include "csvinterface.h"
#include "datastoredialog.h"

//==============================================================================

#include <QFile>
#include <QMainWindow>
#include <QSettings>
#include <QTextStream>

//==============================================================================

namespace OpenCOR {
namespace CSVDataStore {

//==============================================================================

PLUGININFO_FUNC CSVDataStorePluginInfo()
{
    Descriptions descriptions;

    descriptions.insert("en", QString::fromUtf8("a CSV specific data store plugin."));
    descriptions.insert("fr", QString::fromUtf8("une extension de magasin de données spécifique à CSV."));

    return new PluginInfo(PluginInfo::DataStore, true, false,
                          QStringList() << "DataStore",
                          descriptions);
}

//==============================================================================

CSVDataStorePlugin::CSVDataStorePlugin()
{
    // Keep track of our file type interface

    static CsvInterfaceData data(qobject_cast<FileTypeInterface *>(this));

    Core::globalInstance(CsvInterfaceDataSignature, &data);
}

//==============================================================================
// Data store interface
//==============================================================================

QString CSVDataStorePlugin::dataStoreName() const
{
    // Return the name of the data store

    return "CSV";
}

//==============================================================================

DataStore::DataStoreImportData * CSVDataStorePlugin::getImportData(const QString &pFileName,
                                                                   DataStore::DataStore *pDataStore) const
{
    // Determine the number of variables in our CSV file

    DataStore::DataStoreImportData *res = nullptr;
    QFile file(pFileName);

    if (file.open(QIODevice::ReadOnly|QIODevice::Text)) {
        // Determine our number of variables and data points
        // Note #1: we subtract 1 for our number of variables because otherwise
        //          it would include the VOI, which we don't want...
        // Note #2: nbOfDataPoints starts at -1 because we are going to count
        //          the header of our CSV file...

        QTextStream in(&file);
        QString line;
        quint64 nbOfDataPoints = quint64(-1);

        while (!in.atEnd()) {
            line = in.readLine().trimmed();

            if (!line.isEmpty())
                ++nbOfDataPoints;
        }

        in.seek(0);

        res = new DataStore::DataStoreImportData(pFileName, pDataStore, nullptr,
                                                 in.readLine().trimmed().split(",").count()-1,
                                                 nbOfDataPoints);

        file.close();
    }

    // Return some information about the data we want to import

    return res;
}

//==============================================================================

DataStore::DataStoreExportData * CSVDataStorePlugin::getExportData(const QString &pFileName,
                                                                   DataStore::DataStore *pDataStore,
                                                                   const QMap<int, QIcon> &pIcons) const
{
    // Ask which data should be exported

    DataStore::DataStoreDialog dataStoreDialog(pDataStore, true, pIcons, Core::mainWindow());
    QSettings settings;

    settings.beginGroup(SettingsPlugins);
    settings.beginGroup("CSVDataStore");
    settings.beginGroup("DataStoreDialog");

    if (dataStoreDialog.exec(&settings)) {
        // Now that we know which data to export, we can ask for the name of the
        // CSV file where it is to be exported

        QStringList csvFilters = Core::filters(FileTypeInterfaces() << CSVDataStore::fileTypeInterface());
        QString firstCsvFilter = csvFilters.first();
        QString fileName = Core::getSaveFileName(tr("Export To CSV"),
                                                 Core::newFileName(pFileName, tr("Data"), false, CsvFileExtension),
                                                 csvFilters, &firstCsvFilter);

        if (!fileName.isEmpty())
            return new DataStore::DataStoreExportData(fileName, pDataStore, dataStoreDialog.selectedData());
    }

    return nullptr;
}

//==============================================================================

DataStore::DataStoreImporter * CSVDataStorePlugin::dataStoreImporterInstance() const
{
    // Return the 'global' instance of our CSV data store importer

    static CsvDataStoreImporter instance;

    return static_cast<CsvDataStoreImporter *>(Core::globalInstance("OpenCOR::CSVDataStore::CsvDataStoreImporter::instance()",
                                                                    &instance));
}

//==============================================================================

DataStore::DataStoreExporter * CSVDataStorePlugin::dataStoreExporterInstance() const
{
    // Return the 'global' instance of our CSV data store exporter

    static CsvDataStoreExporter instance;

    return static_cast<CsvDataStoreExporter *>(Core::globalInstance("OpenCOR::CSVDataStore::CsvDataStoreExporter::instance()",
                                                                    &instance));
}

//==============================================================================
// File interface
//==============================================================================

bool CSVDataStorePlugin::isFile(const QString &pFileName) const
{
    // Return whether the given file is of the type that we support

    QFile file(pFileName);

    if (file.open(QIODevice::ReadOnly|QIODevice::Text)) {
        QTextStream in(&file);
        bool emptyLine = false;
        bool needNbOfFields = true;
        int nbOfFields = 0;

        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();

            if (line.isEmpty()) {
                // The line is empty, which is fine, but only if it is the last
                // line of the file

                if (emptyLine)
                    return false;
                else
                    emptyLine = true;
            } else if (line.endsWith(",")) {
                // The line ends with a comma, which is not allowed

                return false;
            } else {
                // Make sure that the line has the same number of fields (and at
                // least two of them) as the other lines (with the fields being
                // separated by commas)

                int crtNbOfFields = line.split(",").count();

                if (needNbOfFields) {
                    nbOfFields = crtNbOfFields;

                    if (nbOfFields == 1)
                        return false;

                    needNbOfFields = false;
                } else if (crtNbOfFields != nbOfFields) {
                    return false;
                }
            }
        }

        file.close();

        return true;
    } else {
        return false;
    }
}

//==============================================================================

QString CSVDataStorePlugin::mimeType() const
{
    // Return the MIME type we support

    return CsvMimeType;
}

//==============================================================================

QString CSVDataStorePlugin::fileExtension() const
{
    // Return the extension of the type of file we support

    return CsvFileExtension;
}

//==============================================================================

QString CSVDataStorePlugin::fileTypeDescription() const
{
    // Return the description of the type of file we support

    return tr("CSV File");
}

//==============================================================================

QStringList CSVDataStorePlugin::fileTypeDefaultViews() const
{
    // Return the default views to use for the type of file we support

    return QStringList();
}

//==============================================================================
// I18n interface
//==============================================================================

void CSVDataStorePlugin::retranslateUi()
{
    // We don't handle this interface...
    // Note: even though we don't handle this interface, we still want to
    //       support it since some other aspects of our plugin are
    //       multilingual...
}

//==============================================================================

}   // namespace CSVDataStore
}   // namespace OpenCOR

//==============================================================================
// End of file
//==============================================================================
