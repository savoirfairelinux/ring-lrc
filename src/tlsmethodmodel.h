/****************************************************************************
 *   Copyright (C) 2013-2015 by Savoir-Faire Linux                          *
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
#ifndef TLSMETHODMODEL_H
#define TLSMETHODMODEL_H

#include "typedefs.h"
#include <QtCore/QAbstractListModel>

class TlsMethodModelPrivate;

/**Static model for handling encryption types
 * 
 * This model can be used as a bridge between the Daemon API name (strings)
 * and combobox indexes. It can also be used to translate "default" to the
 * user locales
 */
class LIB_EXPORT TlsMethodModel : public QAbstractListModel {
   #pragma GCC diagnostic push
   #pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
   Q_OBJECT
   #pragma GCC diagnostic pop

public:
   ///@enum Type Every supported encryption types
   enum class Type {
      DEFAULT = 0,
      TLSv1   = 1,
      SSLv3   = 2,
      SSLv23  = 3,
   };

   class Name {
   public:
                static const char* DEFAULT            ;
      constexpr static const char* TLSv1   = "TLSv1"  ;
      constexpr static const char* SSLv3   = "SSLv3"  ;
      constexpr static const char* SSLv23  = "SSLv23" ;
   };

   class DaemonName {
   public:
      constexpr static const char* DEFAULT = "Default";
      constexpr static const char* TLSv1   = "TLSv1"  ;
      constexpr static const char* SSLv3   = "SSLv3"  ;
      constexpr static const char* SSLv23  = "SSLv23" ;
   };

   //Private constructor, can only be called by 'Account'
   explicit TlsMethodModel();

   //Model functions
   virtual QVariant      data     ( const QModelIndex& index, int role = Qt::DisplayRole     ) const override;
   virtual int           rowCount ( const QModelIndex& parent = QModelIndex()                ) const override;
   virtual Qt::ItemFlags flags    ( const QModelIndex& index                                 ) const override;
   virtual bool          setData  ( const QModelIndex& index, const QVariant &value, int role)       override;
   virtual QHash<int,QByteArray> roleNames() const override;

   //Getters
   QModelIndex toIndex (TlsMethodModel::Type type);
   static const char* toDaemonName(TlsMethodModel::Type type);
   static TlsMethodModel::Type fromDaemonName(const QString& name);

   //Singleton
   static TlsMethodModel* instance();

private:
   static TlsMethodModel* m_spInstance;

   TlsMethodModelPrivate* d_ptr;
   Q_DECLARE_PRIVATE(TlsMethodModel);

};
Q_DECLARE_METATYPE(TlsMethodModel*)
#endif
