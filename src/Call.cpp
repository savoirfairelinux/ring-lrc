/***************************************************************************
 *   Copyright (C) 2009-2010 by Savoir-Faire Linux                         *
 *   Author : Jérémy Quentin <jeremy.quentin@savoirfairelinux.com>         *
 *            Emmanuel Lepage Valle <emmanuel.lepage@savoirfairelinux.com >*
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 **************************************************************************/
#include "Call.h"

#include "CallModel.h"

#include "callmanager_interface_singleton.h"
#include "configurationmanager_interface_singleton.h"
#include "ContactBackend.h"
#include "Contact.h"


const call_state Call::actionPerformedStateMap [11][5] = 
{
//                      ACCEPT                  REFUSE                  TRANSFER                   HOLD                           RECORD            /**/
/*INCOMING     */  {CALL_STATE_INCOMING   , CALL_STATE_INCOMING    , CALL_STATE_ERROR        , CALL_STATE_INCOMING     ,  CALL_STATE_INCOMING     },/**/
/*RINGING      */  {CALL_STATE_ERROR      , CALL_STATE_RINGING     , CALL_STATE_ERROR        , CALL_STATE_ERROR        ,  CALL_STATE_RINGING      },/**/
/*CURRENT      */  {CALL_STATE_ERROR      , CALL_STATE_CURRENT     , CALL_STATE_TRANSFER     , CALL_STATE_CURRENT      ,  CALL_STATE_CURRENT      },/**/
/*DIALING      */  {CALL_STATE_DIALING    , CALL_STATE_OVER        , CALL_STATE_ERROR        , CALL_STATE_ERROR        ,  CALL_STATE_ERROR        },/**/
/*HOLD         */  {CALL_STATE_ERROR      , CALL_STATE_HOLD        , CALL_STATE_TRANSF_HOLD  , CALL_STATE_HOLD         ,  CALL_STATE_HOLD         },/**/
/*FAILURE      */  {CALL_STATE_ERROR      , CALL_STATE_FAILURE     , CALL_STATE_ERROR        , CALL_STATE_ERROR        ,  CALL_STATE_ERROR        },/**/
/*BUSY         */  {CALL_STATE_ERROR      , CALL_STATE_BUSY        , CALL_STATE_ERROR        , CALL_STATE_ERROR        ,  CALL_STATE_ERROR        },/**/
/*TRANSFER     */  {CALL_STATE_TRANSFER   , CALL_STATE_TRANSFER    , CALL_STATE_CURRENT      , CALL_STATE_TRANSFER     ,  CALL_STATE_TRANSFER     },/**/
/*TRANSF_HOLD  */  {CALL_STATE_TRANSF_HOLD, CALL_STATE_TRANSF_HOLD , CALL_STATE_HOLD         , CALL_STATE_TRANSF_HOLD  ,  CALL_STATE_TRANSF_HOLD  },/**/
/*OVER         */  {CALL_STATE_ERROR      , CALL_STATE_ERROR       , CALL_STATE_ERROR        , CALL_STATE_ERROR        ,  CALL_STATE_ERROR        },/**/
/*ERROR        */  {CALL_STATE_ERROR      , CALL_STATE_ERROR       , CALL_STATE_ERROR        , CALL_STATE_ERROR        ,  CALL_STATE_ERROR        } /**/
};//                                                                                                                                                    


