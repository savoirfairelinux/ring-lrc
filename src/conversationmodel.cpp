/****************************************************************************
 *   Copyright (C) 2017 Savoir-faire Linux                                  *
 *   Author : Nicolas Jäger <nicolas.jager@savoirfairelinux.com>            *
 *   Author : Sébastien Blin <sebastien.blin@savoirfairelinux.com>          *
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
#include "api/conversationmodel.h"

// std
#include <regex>
#include <algorithm>

// LRC
#include "api/contactmodel.h"
#include "api/newcallmodel.h"
#include "api/newaccountmodel.h"
#include "api/account.h"
#include "callbackshandler.h"
#include "database.h"

#include "availableaccountmodel.h"
#include "namedirectory.h"
#include "phonedirectorymodel.h"
#include "contactmethod.h"

// Dbus
#include "dbus/configurationmanager.h"

namespace lrc
{

using namespace api;

class ConversationModelPimpl : public QObject
{
    Q_OBJECT
public:
    ConversationModelPimpl(const ConversationModel& linked,
                           Database& db,
                           const CallbacksHandler& callbacksHandler);

    ~ConversationModelPimpl();

    /**
     * return a conversation index from conversations or -1 if no index is found.
     * @param uid of the contact to search.
     * @return an int.
     */
    int indexOf(const std::string& uid) const;
    /**
     * Initialize conversations_ and filteredConversations_
     */
    void initConversations();
    /**
     * Sort conversation by last action
     */
    void sortConversations();
    void search();

    const ConversationModel& linked;
    Database& db;
    const CallbacksHandler& callbacksHandler;

    ConversationModel::ConversationQueue conversations;
    mutable ConversationModel::ConversationQueue filteredConversations;
    std::string filter;
    contact::Type typeFilter;

public Q_SLOTS:
    void slotContactsChanged();
    void slotMessageAdded(int uid, const std::string& accountId, const message::Info& msg);
    void slotRegisteredNameFound(const Account* account, NameDirectory::LookupStatus status,
                             const QString& address, const QString& name);

};

ConversationModel::ConversationModel(const account::Info& owner, Database& db, const CallbacksHandler& callbacksHandler)
: QObject()
, pimpl_(std::make_unique<ConversationModelPimpl>(*this, db, callbacksHandler))
, owner(owner)
{

}

ConversationModel::~ConversationModel()
{

}

const ConversationModel::ConversationQueue&
ConversationModel::getFilteredConversations() const
{
    return pimpl_->filteredConversations;
    // TODO
}

conversation::Info
ConversationModel::getConversation(const unsigned int row) const
{
    if (row >= pimpl_->filteredConversations.size())
        return conversation::Info();
    return pimpl_->filteredConversations.at(row);
}

void
ConversationModel::addConversation(const std::string& uid) const
{
    auto conversationIdx = pimpl_->indexOf(uid);

    if (conversationIdx == -1)
        return;

    auto& conversation = pimpl_->conversations.at(conversationIdx);

    if (conversation.participants.empty())
        return;

    auto contactUri = conversation.participants.front();
    auto contact = owner.contactModel->getContact(contactUri);

    // Send contact request if non used
    auto isNotUsed = contact.type == contact::Type::TEMPORARY
    || contact.type == contact::Type::PENDING;
    if (isNotUsed) {
        if (contactUri.length() == 0) {
            contactUri = owner.contactModel->getContact(contactUri).uri;
        }
        owner.contactModel->addContact(contactUri);
    }

}

void
ConversationModel::selectConversation(const std::string& uid)
{
    // Get conversation
    auto conversationIdx = pimpl_->indexOf(uid);

    if (conversationIdx == -1)
        return;

    auto& conversation = pimpl_->conversations.at(conversationIdx);
    auto participants = conversation.participants;

    // Check if conversation has a valid contact.
    if (participants.empty())
        return;

    emit showChatView(conversation);
    /* TODO
    if (conversation.call.status == call::Status::INVALID) {
        emit showChatView(conversation);
        return;
    }
    switch (conversation.call.status) {
    case call::Status::INCOMING_RINGING:
    case call::Status::OUTGOING_RINGING:
    case call::Status::CONNECTING:
    case call::Status::SEARCHING:
            // We are currently in a call
            emit showIncomingCallView(conversation);
            break;
        case call::Status::IN_PROGRESS:
            // We are currently receiving a call
            emit showCallView(conversation);
            break;
        default:
            // We are not in a call, show the chatview
            emit showChatView(conversation);
    }*/
}

