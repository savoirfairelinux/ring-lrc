/****************************************************************************
 *   Copyright (C) 2015 by Savoir-Faire Linux                               *
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
#ifndef COLLECTIONEDITOR_H
#define COLLECTIONEDITOR_H
#include <QtCore/QAbstractItemModel>

#include <typedefs.h>

//Ring
class ContactMethod;
class CollectionEditorBasePrivate;

class LIB_EXPORT CollectionEditorBase
{
public:
   explicit CollectionEditorBase(QAbstractItemModel* m);
   QAbstractItemModel* model() const;

protected:
   QAbstractItemModel* m_pModel;

private:
   CollectionEditorBasePrivate* d_ptr;
   Q_DECLARE_PRIVATE(CollectionEditorBase)
};

template<typename T> class CollectionMediator;

/**
 * This is the class that does the actual work. This class
 * represent a delegate of CollectionInterface. It is usually
 * recommended to implement this as a private class in the .cpp
 * that implement the CollectionInterface.
 *
 * The rational behind this inversion of responsibility layer
 * is to avoid using a template class for CollectionInterface.
 * This would add obstable when implementing objects using it due
 * to C++11 lack of generic template polymorphism. A base class
 * extended by the template call also doesn't solve those issues.
 */
template<typename T>
class LIB_EXPORT CollectionEditor : public CollectionEditorBase {
   friend class CollectionInterface;
public:
   explicit CollectionEditor(CollectionMediator<T>* m);
   virtual ~CollectionEditor();

   CollectionMediator<T>* mediator() const;

   virtual bool save(const T* item) =0;
   virtual bool batchSave(const QList<T*> contacts);
   virtual bool batchRemove(const QList<T*> contacts);
   virtual bool remove(const T* item);

   ///Edit 'item', the implementation may be a GUI or something else
   virtual bool edit       ( T*       item     );

   ///Add a new item to the backend
   virtual bool addNew     (const  T*       item     ) = 0;

   ///Add an existing item to the collection
   virtual bool addExisting(const  T*       item     ) = 0;

   ///Add a new phone number to an existing item
   virtual bool addContactMethod( T*       item , ContactMethod* number );

private:
   /**
    * Return the items generated by this backend. This overloaded
    * version also make sure that the type is compatible.
    */
   virtual QVector<T*> items() const = 0;

   /**
    * Return the metatype of this editor
    */
   QMetaObject metaObject();

   //Attributes
   CollectionMediator<T>* m_pMediator;
};

#include "collectioneditor.hpp"

#endif