const function Call::actionPerformedFunctionMap[11][5] = 
{ 
//                      ACCEPT               REFUSE            TRANSFER                 HOLD                  RECORD             /**/
/*INCOMING       */  {&Call::accept     , &Call::refuse   , &Call::acceptTransf   , &Call::acceptHold  ,  &Call::setRecord     },/**/
/*RINGING        */  {&Call::nothing    , &Call::hangUp   , &Call::nothing        , &Call::nothing     ,  &Call::setRecord     },/**/
/*CURRENT        */  {&Call::nothing    , &Call::hangUp   , &Call::nothing        , &Call::hold        ,  &Call::setRecord     },/**/
/*DIALING        */  {&Call::call       , &Call::cancel   , &Call::nothing        , &Call::nothing     ,  &Call::nothing       },/**/
/*HOLD           */  {&Call::nothing    , &Call::hangUp   , &Call::nothing        , &Call::unhold      ,  &Call::setRecord     },/**/
/*FAILURE        */  {&Call::nothing    , &Call::hangUp   , &Call::nothing        , &Call::nothing     ,  &Call::nothing       },/**/
/*BUSY           */  {&Call::nothing    , &Call::hangUp   , &Call::nothing        , &Call::nothing     ,  &Call::nothing       },/**/
/*TRANSFERT      */  {&Call::transfer   , &Call::hangUp   , &Call::nothing        , &Call::hold        ,  &Call::setRecord     },/**/
/*TRANSFERT_HOLD */  {&Call::transfer   , &Call::hangUp   , &Call::nothing        , &Call::unhold      ,  &Call::setRecord     },/**/
/*OVER           */  {&Call::nothing    , &Call::nothing  , &Call::nothing        , &Call::nothing     ,  &Call::nothing       },/**/
/*ERROR          */  {&Call::nothing    , &Call::nothing  , &Call::nothing        , &Call::nothing     ,  &Call::nothing       } /**/
};//                                                                                                                                 


const call_state Call::stateChangedStateMap [11][6] = 
{
//                      RINGING                  CURRENT             BUSY              HOLD                           HUNGUP           FAILURE             /**/
/*INCOMING     */ {CALL_STATE_INCOMING    , CALL_STATE_CURRENT  , CALL_STATE_BUSY   , CALL_STATE_HOLD         ,  CALL_STATE_OVER  ,  CALL_STATE_FAILURE  },/**/
/*RINGING      */ {CALL_STATE_RINGING     , CALL_STATE_CURRENT  , CALL_STATE_BUSY   , CALL_STATE_HOLD         ,  CALL_STATE_OVER  ,  CALL_STATE_FAILURE  },/**/
/*CURRENT      */ {CALL_STATE_CURRENT     , CALL_STATE_CURRENT  , CALL_STATE_BUSY   , CALL_STATE_HOLD         ,  CALL_STATE_OVER  ,  CALL_STATE_FAILURE  },/**/
/*DIALING      */ {CALL_STATE_RINGING     , CALL_STATE_CURRENT  , CALL_STATE_BUSY   , CALL_STATE_HOLD         ,  CALL_STATE_OVER  ,  CALL_STATE_FAILURE  },/**/
/*HOLD         */ {CALL_STATE_HOLD        , CALL_STATE_CURRENT  , CALL_STATE_BUSY   , CALL_STATE_HOLD         ,  CALL_STATE_OVER  ,  CALL_STATE_FAILURE  },/**/
/*FAILURE      */ {CALL_STATE_FAILURE     , CALL_STATE_FAILURE  , CALL_STATE_BUSY   , CALL_STATE_FAILURE      ,  CALL_STATE_OVER  ,  CALL_STATE_FAILURE  },/**/
/*BUSY         */ {CALL_STATE_BUSY        , CALL_STATE_CURRENT  , CALL_STATE_BUSY   , CALL_STATE_BUSY         ,  CALL_STATE_OVER  ,  CALL_STATE_FAILURE  },/**/
/*TRANSFER     */ {CALL_STATE_TRANSFER    , CALL_STATE_TRANSFER , CALL_STATE_BUSY   , CALL_STATE_TRANSF_HOLD  ,  CALL_STATE_OVER  ,  CALL_STATE_FAILURE  },/**/
/*TRANSF_HOLD  */ {CALL_STATE_TRANSF_HOLD , CALL_STATE_TRANSFER , CALL_STATE_BUSY   , CALL_STATE_TRANSF_HOLD  ,  CALL_STATE_OVER  ,  CALL_STATE_FAILURE  },/**/
/*OVER         */ {CALL_STATE_OVER        , CALL_STATE_OVER     , CALL_STATE_OVER   , CALL_STATE_OVER         ,  CALL_STATE_OVER  ,  CALL_STATE_OVER     },/**/
/*ERROR        */ {CALL_STATE_ERROR       , CALL_STATE_ERROR    , CALL_STATE_ERROR  , CALL_STATE_ERROR        ,  CALL_STATE_ERROR ,  CALL_STATE_ERROR    } /**/
};//                                                                                                                                                           

