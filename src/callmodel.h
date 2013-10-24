/****************************************************************************
 *   Copyright (C) 2009-2013 by Savoir-Faire Linux                          *
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

#ifndef CALL_MODEL2_H
#define CALL_MODEL2_H

#include <QAbstractItemModel>
#include <QMap>
#include "typedefs.h"

//SFLPhone
#include "call.h"
class Account;
struct InternalStruct;
class PhoneNumber;

//Typedef
typedef QMap<uint, Call*>  CallMap;
typedef QList<Call*>       CallList;

///CallModel: Central model/frontend to deal with sflphoned
class LIB_EXPORT CallModel : public QAbstractItemModel
{
   #pragma GCC diagnostic push
   #pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
   Q_OBJECT
   #pragma GCC diagnostic pop
   public:
      ///Accepted (mime) payload types
      enum DropPayloadType {
         NONE    = 0   ,
         CALL    = 1<<0,
         HISTORY = 1<<1,
         CONTACT = 1<<2,
         NUMBER  = 1<<3,
         TEXT    = 1<<4,
      };

      //Constructors, initializer and destructors
      virtual ~CallModel( );

      //Call related
      Q_INVOKABLE Call* addDialingCall   ( const QString& peerName=QString(), Account* account=nullptr );
      Q_INVOKABLE void  attendedTransfer ( Call* toTransfer , Call* target              );
      Q_INVOKABLE void  transfer         ( Call* toTransfer , const PhoneNumber* target );
      Q_INVOKABLE void  removeCall       ( Call* call                                   );
      QModelIndex getIndex               ( Call* call                                   );

      //Conference related
      Q_INVOKABLE bool createConferenceFromCall ( Call* call1, Call* call2      );
      Q_INVOKABLE bool mergeConferences         ( Call* conf1, Call* conf2      );
      Q_INVOKABLE bool addParticipant           ( Call* call2, Call* conference );
      Q_INVOKABLE bool detachParticipant        ( Call* call                    );
      Q_INVOKABLE void removeConference         ( Call* conf                    );

      //Getters
      Q_INVOKABLE bool     isValid             ();
      Q_INVOKABLE int      size                ();
      Q_INVOKABLE CallList getCallList         ();
      Q_INVOKABLE CallList getConferenceList   ();
      Q_INVOKABLE int      acceptedPayloadTypes();

      //Model implementation
      virtual bool          setData      ( const QModelIndex& index, const QVariant &value, int role   );
      virtual QVariant      data         ( const QModelIndex& index, int role = Qt::DisplayRole        ) const;
      virtual int           rowCount     ( const QModelIndex& parent = QModelIndex()                   ) const;
      virtual Qt::ItemFlags flags        ( const QModelIndex& index                                    ) const;
      virtual int           columnCount  ( const QModelIndex& parent = QModelIndex()                   ) const __attribute__ ((const));
      virtual QModelIndex   parent       ( const QModelIndex& index                                    ) const;
      virtual QModelIndex   index        ( int row, int column, const QModelIndex& parent=QModelIndex()) const;
      virtual QVariant      headerData   ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
      virtual QStringList   mimeTypes    (                                                             ) const;
      virtual QMimeData*    mimeData     ( const QModelIndexList &indexes                              ) const;
      virtual bool          dropMimeData ( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent );

      //Singleton
      static CallModel* instance();

      Q_INVOKABLE Call* getCall ( const QString& callId  ) const;
      Q_INVOKABLE Call* getCall ( const QModelIndex& idx ) const;

   private:
      explicit CallModel();
      void init();
      Call* addCall          ( Call* call                , Call* parent = nullptr );
      Call* addConference    ( const QString& confID                              );
      bool  changeConference ( const QString& confId, const QString& state        );
      void  removeConference ( const QString& confId                              );
      Call* addIncomingCall  ( const QString& callId                              );
      Call* addRingingCall   ( const QString& callId                              );

      //Attributes
      QList<InternalStruct*> m_lInternalModel;
      QHash< Call*       , InternalStruct* > m_sPrivateCallList_call   ;
      QHash< QString     , InternalStruct* > m_sPrivateCallList_callId ;

      //Singleton
      static CallModel* m_spInstance;

      //Helpers
      bool isPartOf(const QModelIndex& confIdx, Call* call);
      void initRoles();

   private Q_SLOTS:
      void slotCallStateChanged   ( const QString& callID    , const QString &state   );
      void slotIncomingCall       ( const QString& accountID , const QString & callID );
      void slotIncomingConference ( const QString& confID                             );
      void slotChangingConference ( const QString& confID    , const QString &state   );
      void slotConferenceRemoved  ( const QString& confId                             );
      void slotVoiceMailNotify    ( const QString& accountID , int count              );
      void slotVolumeChanged      ( const QString& device    , double value           );
      void slotAddPrivateCall     ( Call* call                                        );
      void slotNewRecordingAvail  ( const QString& callId    , const QString& filePath);
      void slotCallChanged        ( Call* call                                        );
      void slotDTMFPlayed         ( const QString& str                                );
      void slotRecordStateChanged ( const QString& callId    , bool state             );
      #ifdef ENABLE_VIDEO
      void slotStartedDecoding    ( const QString& callId    , const QString& shmKey  );
      void slotStoppedDecoding    ( const QString& callId    , const QString& shmKey  );
      #endif

   Q_SIGNALS:
      ///Emitted when a call state change
      void callStateChanged        ( Call* call, Call::State previousState   );
      ///Emitted when a new call is incoming
      void incomingCall            ( Call* call                              );
      ///Emitted when a conference is created
      void conferenceCreated       ( Call* conf                              );
      ///Emitted when a conference change state or participant
      void conferenceChanged       ( Call* conf                              );
      ///Emitted when a conference is removed
      void conferenceRemoved       ( Call* conf                              );
      ///Emitted just before a conference is removed
      void aboutToRemoveConference ( Call* conf                              );
      ///Emitted when a new voice mail is available
      void voiceMailNotify         ( const QString& accountID , int    count );
      ///Emitted when the volume change
      void volumeChanged           ( const QString& device    , double value );
      ///Emitted when a call is added
      void callAdded               ( Call* call               , Call* parent );
};
Q_DECLARE_METATYPE(CallModel*)

#endif