void
ConversationModel::removeConversation(const std::string& uid)
{
    // Get conversation
    auto i = std::find_if(pimpl_->conversations.begin(), pimpl_->conversations.end(),
    [uid](const conversation::Info& conversation) {
      return conversation.uid == uid;
    });

    if (i == pimpl_->conversations.end())
        return;

    auto conversation = *i;

    if (conversation.participants.empty())
        return;

    // Remove contact from daemon
    auto contact = conversation.participants.front();
    owner.contactModel->removeContact(contact);
}

void
ConversationModel::placeCall(const std::string& uid) const
{
}

void
ConversationModel::sendMessage(const std::string& uid, const std::string& body) const
{
    auto conversationIdx = pimpl_->indexOf(uid);

    if (conversationIdx == -1)
        return;

    auto& conversation = pimpl_->conversations.at(conversationIdx);

    if (conversation.participants.empty())
        return;

    auto account = AvailableAccountModel::instance().currentDefaultAccount(); // TODO replace by linked account

    auto contact = owner.contactModel->getContact(conversation.participants.front());
    auto isNotUsed = contact.type == contact::Type::TEMPORARY
    || contact.type == contact::Type::PENDING;


    // Send contact request if non used
    if (isNotUsed)
        addConversation(contact.uri);

    // Send message to contact.
    QMap<QString, QString> payloads;
    payloads["text/plain"] = body.c_str();

    // TODO change this for group messages
    auto id = ConfigurationManager::instance().sendTextMessage(account->id(),
    contact.uri.c_str(), payloads);

    message::Info msg;
    msg.contact = contact.uri.c_str();
    msg.body = body;
    msg.timestamp = std::time(nullptr);
    msg.type = message::Type::TEXT;
    msg.status = message::Status::SENDING;

    // TODO Add to database
}

void
ConversationModel::setFilter(const std::string& filter)
{
    //~ owner.contactModel->temporaryContact;
    //~ return;
    auto accountInfo = AvailableAccountModel::instance().currentDefaultAccount();
    if (!accountInfo) return;
    pimpl_->filter = filter;
    // We don't create newConversationItem if we already filter on pending
    conversation::Info newConversationItem;
    if (!pimpl_->filter.empty()) {
        // add the first item, wich is the NewConversationItem
        conversation::Info conversationInfo;
        contact::Info participant;
        participant.alias = "Searching...";
        auto returnFromDb = pimpl_->db.select("photo", "profiles", "uri=:uri", {{":uri", ""}});
        if (returnFromDb.nbrOfCols == 1 and returnFromDb.payloads.size() == 1)
            participant.avatar = returnFromDb.payloads[0];
        else
            participant.avatar = "";
        conversationInfo.uid = participant.uri;
        participant.alias += filter;
        //~ owner.contactModel->temporaryContact = std::unique_ptr<contact::Info>(&participant);
        //~ owner.contactModel->temporaryContact = std::move(participant);
        //~ owner.contactModel->temporaryContact.alias = "toto"; //participant.alias;
        conversationInfo.participants.emplace_back("");
        conversationInfo.accountId = accountInfo->id().toStdString();
        if (!pimpl_->conversations.empty()) {
            auto firstContactUri = pimpl_->conversations.front().participants.front();
            auto firstContact = owner.contactModel->getContact(firstContactUri);
            auto isUsed = pimpl_->conversations.front().isUsed ;
            if (isUsed || firstContact.type == contact::Type::PENDING) {
                // No newConversationItem, create one
                newConversationItem = conversationInfo;
                pimpl_->conversations.emplace_front(newConversationItem);
            } else {
                // The item already exists
                pimpl_->conversations.pop_front();
                newConversationItem = conversationInfo;
                pimpl_->conversations.emplace_front(newConversationItem);
            }
        } else {
            newConversationItem = conversationInfo;
            pimpl_->conversations.emplace_front(newConversationItem);
        }
        pimpl_->search();
    } else {
        // No filter, so we can remove the newConversationItem
        if (!pimpl_->conversations.empty()) {
            auto firstContactUri = pimpl_->conversations.front().participants.front();
            auto firstContact = owner.contactModel->getContact(firstContactUri);
            auto temporaryContactUri = "owner.contactModel->temporaryContact.uri";
            if (firstContact.uri == temporaryContactUri) {
                pimpl_->conversations.pop_front();
            }
        }
    }
    emit modelUpdated();
}

void
ConversationModel::addParticipant(const std::string& uid, const::std::string& uri)
{

}

void
ConversationModel::clearHistory(const std::string& uid)
{

}

ConversationModelPimpl::ConversationModelPimpl(const ConversationModel& linked,
                                               Database& db,
                                               const CallbacksHandler& callbacksHandler)