const function Call::stateChangedFunctionMap[11][6] = 
{ 
//                      RINGING                  CURRENT             BUSY              HOLD                    HUNGUP           FAILURE            /**/
/*INCOMING       */  {&Call::nothing    , &Call::start     , &Call::startWeird     , &Call::startWeird   ,  &Call::startStop    , &Call::start   },/**/
/*RINGING        */  {&Call::nothing    , &Call::start     , &Call::start          , &Call::start        ,  &Call::startStop    , &Call::start   },/**/
/*CURRENT        */  {&Call::nothing    , &Call::nothing   , &Call::warning        , &Call::nothing      ,  &Call::stop         , &Call::nothing },/**/
/*DIALING        */  {&Call::nothing    , &Call::warning   , &Call::warning        , &Call::warning      ,  &Call::stop         , &Call::warning },/**/
/*HOLD           */  {&Call::nothing    , &Call::nothing   , &Call::warning        , &Call::nothing      ,  &Call::stop         , &Call::nothing },/**/
/*FAILURE        */  {&Call::nothing    , &Call::warning   , &Call::warning        , &Call::warning      ,  &Call::stop         , &Call::nothing },/**/
/*BUSY           */  {&Call::nothing    , &Call::nothing   , &Call::nothing        , &Call::warning      ,  &Call::stop         , &Call::nothing },/**/
/*TRANSFERT      */  {&Call::nothing    , &Call::nothing   , &Call::warning        , &Call::nothing      ,  &Call::stop         , &Call::nothing },/**/
/*TRANSFERT_HOLD */  {&Call::nothing    , &Call::nothing   , &Call::warning        , &Call::nothing      ,  &Call::stop         , &Call::nothing },/**/
/*OVER           */  {&Call::nothing    , &Call::warning   , &Call::warning        , &Call::warning      ,  &Call::stop         , &Call::warning },/**/
/*ERROR          */  {&Call::nothing    , &Call::nothing   , &Call::nothing        , &Call::nothing      ,  &Call::stop         , &Call::nothing } /**/
};//                                                                                                                                                   

const char * Call::historyIcons[3] = {ICON_HISTORY_INCOMING, ICON_HISTORY_OUTGOING, ICON_HISTORY_MISSED};

ContactBackend* Call::m_pContactBackend = 0;

void Call::setContactBackend(ContactBackend* be)
{
   m_pContactBackend = be;
}

///Constructor
Call::Call(call_state startState, QString callId, QString peerName, QString peerNumber, QString account)
   : conference(false)
{
   this->m_pCallId          = callId     ;
   this->m_pPeerPhoneNumber = peerNumber ;
   this->m_pPeerName        = peerName   ;
   changeCurrentState(startState)        ;
   this->m_pAccount         = account    ;
   this->recording          = false      ;
   this->m_pStartTime       = NULL       ;
   this->m_pStopTime        = NULL       ;
   emit changed();
}

///Destructor
Call::~Call()
{
   delete m_pStartTime ;
   delete m_pStopTime  ;
}

///Constructor
Call::Call(QString confId, QString account) 
   : conference(true)
{
   this->m_pConfId  = confId  ;
   this->m_pAccount = account ;
}

/*****************************************************************************
 *                                                                           *
 *                               Call builder                                *
 *                                                                           *
 ****************************************************************************/

///Build a call from its ID
Call* Call::buildExistingCall(QString callId)
{
   CallManagerInterface & callManager = CallManagerInterfaceSingleton::getInstance();
   MapStringString details = callManager.getCallDetails(callId).value();
   
   qDebug() << "Constructing existing call with details : " << details;
   
   QString    peerNumber = details[ CALL_PEER_NUMBER ];
   QString    peerName   = details[ CALL_PEER_NAME   ];
   QString    account    = details[ CALL_ACCOUNTID   ];
   call_state startState = getStartStateFromDaemonCallState(details[CALL_STATE], details[CALL_TYPE]);
   
   Call* call            = new Call(startState, callId, peerName, peerNumber, account)                 ;
   call->m_pStartTime    = new QDateTime(QDateTime::currentDateTime())                                 ;
   call->recording       = callManager.getIsRecording(callId)                                          ;
   call->m_pHistoryState = getHistoryStateFromDaemonCallState(details[CALL_STATE], details[CALL_TYPE]) ;
   
   return call;
}

