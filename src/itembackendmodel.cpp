/****************************************************************************
 *   Copyright (C) 2014 by Savoir-Faire Linux                               *
 *   Author : Emmanuel Lepage Vallee <emmanuel.lepage@savoirfairelinux.com> *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Lesser General Public             *
 *   License as published by the Free Software Foundation; either           *
 *   version 2.1 of the License, or (at your option) any later version.     *
 *                                                                          *
 *   This library is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU General Public License      *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/
#include "itembackendmodel.h"

#include "commonbackendmanagerinterface.h"

CommonItemBackendModel::CommonItemBackendModel(QObject* parent) : QAbstractItemModel(parent)
{
   connect(ContactModel::instance(),SIGNAL(newBackendAdded(AbstractContactBackend*)),this,SLOT(slotUpdate()));
}

CommonItemBackendModel::~CommonItemBackendModel()
{
   while (m_lTopLevelBackends.size()) {
      ProxyItem* item = m_lTopLevelBackends[0];
      m_lTopLevelBackends.remove(0);
      while (item->m_Children.size()) {
         //FIXME I don't think it can currently happen, but there may be
         //more than 2 levels.
         ProxyItem* item2 = item->m_Children[0];
         item2->m_Children.remove(0);
         delete item2;
      }
      delete item;
   }
}

QVariant CommonItemBackendModel::data (const QModelIndex& idx, int role) const
{
   if (idx.isValid()) {
      ProxyItem* item = static_cast<ProxyItem*>(idx.internalPointer());
      switch(role) {
         case Qt::DisplayRole:
            return item->backend->name();
            break;
         case Qt::DecorationRole:
            return item->backend->icon();
            break;
         case Qt::CheckStateRole:
            return item->backend->isEnabled()?Qt::Checked:Qt::Unchecked;
      };
   }
   //else {
//       ProxyItem* item = static_cast<ProxyItem*>(idx.internalPointer());
//       return item->model->data(item->model->index(item->row,item->col));
   //}
   return QVariant();
}

int CommonItemBackendModel::rowCount (const QModelIndex& parent) const
{
   if (!parent.isValid()) {
      static bool init = false; //FIXME this doesn't allow dynamic backends
      static int result = 0;
      if (!init) {
         for(int i=0;i<ContactModel::instance()->backends().size();i++)
            result += ContactModel::instance()->backends()[i]->parentBackend()==nullptr?1:0;
         init = true;
      }
      return result;
   }
   else {
      ProxyItem* item = static_cast<ProxyItem*>(parent.internalPointer());
      return item->backend->childrenBackends().size();
   }
}

int CommonItemBackendModel::columnCount (const QModelIndex& parent) const
{
   Q_UNUSED(parent)
   return 1;
}

Qt::ItemFlags CommonItemBackendModel::flags(const QModelIndex& idx) const
{
   if (!idx.isValid())
      return 0;
   ProxyItem* item = static_cast<ProxyItem*>(idx.internalPointer());
   bool checkable = item->backend->supportedFeatures() & (AbstractContactBackend::SupportedFeatures::ENABLEABLE | 
   AbstractContactBackend::SupportedFeatures::DISABLEABLE | AbstractContactBackend::SupportedFeatures::MANAGEABLE  );
   return Qt::ItemIsEnabled | Qt::ItemIsSelectable | (checkable?Qt::ItemIsUserCheckable:Qt::NoItemFlags);
}

bool CommonItemBackendModel::setData (const QModelIndex& index, const QVariant &value, int role )
{
   Q_UNUSED(index)
   Q_UNUSED(value)
   Q_UNUSED(role)
   return false;
}

QModelIndex CommonItemBackendModel::parent( const QModelIndex& idx ) const
{
   if (idx.isValid()) {
      ProxyItem* item = static_cast<ProxyItem*>(idx.internalPointer());
      if (!item->parent)
         return QModelIndex();
      return createIndex(item->row,item->col,item->parent);
   }
   return QModelIndex();
}

QModelIndex CommonItemBackendModel::index( int row, int column, const QModelIndex& parent ) const
{
   if (parent.isValid()) {
      ProxyItem* parentItem = static_cast<ProxyItem*>(parent.internalPointer());
      ProxyItem* item = nullptr;
      if (row < parentItem->m_Children.size())
         item = parentItem->m_Children[row];
      else {
         item = new ProxyItem();
         item->parent = parentItem;
         item->backend = static_cast<AbstractContactBackend*>(parentItem->backend->childrenBackends()[row]);
         parentItem->m_Children << item;
      }
      item->row    = row;
      item->col    = column;
      return createIndex(row,column,item);
   }
   else { //Top level
      ProxyItem* item = nullptr;
      if (row < m_lTopLevelBackends.size())
         item = m_lTopLevelBackends[row];
      else {
         qDebug() << "C3" << item;
         item = new ProxyItem();
         item->backend = ContactModel::instance()->backends()[row];
         const_cast<CommonItemBackendModel*>(this)->m_lTopLevelBackends << item;
         qDebug() << "DANS IF";
      }
      item->row = row;
      item->col = column;
      return createIndex(item->row,item->col,item);
   }
}

void CommonItemBackendModel::slotUpdate()
{
   emit layoutChanged();
}
