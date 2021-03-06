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
// Standard file
//==============================================================================

#pragma once

//==============================================================================

#include "standardsupportglobal.h"

//==============================================================================

#include <QObject>
#include <QString>

//==============================================================================

namespace OpenCOR {
namespace StandardSupport {

//==============================================================================

class STANDARDSUPPORT_EXPORT StandardFile : public QObject
{
    Q_OBJECT

public:
    explicit StandardFile(const QString &pFileName);

    virtual bool load() = 0;
    bool reload();

    virtual bool save(const QString &pFileName = {});

    QString fileName() const;
    void setFileName(const QString &pFileName);

    bool isNew() const;

    bool isModified() const;
    void setModified(bool pModified) const;

protected:
    QString mFileName;
    bool mNew;

    virtual void reset() = 0;
};

//==============================================================================

} // namespace StandardSupport
} // namespace OpenCOR

//==============================================================================
// End of file
//==============================================================================