///Build a call from a dialing call (a call that is about to exist)
Call* Call::buildDialingCall(QString callId, const QString & peerName, QString account)
{
   Call* call = new Call(CALL_STATE_DIALING, callId, peerName, "", account);
   call->m_pHistoryState = NONE;
   return call;
}

///Build a call from a dbus event
Call* Call::buildIncomingCall(const QString & callId)
{
   CallManagerInterface & callManager = CallManagerInterfaceSingleton::getInstance();
   MapStringString details = callManager.getCallDetails(callId).value();
   
   qDebug() << "details = " << details;
   
   QString from     = details[ CALL_PEER_NUMBER ];
   QString account  = details[ CALL_ACCOUNTID   ];
   QString peerName = details[ CALL_PEER_NAME   ];
   
   Call* call = new Call(CALL_STATE_INCOMING, callId, peerName, from, account);
   call->m_pHistoryState = MISSED;
   return call;
}

///Build a rigging call (from dbus)
Call* Call::buildRingingCall(const QString & callId)
{
   CallManagerInterface& callManager = CallManagerInterfaceSingleton::getInstance();
   MapStringString details = callManager.getCallDetails(callId).value();
   
   QString from     = details[ CALL_PEER_NUMBER ];
   QString account  = details[ CALL_ACCOUNTID   ];
   QString peerName = details[ CALL_PEER_NAME   ];
   
   Call* call = new Call(CALL_STATE_RINGING, callId, peerName, from, account);
   call->m_pHistoryState = OUTGOING;
   return call;
}

/*****************************************************************************
 *                                                                           *
 *                                  History                                  *
 *                                                                           *
 ****************************************************************************/

///Build a call that is already over
Call* Call::buildHistoryCall(const QString & callId, uint startTimeStamp, uint stopTimeStamp, QString account, QString name, QString number, QString type)
{
   if(name == "empty") name = "";
   Call* call            = new Call(CALL_STATE_OVER, callId, name, number, account );
   call->m_pStartTime    = new QDateTime(QDateTime::fromTime_t(startTimeStamp)     );
   call->m_pStopTime     = new QDateTime(QDateTime::fromTime_t(stopTimeStamp)      );
   call->m_pHistoryState = getHistoryStateFromType(type                            );
   return call;
}

///Get the history state from the type (see Call.cpp header)
history_state Call::getHistoryStateFromType(QString type)
{
   if(type == DAEMON_HISTORY_TYPE_MISSED        )
      return MISSED   ;
   else if(type == DAEMON_HISTORY_TYPE_OUTGOING )
      return OUTGOING ;
   else if(type == DAEMON_HISTORY_TYPE_INCOMING )
      return INCOMING ;
   return NONE        ;
}

///Get the type from an history state (see Call.cpp header)
QString Call::getTypeFromHistoryState(history_state historyState)
{
   if(historyState == MISSED        )
      return DAEMON_HISTORY_TYPE_MISSED   ;
   else if(historyState == OUTGOING )
      return DAEMON_HISTORY_TYPE_OUTGOING ;
   else if(historyState == INCOMING )
      return DAEMON_HISTORY_TYPE_INCOMING ;
   return QString()                       ;
}

