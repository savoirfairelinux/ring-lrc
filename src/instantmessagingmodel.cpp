/****************************************************************************
 *   Copyright (C) 2012-2013 by Savoir-Faire Linux                          *
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
#include "instantmessagingmodel.h"

#include "callmodel.h"
#include "dbus/callmanager.h"
#include "call.h"
#include "contact.h"

InstantMessagingModelManager* InstantMessagingModelManager::m_spInstance  = nullptr;
CallModel*                InstantMessagingModelManager::m_spCallModel = nullptr;

///Signleton
InstantMessagingModelManager* InstantMessagingModelManager::instance()
{
   if (!m_spInstance) {
      m_spInstance = new InstantMessagingModelManager();
   }
   return m_spInstance;
}

void InstantMessagingModelManager::init(CallModel* model) {
   m_spCallModel = model;
   instance();
}

///Constructor
InstantMessagingModelManager::InstantMessagingModelManager() : QObject(0)
{
   CallManagerInterface& callManager = DBus::CallManager::instance();
   connect(&callManager, SIGNAL(incomingMessage(QString,QString,QString)), this, SLOT(newMessage(QString,QString,QString)));
}

///Called when a new message is incoming
void InstantMessagingModelManager::newMessage(QString callId, QString from, QString message)
{
   if (!m_lModels[callId] && m_spCallModel) {
      Call* call = m_spCallModel->getCall(callId);
      if (call) {
         qDebug() << "Creating messaging model for call" << callId;
         m_lModels[callId] = new InstantMessagingModel(call);
         emit newMessagingModel(call,m_lModels[callId]);
         m_lModels[callId]->addIncommingMessage(from,message);
      }
   }
   else if (m_lModels[callId]) {
      m_lModels[callId]->addIncommingMessage(from,message);
   }
}

///Singleton
InstantMessagingModel* InstantMessagingModelManager::getModel(Call* call) {
   QString key = call->isConference()?call->getConfId():call->getCallId();
   if (!m_lModels[key]) {
      m_lModels[key] = new InstantMessagingModel(call);
      emit newMessagingModel(call,m_lModels[key]);
   }
   return m_lModels[key];
}

///Constructor
InstantMessagingModel::InstantMessagingModel(Call* call, QObject* par) : QAbstractListModel(par),m_pCall(call)
{
   //QStringList callList = callManager.getCallList();
}

///Get data from the model
QVariant InstantMessagingModel::data( const QModelIndex& idx, int role) const
{
   if (idx.column() == 0) {
      switch (role) {
         case Qt::DisplayRole:
            return QVariant(m_lMessages[idx.row()].message);
            break;
         case MESSAGE_TYPE_ROLE:
            return QVariant(m_lMessages[idx.row()].message);
            break;
         case MESSAGE_FROM_ROLE:
            return QVariant(m_lMessages[idx.row()].from);
            break;
         case MESSAGE_TEXT_ROLE:
            return INCOMMING_IM;
            break;
         case MESSAGE_CONTACT_ROLE:
            if (m_pCall->getContact()) {
               return QVariant();
            }
            break;
         case MESSAGE_IMAGE_ROLE: {
            if (m_lImages.find(idx) != m_lImages.end())
               return m_lImages[idx];
            Contact* c =m_pCall->getContact();
            if (c && c->getPhoto()) {
               return QVariant::fromValue<void*>((void*)c->getPhoto());
            }
            return QVariant();
            break;
         }
         default:
            break;
      }
   }
   return QVariant();
}

///Number of row
int InstantMessagingModel::rowCount(const QModelIndex& parentIdx) const
{
   Q_UNUSED(parentIdx)
   return m_lMessages.size();
}

///Model flags
Qt::ItemFlags InstantMessagingModel::flags(const QModelIndex& idx) const
{
   Q_UNUSED(idx)
   return Qt::ItemIsEnabled;
}

///Set model data
bool InstantMessagingModel::setData(const QModelIndex& idx, const QVariant &value, int role)
{
   Q_UNUSED(idx)
   Q_UNUSED(value)
   Q_UNUSED(role)
   if (idx.column() == 0 && role == MESSAGE_IMAGE_ROLE   ) {
      m_lImages[idx] = value;
   }
   return false;
}

///Add new incoming message (to be used internally)
void InstantMessagingModel::addIncommingMessage(QString from, QString message)
{
   InternalIM im;
   im.from    = from;
   im.message = message;
   m_lMessages << im;
   emit dataChanged(index(m_lMessages.size() -1,0), index(m_lMessages.size()-1,0));
}

///Add new outgoing message (to be used internally and externally)
void InstantMessagingModel::addOutgoingMessage(QString message)
{
   InternalIM im;
   im.from    = "Me";
   im.message = message;
   m_lMessages << im;
   emit dataChanged(index(m_lMessages.size() -1,0), index(m_lMessages.size()-1,0));
}
