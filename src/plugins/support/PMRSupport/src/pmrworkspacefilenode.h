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
// PMR workspace file node
//==============================================================================

#pragma once

//==============================================================================

#include "pmrsupportglobal.h"

//==============================================================================

#include <QObject>

//==============================================================================

namespace OpenCOR {
namespace PMRSupport {

//==============================================================================

class PmrWorkspaceFileNode;

//==============================================================================

typedef QPair<QChar, QChar> CharPair;
typedef QList<PmrWorkspaceFileNode *> PmrWorkspaceFileNodes;

//==============================================================================

class PMRSUPPORT_EXPORT PmrWorkspaceFileNode : public QObject
{
    Q_OBJECT

public:
    explicit PmrWorkspaceFileNode(const QString &pName, const QString &pPath,
                                  const CharPair &pStatus,
                                  PmrWorkspaceFileNode *pParent);
    explicit PmrWorkspaceFileNode(PmrWorkspaceFileNode *pParent);

    PmrWorkspaceFileNode * parent() const;

    QString name() const;

    QString path() const;
    void setPath(const QString &pPath);

    CharPair status() const;
    void setStatus(const CharPair &pStatus);

    bool hasChildren() const;
    PmrWorkspaceFileNodes children() const;
    PmrWorkspaceFileNode * addChild(const QString &pName,
                                    const CharPair &pStatus = CharPair());
    bool removeChild(PmrWorkspaceFileNode *pChild);

private:
    PmrWorkspaceFileNode *mParent;
    QString mName;
    QString mPath;
    CharPair mStatus;

    PmrWorkspaceFileNodes mChildren;

    void constructor(const QString &pName, const QString &pPath,
                     const CharPair &pStatus, PmrWorkspaceFileNode *pParent);
};

//==============================================================================

}   // namespace PMRSupport
}   // namespace OpenCOR

//==============================================================================
// End of file
//==============================================================================