///Get history state from daemon
history_state Call::getHistoryStateFromDaemonCallState(QString daemonCallState, QString daemonCallType)
{
   if((daemonCallState      == DAEMON_CALL_STATE_INIT_CURRENT  || daemonCallState == DAEMON_CALL_STATE_INIT_HOLD) && daemonCallType == DAEMON_CALL_TYPE_INCOMING )
      return INCOMING ;
   else if((daemonCallState == DAEMON_CALL_STATE_INIT_CURRENT  || daemonCallState == DAEMON_CALL_STATE_INIT_HOLD) && daemonCallType == DAEMON_CALL_TYPE_OUTGOING )
      return OUTGOING ;
   else if(daemonCallState  == DAEMON_CALL_STATE_INIT_BUSY                                                                                                       )
      return OUTGOING ;
   else if(daemonCallState  == DAEMON_CALL_STATE_INIT_INACTIVE && daemonCallType == DAEMON_CALL_TYPE_INCOMING                                                    )
      return INCOMING ;
   else if(daemonCallState  == DAEMON_CALL_STATE_INIT_INACTIVE && daemonCallType == DAEMON_CALL_TYPE_OUTGOING                                                    )
      return MISSED   ;
   else
      return NONE     ;
}

///Get the start sate from the daemon state
call_state Call::getStartStateFromDaemonCallState(QString daemonCallState, QString daemonCallType)
{
   if(daemonCallState      == DAEMON_CALL_STATE_INIT_CURRENT  )
      return CALL_STATE_CURRENT  ;
   else if(daemonCallState == DAEMON_CALL_STATE_INIT_HOLD     )
      return CALL_STATE_HOLD     ;
   else if(daemonCallState == DAEMON_CALL_STATE_INIT_BUSY     )
      return CALL_STATE_BUSY     ;
   else if(daemonCallState == DAEMON_CALL_STATE_INIT_INACTIVE && daemonCallType == DAEMON_CALL_TYPE_INCOMING )
      return CALL_STATE_INCOMING ;
   else if(daemonCallState == DAEMON_CALL_STATE_INIT_INACTIVE && daemonCallType == DAEMON_CALL_TYPE_OUTGOING )
      return CALL_STATE_RINGING  ;
   else if(daemonCallState == DAEMON_CALL_STATE_INIT_INCOMING )
      return CALL_STATE_INCOMING ;
   else if(daemonCallState == DAEMON_CALL_STATE_INIT_RINGING  )
      return CALL_STATE_RINGING  ;
   else
      return CALL_STATE_FAILURE  ;
}

/*****************************************************************************
 *                                                                           *
 *                                  Getters                                  *
 *                                                                           *
 ****************************************************************************/

///Transfer state from internal to daemon internal syntaz
daemon_call_state Call::toDaemonCallState(const QString & stateName)
{
   if(stateName == QString(CALL_STATE_CHANGE_HUNG_UP)        )
      return DAEMON_CALL_STATE_HUNG_UP ;
   if(stateName == QString(CALL_STATE_CHANGE_RINGING)        )
      return DAEMON_CALL_STATE_RINGING ;
   if(stateName == QString(CALL_STATE_CHANGE_CURRENT)        )
      return DAEMON_CALL_STATE_CURRENT ;
   if(stateName == QString(CALL_STATE_CHANGE_UNHOLD_CURRENT) )
      return DAEMON_CALL_STATE_CURRENT ;
   if(stateName == QString(CALL_STATE_CHANGE_UNHOLD_RECORD)  )
      return DAEMON_CALL_STATE_CURRENT ;
   if(stateName == QString(CALL_STATE_CHANGE_HOLD)           )
      return DAEMON_CALL_STATE_HOLD    ;
   if(stateName == QString(CALL_STATE_CHANGE_BUSY)           )
      return DAEMON_CALL_STATE_BUSY    ;
   if(stateName == QString(CALL_STATE_CHANGE_FAILURE)        )
      return DAEMON_CALL_STATE_FAILURE ;
   
   qDebug() << "stateChanged signal received with unknown state.";
   return DAEMON_CALL_STATE_FAILURE    ;
}

///Get the time (second from 1 jan 1970) when the call ended
QString Call::getStopTimeStamp()    const
{
   if (m_pStopTime == NULL)
      return QString();
   return QString::number(m_pStopTime->toTime_t());
}

///Get the time (second from 1 jan 1970) when the call started
QString Call::getStartTimeStamp()   const
{
   if (m_pStartTime == NULL)
      return QString();
   return QString::number(m_pStartTime->toTime_t());
}

