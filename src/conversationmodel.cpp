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
#include <stdexcept>
#include <iostream>

// Models and database
#include "database.h"
#include "api/contactmodel.h"
#include "api/newcallmodel.h"
#include "api/newaccountmodel.h"

// Dbus
#include "dbus/configurationmanager.h"

// Lrc
#include "availableaccountmodel.h"

namespace lrc
{

namespace api
{

class ConversationModelPimpl : public QObject
{
public:
    ConversationModelPimpl(ConversationModel& linked,
                           const NewAccountModel& p,
                           const Database& d,
                           const account::Info& o);

    ~ConversationModelPimpl();

    // shortcuts in owner
    NewCallModel& callModel; // [jn] : is it really useful ?

    /**
     * return a conversation index from conversations or -1 if no index is found.
     * @param uid of the contact to search.
     * @return an int.
     */
    int indexOf(const std::string& uid) const;
    /**
     * Initialize pimpl_->conversations and pimpl_->filteredConversations
     */
    void initConversations();
    /**
     * Sort conversation by last action
     */
    void sortConversations();
    void search();

    ConversationModel& linked;
    const NewAccountModel& parent;
    const Database& database;

    ConversationQueue conversations;
    mutable ConversationQueue filteredConversations;
    std::string filter;
    contact::Type typeFilter;

public Q_SLOTS:
    void slotContactModelUpdated();
    void slotMessageAdded(int uid, const std::string& accountId, const message::Info& msg);
    void registeredNameFound(const Account* account, NameDirectory::LookupStatus status,
                             const QString& address, const QString& name);
    void slotIncomingCall(const std::string& callId, const std::string& fromId);
    void slotCallStatusChanged(const std::string& callId);

};

ConversationModel::ConversationModel(const NewAccountModel& parent,
                                     const Database& database,
                                     const account::Info& info)
: pimpl_(std::make_unique<ConversationModelPimpl>(*this, parent, database, owner))
, owner(info)
{

}

ConversationModel::~ConversationModel()
{

}

const ConversationQueue&
ConversationModel::getFilteredConversations() const
{
    pimpl_->filteredConversations = pimpl_->conversations;

    auto filter = pimpl_->filter;
    auto typeFilter = pimpl_->typeFilter;
    auto it = std::copy_if(pimpl_->conversations.begin(), pimpl_->conversations.end(), pimpl_->filteredConversations.begin(),
    [&filter, &typeFilter, this] (const conversation::Info& entry) {
        auto contact = owner.contactModel->getContact(entry.participants.front());
        // Check type
        if (typeFilter != contact::Type::PENDING) {
            if (contact.type == contact::Type::PENDING) return false;
            if (contact.type == contact::Type::TEMPORARY) return true;
        } else {
            if (contact.type != contact::Type::PENDING) return false;
        }

        // Check contact
        try {
            auto regexFilter = std::regex(filter, std::regex_constants::icase);
            bool result = std::regex_search(contact.uri, regexFilter)
            | std::regex_search(contact.alias, regexFilter);
            return result;
        } catch(std::regex_error&) {
            // If the regex is incorrect, just test if filter is a substring of the title or the alias.
            return contact.alias.find(filter) != std::string::npos
            && contact.uri.find(filter) != std::string::npos;
        }
    });
    pimpl_->filteredConversations.resize(std::distance(pimpl_->filteredConversations.begin(), it));

    return pimpl_->filteredConversations;
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

    try  {
        auto call = owner.callModel->getCall(conversation.callId);
        switch (call.status) {
            case call::Status::INCOMING_RINGING:
            case call::Status::OUTGOING_RINGING:
            case call::Status::CONNECTING:
            case call::Status::SEARCHING:
                // We are currently in a call
                emit showIncomingCallView(conversation);
                break;
            case call::Status::PAUSED:
            case call::Status::PEER_PAUSED:
            case call::Status::CONNECTED:
            case call::Status::IN_PROGRESS:
                // We are currently receiving a call
                emit showCallView(conversation);
                break;
            case call::Status::INVALID:
            case call::Status::OUTGOING_REQUESTED:
            case call::Status::INACTIVE:
            case call::Status::ENDED:
            case call::Status::TERMINATING:
            case call::Status::AUTO_ANSWERING:
            default:
                // We are not in a call, show the chatview
                emit showChatView(conversation);
        }
    } catch (const std::out_of_range&) {
        emit showChatView(conversation);
    }
}

void
ConversationModel::removeConversation(const std::string& uid, bool banned)
{
    // Get conversation
    auto i = std::find_if(pimpl_->conversations.begin(), pimpl_->conversations.end(),
    [uid](const conversation::Info& conversation) {
      return conversation.uid == uid;
    });

    if (i == pimpl_->conversations.end())
        return;

    auto& conversation = *i;

    if (conversation.participants.empty())
        return;

    // Remove contact from daemon
    auto contact = conversation.participants.front();
    owner.contactModel->removeContact(contact, banned);
}

void
ConversationModel::placeCall(const std::string& uid)
{
    auto conversationIdx = pimpl_->indexOf(uid);

    if (conversationIdx == -1)
        return;

    auto& conversation = pimpl_->conversations.at(conversationIdx);

    auto url = conversation.participants.front();
    if (owner.contact.type == contact::Type::RING) {
        url = "ring:" + url;
    }
    conversation.callId = owner.callModel->createCall(url);
    emit showIncomingCallView(conversation);
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

    auto contact = owner.contactModel->getContact(conversation.participants.front());
    auto isNotUsed = contact.type == contact::Type::TEMPORARY
    || contact.type == contact::Type::PENDING;

    // Send contact request if non used
    if (isNotUsed)
        addConversation(uid);

    // Send message to contact.
    QMap<QString, QString> payloads;
    payloads["text/plain"] = body.c_str();

    // TODO change this for group messages
    auto id = ConfigurationManager::instance().sendTextMessage(QString(owner.id.c_str()),
    contact.uri.c_str(), payloads);

    message::Info msg;
    msg.contact = contact.uri.c_str();
    msg.body = body;
    msg.timestamp = std::time(nullptr);
    msg.type = message::Type::TEXT;
    msg.status = message::Status::SENDING;

    pimpl_->database.addMessage(owner.id, msg);

}

void
ConversationModel::setFilter(const std::string& filter)
{
    auto account = AvailableAccountModel::instance().currentDefaultAccount();
    if (!account) return;
    pimpl_->filter = filter;
    // We don't create newConversationItem if we already filter on pending
    conversation::Info newConversationItem;
    if (!pimpl_->filter.empty()) {
        // add the first item, wich is the NewConversationItem
        conversation::Info conversation;
        contact::Info participant;
        participant.alias = "Searching...";
        participant.avatar = pimpl_->database.getContactAttribute("", "photo");
        conversation.uid = participant.uri;
        participant.alias += filter;
        participant.type = contact::Type::TEMPORARY;
        owner.contactModel->temporaryContact = participant;
        conversation.participants.emplace_back("");
        conversation.accountId = account->id().toStdString();
        if (!pimpl_->conversations.empty()) {
            auto firstContactUri = pimpl_->conversations.front().participants.front();
            auto firstContact = owner.contactModel->getContact(firstContactUri);
            auto isUsed = firstContact.type != contact::Type::TEMPORARY;
            if (isUsed) {
                // No newConversationItem, create one
                newConversationItem = conversation;
                pimpl_->conversations.emplace_front(newConversationItem);
            } else {
                // The item already exists
                pimpl_->conversations.pop_front();
                newConversationItem = conversation;
                pimpl_->conversations.emplace_front(newConversationItem);
            }
        } else {
            newConversationItem = conversation;
            pimpl_->conversations.emplace_front(newConversationItem);
        }
        pimpl_->search();
    } else {
        // No filter, so we can remove the newConversationItem
        if (!pimpl_->conversations.empty()) {
            auto firstContactUri = pimpl_->conversations.front().participants.front();
            auto firstContact = owner.contactModel->getContact(firstContactUri);
            auto temporaryContactUri = owner.contactModel->temporaryContact.uri;
            if (firstContact.uri == temporaryContactUri) {
                pimpl_->conversations.pop_front();
            }
        }
    }
    emit modelUpdated();
}


void
ConversationModel::setFilter(const contact::Type& filter)
{
    auto account = AvailableAccountModel::instance().currentDefaultAccount();
    if (!account) return;
    pimpl_->typeFilter = filter;
    emit modelUpdated();
}

void
ConversationModel::addParticipant(const std::string& uid, const::std::string& uri)
{
    // TODO
}

void
ConversationModel::clearHistory(const std::string& uid)
{

}

void
ConversationModelPimpl::slotMessageAdded(int uid, const std::string& account, const message::Info& msg)
{
    auto conversationIdx = indexOf(msg.contact);

    if (conversationIdx == -1)
        return;

    auto conversation = conversations.at(conversationIdx);

    // Add message to conversation
    conversation.messages.insert(std::pair<int, message::Info>(uid, msg));
    conversation.lastMessageUid = uid;

    emit linked.newMessageAdded(msg.contact, msg);

    sortConversations();
    emit linked.modelUpdated();
}

ConversationModelPimpl::ConversationModelPimpl(ConversationModel& linked,
                                               const NewAccountModel& p,
                                               const Database& d,
                                               const account::Info& o)
: linked(linked)
, parent(p)
, database(d)
, callModel(*o.callModel)
, typeFilter(contact::Type::INVALID)
{
    initConversations();
    connect(&database, &Database::messageAdded, this, &ConversationModelPimpl::slotMessageAdded);
    connect(&*linked.owner.contactModel, &ContactModel::modelUpdated,
            this, &ConversationModelPimpl::slotContactModelUpdated);
    connect(&*linked.owner.callModel, &NewCallModel::newIncomingCall,
            this, &ConversationModelPimpl::slotIncomingCall);
    connect(&*linked.owner.contactModel, &ContactModel::incomingCallFromPending,
            this, &ConversationModelPimpl::slotIncomingCall);
    connect(&*linked.owner.callModel,
            &NewCallModel::callStatusChanged,
            this,
            &ConversationModelPimpl::slotCallStatusChanged);

}

ConversationModelPimpl::~ConversationModelPimpl()
{

}

int
ConversationModelPimpl::indexOf(const std::string& uid) const
{
    for (unsigned int i = 0; i < conversations.size() ; ++i) {
        if(conversations.at(i).uid == uid) return i;
    }
    return -1;
}

void
ConversationModelPimpl::initConversations()
{

    auto account = AvailableAccountModel::instance().currentDefaultAccount();

    if (!account)
        return;

    conversations.clear();
    // Fill conversations
    for (auto const& contact : linked.owner.contactModel->getAllContacts())
    {
        auto contactinfo = contact;
        conversation::Info conversation;
        conversation.uid = contactinfo.second->uri;
        conversation.participants.emplace_back((*contactinfo.second.get()).uri);
        auto messages = database.getHistory(linked.owner.id,
                                             contactinfo.second->uri);
        conversation.messages = messages;
        if (!messages.empty()) {
            conversation.lastMessageUid = (--messages.end())->first;
        }
        conversation.accountId = account->id().toStdString();
        conversations.emplace_front(conversation);
    }
    sortConversations();

    filteredConversations = conversations;

}

void
ConversationModelPimpl::search()
{
    if (linked.owner.contact.type == contact::Type::SIP) {
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
                    conversation::Info conversation;
                    contact::Info participant;
                    participant.uri = uid;
                    participant.alias = uid;
                    participant.type = contact::Type::TEMPORARY;
                    conversation.uid = participant.uri;
                    linked.owner.contactModel->temporaryContact = participant;
                    conversation.participants.emplace_back("");
                    conversation.accountId = linked.owner.id;
                    conversations.emplace_front(conversation);
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
    this, &ConversationModelPimpl::registeredNameFound);

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
                    conversation::Info conversation;
                    contact::Info participant;
                    participant.uri = uid;
                    participant.alias = cm->bestName().toStdString();
                    participant.type = contact::Type::TEMPORARY;
                    conversation.uid = participant.uri;
                    linked.owner.contactModel->temporaryContact = participant;
                    conversation.participants.emplace_back("");
                    conversation.accountId = account->id().toStdString();
                    conversations.emplace_front(conversation);
                }
                emit linked.modelUpdated();
            }
        }
    }
}

