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
// Workspaces widget
//==============================================================================

#pragma once

//==============================================================================

#include "filemanager.h"
#include "pmrworkspacesmodel.h"
#include "pmrworkspace.h"
#include "treeviewwidget.h"

//==============================================================================

#include <QMap>
#include <QModelIndex>
//#include <QPersistentModelIndex>
#include <QStyledItemDelegate>

//==============================================================================

namespace OpenCOR {

//==============================================================================

namespace PMRSupport {
    class PmrRepository;
}   // namespace PMRSupport

//==============================================================================

namespace PMRWorkspaces {

class PmrWorkspacesWidget;

//==============================================================================

class PmrWorkspacesItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    PmrWorkspacesItemDelegate(PmrWorkspacesWidget *pWidget, QObject *parent = 0);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;

private:
    PmrWorkspacesWidget *mWidget;
};

//==============================================================================

class PmrWorkspacesWidget : public Core::TreeViewWidget
{
    Q_OBJECT

public:
    explicit PmrWorkspacesWidget(PMRSupport::PmrRepository *pPmrRepository, QWidget *pParent);
    ~PmrWorkspacesWidget();

//    Qt::ItemFlags flags(const QModelIndex &index) const;

    virtual void loadSettings(QSettings *pSettings);
    virtual void saveSettings(QSettings *pSettings) const;

//    void newFolder();
//    void deleteItems();

//    QString filePath(const QModelIndex &pFileIndex) const;

protected:
//    virtual void keyPressEvent(QKeyEvent *pEvent);
    virtual void drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const;
    void resizeEvent(QResizeEvent *event);
    
private:
    PmrWorkspacesModel *mModel;

    PMRSupport::PmrRepository *mPmrRepository;

//    QMap<QString, QPersistentModelIndex> mUrlIndexMap;

//    Core::FileManager *mFileManager;

    void loadItemSettings(QSettings *pSettings, QStandardItem *pParentItem);
    void saveItemSettings(QSettings *pSettings, QStandardItem *pItem,
                          const int &pParentItemIndex) const;

//    QString newFolderName(QStandardItem *pFolderItem) const;
//    void collapseEmptyFolders(QStandardItem *pFolder);

//    QStandardItem *parentItem(QStandardItem *pDropItem,
//                              const QAbstractItemView::DropIndicatorPosition &pDropPosition);

//    bool ownedBy(const QString &pFileName, QStandardItem *pItem);

//    void dropItems(QStandardItem *pDropItem,
//                   const QAbstractItemView::DropIndicatorPosition &pDropPosition,
//                   QStandardItem *pNewParentItem, QList<QStandardItem *> pItems);
//    void dropItem(QStandardItem *pDropItem,
//                  const QAbstractItemView::DropIndicatorPosition &pDropPosition,
//                  QStandardItem *pNewParentItem, QStandardItem *pItem);

//    void addFile(const QString &pFileName, QStandardItem *pDropItem,
//                 const QAbstractItemView::DropIndicatorPosition &pDropPosition);
//
//    void moveItem(QStandardItem *pItem, QStandardItem *pDropItem,
//                  const QAbstractItemView::DropIndicatorPosition &pDropPosition);

    //void resizeToContents();

//    QStringList selectedFiles() const;

//    bool parentIndexExists(const QModelIndex &pIndex,
//                           const QModelIndexList &pIndexes) const;

//    QModelIndexList cleanIndexList(const QModelIndexList &pIndexes) const;

//    bool itemIsOrIsChildOf(QStandardItem *pItem, QStandardItem *pOtherItem) const;

//    void backupExpandedInformation(QStandardItem *pItem) const;
//    void restoreExpandedInformation(QStandardItem *pItem);

//    void updateFileItems(QStandardItem *pItem, const QString &pFileName,
//                         const Core::File::Status &pStatus) const;

//    bool canCreateNewFolder() const;

Q_SIGNALS:
//    void filesOpenRequested(const QStringList &pFileNames);

//    void newFolderEnabled(const bool &pEnabled);
//    void deleteItemsEnabled(const bool &pEnabled);

public Q_SLOTS:
    void clearWorkspaces(void);
    void displayWorkspaces(const PMRSupport::PmrWorkspaces &pWorkspaces);
    void refreshWorkspaces(void);

private Q_SLOTS:
    void on_clicked(const QModelIndex &pItemIndex);
//    void expandedFolder(const QModelIndex &pFolderIndex);
//    void collapsedFolder(const QModelIndex &pFolderIndex);

//    void emitItemsRelatedSignals();

//    void fileChanged(const QString &pFileName) const;
//    void fileDeleted(const QString &pFileName) const;
};

//==============================================================================

}   // namespace PMRWorkspaces
}   // namespace OpenCOR

//==============================================================================
// End of file
//==============================================================================