///Get the number where the call have been transferred
QString Call::getTransferNumber()    const
{
   return m_pTransferNumber;
}

///Get the call / peer number
QString Call::getCallNumber()        const
{
   return m_pCallNumber;
}

///Return the call id
QString Call::getCallId()            const
{
   return m_pCallId;
}

///Return the peer phone number
QString Call::getPeerPhoneNumber()   const
{
   return m_pPeerPhoneNumber;
}

///Get the peer name
QString Call::getPeerName()          const
{
   return m_pPeerName;
}

///Get the current state
call_state Call::getCurrentState()   const
{
   return currentState;
}

///Get the call recording
bool Call::getRecording()            const
{
   return recording;
}

///Get the call account id
QString Call::getAccountId()         const
{
   return m_pAccount;
}

///Is this call a conference
bool Call::isConference()            const
{
   return conference;
}

///Get the conference ID
QString Call::getConfId()            const
{
   return m_pConfId;
}

///Get the current codec
QString Call::getCurrentCodecName()  const
{
   CallManagerInterface & callManager = CallManagerInterfaceSingleton::getInstance();
   return callManager.getCurrentAudioCodecName(m_pCallId);
}

///Get the state
call_state Call::getState()          const
{
   return currentState;
}

///Get the history state
history_state Call::getHistoryState() const
{
   return m_pHistoryState;
}

///Is this call over?
bool Call::isHistory()               const
{
   return (getState() == CALL_STATE_OVER);
}

///This function could also be called mayBeSecure or haveChancesToBeEncryptedButWeCantTell.
bool Call::isSecure() const {

   if (m_pAccount.isEmpty()) {
      qDebug() << "Account not set, can't check security";
      return false;
   }

   AccountList accountList(true);
   Account* currentAccount = accountList.getAccountById(m_pAccount);

   if ((currentAccount->getAccountDetail(TLS_ENABLE ) == "true") || (currentAccount->getAccountDetail(TLS_METHOD).toInt())) {
      return true;
   }
   return false;
}


/*****************************************************************************
 *                                                                           *
 *                                  Setters                                  *
 *                                                                           *
 ****************************************************************************/

///Set the transfer number
void Call::setTransferNumber(QString number)
{
   m_pTransferNumber = number;
}

///This call is a conference
void Call::setConference(bool value)
{
   conference = value;
}

///Set the call number
void Call::setCallNumber(QString number)
{
   m_pCallNumber = number;
   emit changed();
}

///Set the conference ID
void Call::setConfId(QString value)
{
   m_pConfId = value;
}

/*****************************************************************************
 *                                                                           *
 *                                  Mutator                                  *
 *                                                                           *
 ****************************************************************************/

///The call state just changed
call_state Call::stateChanged(const QString& newStateName)
{
   if (!conference) {
      call_state previousState = currentState;
      daemon_call_state dcs = toDaemonCallState(newStateName);
      changeCurrentState(stateChangedStateMap[currentState][dcs]);
      (this->*(stateChangedFunctionMap[previousState][dcs]))();
      qDebug() << "Calling stateChanged " << newStateName << " -> " << toDaemonCallState(newStateName) << " on call with state " << previousState << ". Become " << currentState;
      return currentState;
   }
   else {
      qDebug() << "A conference have no call state";
      return CALL_STATE_ERROR;
   }
}

///An acount have been performed
call_state Call::actionPerformed(call_action action)
{
   call_state previousState = currentState;
   Q_ASSERT_X((previousState>10) || (previousState<0),"perform action","Invalid previous state ("+QString::number(previousState)+")");
   Q_ASSERT_X((state>4) || (state < 0),"perform action","Invalid action ("+QString::number(action)+")");
   Q_ASSERT_X((action>5) || (action < 0),"perform action","Invalid action ("+QString::number(action)+")");
   //update the state
   changeCurrentState(actionPerformedStateMap[previousState][action]);
   //execute the action associated with this transition
   (this->*(actionPerformedFunctionMap[previousState][action]))(); //WARNING BUG //WARNING SEGFAULT //TODO remove this node, it was not a good idea, it is not stable
   qDebug() << "Calling action " << action << " on call with state " << previousState << ". Become " << currentState;
   //return the new state
   return currentState;
}