void
ConversationModelPimpl::sortConversations()
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

void
ConversationModelPimpl::registeredNameFound(const Account* account,
                                            NameDirectory::LookupStatus status,
                                            const QString& address,
                                            const QString& name)
{
    Q_UNUSED(account)

    if (status == NameDirectory::LookupStatus::SUCCESS) {
        if (!conversations.empty()) {
            auto firstConversation = conversations.front();
            auto contact = linked.owner.contactModel->getContact(firstConversation.participants[0]);
            auto isNotUsed = contact.type == contact::Type::TEMPORARY;
            if (isNotUsed) {
                auto account = AvailableAccountModel::instance().currentDefaultAccount();
                if (!account) return;
                auto uid = address.toStdString();
                conversations.pop_front();
                auto conversationIdx = indexOf(uid);
                if (conversationIdx == -1) {
                    // create the new conversation
                    conversation::Info conversation;
                    contact::Info participant;
                    participant.uri = uid;
                    participant.alias = name.toStdString();
                    participant.type = contact::Type::TEMPORARY;
                    conversation.uid = participant.uri;
                    linked.owner.contactModel->temporaryContact = participant;
                    conversation.participants.emplace_back("");
                    conversation.accountId = account->id().toStdString();
                    conversations.emplace_front(conversation);
                }
                emit linked.modelUpdated();
            }
        }
    }
    disconnect(&NameDirectory::instance(), &NameDirectory::registeredNameFound,
    this, &ConversationModelPimpl::registeredNameFound);
}

void
ConversationModelPimpl::slotContactModelUpdated()
{
    initConversations();
    emit linked.modelUpdated();
}

void
ConversationModelPimpl::slotIncomingCall(const std::string& fromId, const std::string& callId)
{
    auto conversationIdx = indexOf(fromId);

    if (conversationIdx == -1) return; // Not a contact
    auto& conversation = conversations.at(conversationIdx);

    qDebug() << "Add call to conversation with " << fromId.c_str();
    conversation.callId = callId;
    emit linked.showIncomingCallView(conversation);
}

void
ConversationModelPimpl::slotCallStatusChanged(const std::string& callId)
{
    // Get conversation
    auto i = std::find_if(
    conversations.begin(), conversations.end(),
    [callId](const conversation::Info& conversation) {
      return conversation.callId == callId;
    });

    if (i == conversations.end()) return;

    auto& conversation = *i;
    auto uid = conversation.uid;
    linked.selectConversation(uid);
}

} // namespace api
} // namespace lrc

#include "api/moc_conversationmodel.cpp"