: linked(linked)
, db(db)
, callbacksHandler(callbacksHandler)
{
    initConversations();

    // [jn] those signals don't make sense anymore. We may have to recycle them, or just to delete them.
    // since adding a message from Conversation use insertInto wich return something, I guess we can deal with
    // messageAdded without signal.
    //~ connect(&db, &Database::messageAdded, this, &ConversationModelPimpl::slotMessageAdded);
    //~ connect(&*linked.owner.contactModel, &ContactModel::modelUpdated,
            //~ this, &ConversationModelPimpl::slotContactModelUpdated);
}


ConversationModelPimpl::~ConversationModelPimpl()
{

}

void
ConversationModelPimpl::search()
{
    if (linked.owner.profile.type == contact::Type::SIP) {
        if (!conversations.empty()) {
            auto firstConversation = conversations.front();
            auto contact = linked.owner.contactModel->getContact(firstConversation.participants[0]);
            auto isNotUsed = contact.type == contact::Type::TEMPORARY;
            if (isNotUsed) {
                auto uid = filter;
                conversations.pop_front();
                auto conversationIdx = indexOf(uid);
                if (conversationIdx == -1) {
                    // create the new conversation
                    //TODO
                }
                emit linked.modelUpdated();
            }
        }
        return;
    }
    // Update alias
    auto uri = URI(QString(filter.c_str()));
    // Query NS
    Account* account = nullptr;
    if (uri.schemeType() != URI::SchemeType::NONE) {
        account = AvailableAccountModel::instance().currentDefaultAccount(uri.schemeType());
    } else {
        account = AvailableAccountModel::instance().currentDefaultAccount();
    }
    if (!account) return;
    connect(&NameDirectory::instance(), &NameDirectory::registeredNameFound,
    this, &ConversationModelPimpl::slotRegisteredNameFound);

    if (account->protocol() == Account::Protocol::RING &&
        uri.protocolHint() != URI::ProtocolHint::RING)
    {
        account->lookupName(QString(filter.c_str()));
    } else {
        /* no lookup, simply use the URI as is */
        auto cm = PhoneDirectoryModel::instance().getNumber(uri, account);
        if (!conversations.empty()) {
            auto firstConversation = conversations.front();
            auto contact = linked.owner.contactModel->getContact(firstConversation.participants[0]);
            auto isNotUsed = contact.type == contact::Type::TEMPORARY;
            if (isNotUsed) {
                auto account = AvailableAccountModel::instance().currentDefaultAccount();
                if (!account) return;
                auto uid = cm->uri().toStdString();
                conversations.pop_front();
                auto conversationIdx = indexOf(uid);
                if (conversationIdx == -1) {
                    // create the new conversation
                    // TODO
                }
                emit linked.modelUpdated();
            }
        }
    }
}