/*
void Call::putRecording()
{
   CallManagerInterface & callManager = CallManagerInterfaceSingleton::getInstance();
   bool daemonRecording = callManager.getIsRecording(this -> m_pCallId);
   if(daemonRecording != recording)
   {
      callManager.setRecording(this->m_pCallId);
   }
}
*/
///Change the state
void Call::changeCurrentState(call_state newState)
{
   //qDebug() << "Call state changed to: " << newState;
   currentState = newState;

   emit changed();

   if (currentState == CALL_STATE_OVER)
      emit isOver(this);
}


/*****************************************************************************
 *                                                                           *
 *                              Automate function                            *
 *                                                                           *
 ****************************************************************************/
///@warning DO NOT TOUCH THAT, THEY ARE CALLED FROM AN ARRAY, HIGH FRAGILITY

///Do nothing (literally)
void Call::nothing()
{
}

///Accept the call
void Call::accept()
{
   CallManagerInterface & callManager = CallManagerInterfaceSingleton::getInstance();
   qDebug() << "Accepting call. callId : " << m_pCallId;
   callManager.accept(m_pCallId);
   this->m_pStartTime = new QDateTime(QDateTime::currentDateTime());
   this->m_pHistoryState = INCOMING;
}

///Refuse the call
void Call::refuse()
{
   CallManagerInterface & callManager = CallManagerInterfaceSingleton::getInstance();
   qDebug() << "Refusing call. callId : " << m_pCallId;
   callManager.refuse(m_pCallId);
   this->m_pStartTime = new QDateTime(QDateTime::currentDateTime());
   this->m_pHistoryState = MISSED;
}

///Accept the transfer
void Call::acceptTransf()
{
   CallManagerInterface & callManager = CallManagerInterfaceSingleton::getInstance();
   qDebug() << "Accepting call and transfering it to number : " << m_pTransferNumber << ". callId : " << m_pCallId;
   callManager.accept(m_pCallId);
   callManager.transfer(m_pCallId, m_pTransferNumber);
//   m_pHistoryState = TRANSFERED;
}

///Put the call on hold
void Call::acceptHold()
{
   CallManagerInterface & callManager = CallManagerInterfaceSingleton::getInstance();
   qDebug() << "Accepting call and holding it. callId : " << m_pCallId;
   callManager.accept(m_pCallId);
   callManager.hold(m_pCallId);
   this->m_pHistoryState = INCOMING;
}

///Hang up
void Call::hangUp()
{
   CallManagerInterface & callManager = CallManagerInterfaceSingleton::getInstance();
   this->m_pStopTime = new QDateTime(QDateTime::currentDateTime());
   qDebug() << "Hanging up call. callId : " << m_pCallId;
   callManager.hangUp(m_pCallId);
}

///Cancel this call
void Call::cancel()
{
   CallManagerInterface & callManager = CallManagerInterfaceSingleton::getInstance();
   qDebug() << "Canceling call. callId : " << m_pCallId;
   callManager.hangUp(m_pCallId);
}

///Put on hold
void Call::hold()
{
   CallManagerInterface & callManager = CallManagerInterfaceSingleton::getInstance();
   qDebug() << "Holding call. callId : " << m_pCallId;
   callManager.hold(m_pCallId);
}

///Start the call
void Call::call()
{
   CallManagerInterface & callManager = CallManagerInterfaceSingleton::getInstance();
   qDebug() << "account = " << m_pAccount;
   if(m_pAccount.isEmpty()) {
      qDebug() << "account is not set, taking the first registered.";
      this->m_pAccount = CallModelConvenience::getCurrentAccountId();
   }
   if(!m_pAccount.isEmpty()) {
      qDebug() << "Calling " << m_pCallNumber << " with account " << m_pAccount << ". callId : " << m_pCallId;
      callManager.placeCall(m_pAccount, m_pCallId, m_pCallNumber);
      this->m_pAccount = m_pAccount;
      this->m_pPeerPhoneNumber = m_pCallNumber;
      if (m_pContactBackend) {
         Contact* contact = m_pContactBackend->getContactByPhone(m_pPeerPhoneNumber);
         if (contact)
            m_pPeerName = contact->getFormattedName();
      }
      this->m_pStartTime = new QDateTime(QDateTime::currentDateTime());
      this->m_pHistoryState = OUTGOING;
   }
   else {
      qDebug() << "Trying to call " << m_pTransferNumber << " with no account registered . callId : " << m_pCallId;
      this->m_pHistoryState = NONE;
      throw "No account registered!";
   }
}

