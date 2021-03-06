//=============================================================================
//
//                               OpenFlipper
//        Copyright (C) 2009 by Computer Graphics Group, RWTH Aachen
//                           www.openflipper.org
//
//-----------------------------------------------------------------------------
//
//                                License
//
//  OpenFlipper is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
// 
//  OpenFlipper is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
// 
//  You should have received a copy of the GNU Lesser General Public License
//  along with OpenFlipper.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------
//
//   $Revision: $
//   $Author: $
//   $Date: $
//
//=============================================================================

#ifndef TREEMODELOBJECTSELECTION_H
#define TREEMODELOBJECTSELECTION_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

#include "TreeItemObjectSelection.hh"

class TreeModelObjectSelection : public QAbstractItemModel
{
  Q_OBJECT

  signals:
    // the connected TreeView changed data
    void dataChangedInside(int _id, int _column, const QVariant& _value);       

  public:

    /// Constructor
    TreeModelObjectSelection(QObject *_parent = 0);

    /// Destructor
    ~TreeModelObjectSelection();

//===========================================================================
/** @name inherited from QAbstractItemModel
  * @{ */
//===========================================================================

public:

    /// Get the data of the corresponding entry
    QVariant data(const QModelIndex &index, int role) const;

    /// return the types of the corresponding entry
    Qt::ItemFlags flags(const QModelIndex &index) const;

    /// return the header data of the model
    QVariant headerData(int section, 
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;

    /// Get the ModelIndex at given row,column
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;

    /// Get the parent ModelIndex
    QModelIndex parent(const QModelIndex &index) const;

    /// get the number of rows
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    /// get the number of columns
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    /// Set the data at the given index
    bool setData(const QModelIndex &index, const QVariant &value , int role);

/** @} */

//===========================================================================
/** @name Internal DataStructure (the TreeItemObjectSelection Tree)
  * @{ */
//===========================================================================

public:

    /// Return the ModelIndex corresponding to a given TreeItemObjectSelection and Column
    QModelIndex getModelIndex(TreeItemObjectSelection* _object, int _column );

    /// Return the ModelIndex corresponding to a given object id and Column
    QModelIndex getModelIndex(int _id, int _column );

    /// Check if the given item is the root item
    bool isRoot(TreeItemObjectSelection* _item);

    /// Get the name of a given object
    bool getObjectName(TreeItemObjectSelection* _object , QString& _name);

    /// Get the TreeItemObjectSelection corresponding to a given ModelIndex
    TreeItemObjectSelection *getItem(const QModelIndex &index) const;

    /// Get the name of a TreeItemObjectSelection corresponding to a given ModelIndex
    QString itemName(const QModelIndex &index) const;

    /// Get the id of a TreeItemObjectSelection corresponding to a given ModelIndex
    int itemId(const QModelIndex &index) const;


    /// The object with the given id has been changed. Check if model also has to be changed
    void objectChanged(int id_);

    /// The object with the given id has been added. add it to the internal tree
    void objectAdded(BaseObject* _object);

     /// The object with the given id has been added. add it to the internal tree
    void objectAdded(BaseObject* _object, BaseObject* _parent);

    /// The object with the given id has been deleted. delete it from the internal tree
    void objectDeleted(int id_);

    /// move the item to a new parent
    void moveItem(TreeItemObjectSelection* _item, TreeItemObjectSelection* _parent );
private:

    /// Rootitem of the tree
    TreeItemObjectSelection* rootItem_;


    void propagateUpwards(TreeItemObjectSelection* _obj,   int _column, bool _value );
    void propagateDownwards(TreeItemObjectSelection* _obj, int _column );

/** @} */


};

#endif