void
ConversationModelPimpl::initConversations()
{
    auto account = AvailableAccountModel::instance().currentDefaultAccount();

    if (!account)
        return;

    conversations.clear();
    // Fill conversations
    auto accountProfileId = db.select("id",
                              "profiles",
                              "uri=:uri",
                              {{":uri", linked.owner.profile.uri}}).payloads;
    if (accountProfileId.empty()) {
        qDebug() << "ConversationModelPimpl::initConversations(), account not in bdd... abort";
        return;
    }
    auto conversationsForAccount = db.select("id",
                                  "conversations",
                                  "participant_id=:participant_id",
                                  {{":participant_id", accountProfileId[0]}}).payloads;
    std::sort(conversationsForAccount.begin(), conversationsForAccount.end());
    for (auto const& contact : linked.owner.contactModel->getAllContacts())
    {
        // for each contact
        // TODO: split this
        auto contactinfo = contact;
        auto contactProfileId = db.select("id",
                                      "profiles",
                                      "uri=:uri",
                                      {{":uri", contactinfo.second.uri}}).payloads;
        if (contactProfileId.empty()) {
          qDebug() << "ConversationModelPimpl::initConversations(), contact not in bdd... abort";
          return;
        }
        // Get linked conversation with they
        auto conversationsForContact = db.select("id",
                                      "conversations",
                                      "participant_id=:participant_id",
                                      {{":participant_id", contactProfileId[0]}}).payloads;

        std::sort(conversationsForContact.begin(), conversationsForContact.end());
        std::vector<std::string> common;

        std::set_intersection(conversationsForAccount.begin(), conversationsForAccount.end(),
                              conversationsForContact.begin(), conversationsForContact.end(),
                              std::back_inserter(common));
        // Can't find a conversation with this contact. It's anormal, add one in the db
        if (common.empty()) {
            auto newConversationsId = db.select("IFNULL(MAX(id), 0) + 1",
                                          "conversations",
                                          "1=1",
                                          {}).payloads[0];
            db.insertInto("conversations",
                          {{":id", "id"}, {":participant_id", "participant_id"}},
                          {{":id", newConversationsId}, {":participant_id", accountProfileId[0]}});
            db.insertInto("conversations",
                          {{":id", "id"}, {":participant_id", "participant_id"}},
                          {{":id", newConversationsId}, {":participant_id", contactProfileId[0]}});
            // Add "Conversation started" message
            db.insertInto("interactions",
                          {{":account_id", "account_id"}, {":author_id", "author_id"},
                          {":conversation_id", "conversation_id"}, {":device_id", "device_id"},
                          {":group_id", "group_id"}, {":timestamp", "timestamp"},
                          {":body", "body"}, {":type", "type"},
                          {":status", "status"}},
                          {{":account_id", accountProfileId[0]}, {":author_id", accountProfileId[0]},
                          {":conversation_id", newConversationsId}, {":device_id", "0"},
                          {":group_id", "0"}, {":timestamp", "0"},
                          {":body", "Conversation started"}, {":type", "CONTACT"},
                          {":status", "SUCCEED"}});
            common.emplace_back(newConversationsId);
        }

        // Add the conversation
        conversation::Info conversation;
        conversation.uid = common[0];
        conversation.accountId = linked.owner.id;
        conversation.participants = {contactinfo.second.uri};
        conversation.callId = ""; // TODO update if current call.
        // Get messages
        auto messagesResult = db.select("id, body, timestamp, type, status",
                                        "interactions",
                                        "conversation_id=:conversation_id",
                                        {{":conversation_id", conversation.uid}});
        if (messagesResult.nbrOfCols == 5) {
            auto payloads = messagesResult.payloads;
            for (auto i = 0; i < payloads.size(); i += 5) {
                message::Info msg;
                msg.contact = contactinfo.second.uri;
                msg.body = payloads[i + 1];
                msg.timestamp = std::stoi(payloads[i + 2]);
                msg.type = message::StringToType(payloads[i + 3]);
                msg.status = message::StringToStatus(payloads[i + 4]);
                conversation.messages.emplace(std::make_pair<int, message::Info>(std::stoi(payloads[i]), std::move(msg)));
                conversation.lastMessageUid = std::stoi(payloads[i]);
            }
        }
        conversations.emplace_front(conversation);
    }

    sortConversations();
    filteredConversations = conversations;
}

void
ConversationModelPimpl::sortConversations()
{

}

void
ConversationModelPimpl::slotContactsChanged()
{

}

void
ConversationModelPimpl::slotMessageAdded(int uid, const std::string& account, const message::Info& msg)
{
    auto conversationIdx = indexOf(msg.contact);

    if (conversationIdx == -1)
        return;

    auto conversation = conversations.at(conversationIdx);

    if (conversation.participants.empty())
        return;

    if (!conversation.isUsed)
        conversation.isUsed = true;

    // Add message to conversation
    conversation.messages.insert(std::pair<int, message::Info>(uid, msg));
    conversation.lastMessageUid = uid;

    emit linked.newMessageAdded(msg.contact, msg);

    sortConversations();
    emit linked.modelUpdated();
}

void
ConversationModelPimpl::slotRegisteredNameFound(const Account* account, NameDirectory::LookupStatus status,
                                           const QString& address, const QString& name)
{
    std::sort(conversations.begin(), conversations.end(),
    [](const conversation::Info& conversationA, const conversation::Info& conversationB)
    {
        auto historyA = conversationA.messages;
        auto historyB = conversationB.messages;
        // A or B is a new conversation (without INVITE message)
        if (historyA.empty()) return true;
        if (historyB.empty()) return false;
        // Sort by last Interaction
        try
        {
            auto lastMessageA = historyA.at(conversationA.lastMessageUid);
            auto lastMessageB = historyB.at(conversationB.lastMessageUid);
            return lastMessageA.timestamp > lastMessageB.timestamp;
        }
        catch (const std::exception& e)
        {
            qDebug() << "ConversationModel::sortConversations(), can't get lastMessage";
            return true;
        }
    });
}

int
ConversationModelPimpl::indexOf(const std::string& uid) const
{
    for (unsigned int i = 0; i < conversations.size() ; ++i) {
        if(conversations.at(i).uid == uid) return i;
    }
    return -1;
}

} // namespace lrc

#include "api/moc_conversationmodel.cpp"
#include "conversationmodel.moc"