///Trnasfer the call
void Call::transfer()
{
   CallManagerInterface & callManager = CallManagerInterfaceSingleton::getInstance();
   qDebug() << "Transfering call to number : " << m_pTransferNumber << ". callId : " << m_pCallId;
   callManager.transfer(m_pCallId, m_pTransferNumber);
   this->m_pStopTime = new QDateTime(QDateTime::currentDateTime());
}

void Call::unhold()
{
   CallManagerInterface & callManager = CallManagerInterfaceSingleton::getInstance();
   qDebug() << "Unholding call. callId : " << m_pCallId;
   callManager.unhold(m_pCallId);
}

/*
void Call::switchRecord()
{
   qDebug() << "Switching record state for call automate. callId : " << callId;
   recording = !recording;
}
*/

///Record the call
void Call::setRecord()
{
   CallManagerInterface & callManager = CallManagerInterfaceSingleton::getInstance();
   qDebug() << "Setting record " << !recording << " for call. callId : " << m_pCallId;
   callManager.setRecording(m_pCallId);
   recording = !recording;
}

///Start the timer
void Call::start()
{
   qDebug() << "Starting call. callId : " << m_pCallId;
   this->m_pStartTime = new QDateTime(QDateTime::currentDateTime());
}

///Toggle the timer
void Call::startStop()
{
   qDebug() << "Starting and stoping call. callId : " << m_pCallId;
   this->m_pStartTime = new QDateTime(QDateTime::currentDateTime());
   this->m_pStopTime = new QDateTime(QDateTime::currentDateTime());
}

///Stop the timer
void Call::stop()
{
   qDebug() << "Stoping call. callId : " << m_pCallId;
   this->m_pStopTime = new QDateTime(QDateTime::currentDateTime());
}

///Handle error instead of crashing
void Call::startWeird()
{
   qDebug() << "Starting call. callId : " << m_pCallId;
   this->m_pStartTime = new QDateTime(QDateTime::currentDateTime());
   qDebug() << "Warning : call " << m_pCallId << " had an unexpected transition of state at its start.";
}

///Print a warning
void Call::warning()
{
   qDebug() << "Warning : call " << m_pCallId << " had an unexpected transition of state.";
}

/*****************************************************************************
 *                                                                           *
 *                             Keyboard handling                             *
 *                                                                           *
 ****************************************************************************/

///Input text on the call item
void Call::appendText(QString str)
{
   QString * editNumber;
   
   switch (currentState) {
   case CALL_STATE_TRANSFER    :
   case CALL_STATE_TRANSF_HOLD :
      editNumber = &m_pTransferNumber;
      break;
   case CALL_STATE_DIALING     :
      editNumber = &m_pCallNumber;
      break;
   default                     :
      qDebug() << "Backspace on call not editable. Doing nothing.";
      return;
   }

   editNumber->append(str);

   emit changed();
}

///Remove the last character
void Call::backspaceItemText()
{
   QString * editNumber;

   switch (currentState) {
      case CALL_STATE_TRANSFER        :
      case CALL_STATE_TRANSF_HOLD     :
         editNumber = &m_pTransferNumber;
         break;
      case CALL_STATE_DIALING         :
         editNumber = &m_pCallNumber;
         break;
      default                         :
         qDebug() << "Backspace on call not editable. Doing nothing.";
         return;
   }
   QString text = *editNumber;
   int textSize = text.size();
   if(textSize > 0) {
      *editNumber = text.remove(textSize-1, 1);

      emit changed();
   }
   else {
      changeCurrentState(CALL_STATE_OVER);
   }
}
