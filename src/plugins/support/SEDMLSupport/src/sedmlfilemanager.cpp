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
// SED-ML file manager
//==============================================================================

#include "corecliutils.h"
#include "filemanager.h"
#include "sedmlfilemanager.h"
#include "sedmlsupportplugin.h"

//==============================================================================

#include "sedmlapidisablewarnings.h"
    #include "sedml/SedDocument.h"
    #include "sedml/SedReader.h"
#include "sedmlapienablewarnings.h"

//==============================================================================

namespace OpenCOR {
namespace SEDMLSupport {

//==============================================================================

SedmlFileManager * SedmlFileManager::instance()
{
    // Return the 'global' instance of our SED-ML file manager class

    static SedmlFileManager instance;

    return static_cast<SedmlFileManager *>(Core::globalInstance("OpenCOR::SEDMLSupport::SedmlFileManager",
                                                                &instance));
}

//==============================================================================

bool SedmlFileManager::isSedmlFile(const QString &pFileName) const
{
    // Return whether the given file is a SED-ML file

    return instance()->isFile(pFileName);
}

//==============================================================================

SedmlFile * SedmlFileManager::sedmlFile(const QString &pFileName)
{
    // Return the SedmlFile object, if any, associated with the given file

    return qobject_cast<SEDMLSupport::SedmlFile *>(instance()->file(pFileName));
}

//==============================================================================

bool SedmlFileManager::canLoadFileContents(const QString &pFileContents) const
{
    // Try to load the SED-ML file contents

    QByteArray fileContentsByteArray = pFileContents.toUtf8();

    libsedml::SedDocument *sedmlDocument = libsedml::readSedML(fileContentsByteArray.constData());

    return sedmlDocument->getNumErrors(libsedml::LIBSEDML_SEV_ERROR) == 0;
}

//==============================================================================

QObject * SedmlFileManager::newFile(const QString &pFileName) const
{
    // Create and return a new SED-ML file

    return new SedmlFile(Core::nativeCanonicalFileName(pFileName));
}

//==============================================================================

}   // namespace SEDMLSupport
}   // namespace OpenCOR

//==============================================================================
// End of file
//==============================================================================