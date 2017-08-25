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
#pragma once

// Std
#include <memory>
#include <string>

// Qt
#include <qobject.h>
#include <QtSql/QSqlQuery>

// Lrc
#include "message.h"
#include "typedefs.h"
#include "namedirectory.h"
#include "lrc.h"

constexpr char ringDB[] = "ring.db"; // TODO move this and use QStandardPath

class Account;

class LIB_EXPORT DatabaseManager : public QObject {
    Q_OBJECT
    public:
    ~DatabaseManager();

    // Messages related
    /**
     * Add a message object into the database
     * @param account account linked to message
     * @param message the object to add
     */
    void addMessage(const std::string& account, const Message::Info& message);
    /**
     * Remove the history of the conversation between account and uid
     * @param account
     * @param uid
     * @param removeContact if we also want to remove the contact
     */
    void removeHistory(const std::string& account, const std::string& uid, bool removeContact = false);
    /**
     * @param  account
     * @param  uid
     * @return messages from the conversation between account and uid
     */
    Messages getMessages(const std::string& account, const std::string& uid) const;
    /**
     * @param  account
     * @param  uid
     * @return the number of unread messages in the conversation between account and uid
     */
    unsigned int numberOfUnreads(const std::string& account, const std::string& uid) const;
    /**
     * Set a message READ
     * @param uid of a message
     */
    void setMessageRead(int uid);

    // Contacts related
    /**
     * Add a contact into the database
     * @param contact a uid
     * @param payload the VCard of a contact
     */
    void addContact(const std::string& contact, const QByteArray& payload);
    /**
     * @param  uid of a contact
     * @param  the attribute to search for a contact
     * @return attribute of a contact
     */
    std::string getContactAttribute(const std::string& uid, const std::string& attribute) const;

    Q_SIGNALS:
    /**
     * Will be emitted each time a message is successfully stored into the database
     * @param uid the uid of the message
     * @param account linked to the conversation
     * @param msg the message added
     */
    void messageAdded(int uid, const std::string& account, Message::Info msg);
    /**
     * Will be emitted each time a contact is added into the database
     * @param uid the uid of the contact
     */
    void contactAdded(const std::string& uid);

    public Q_SLOTS:
    // NOTE: temporary, will be removed
    void slotRegisteredNameFound(const Account* account,
                                 NameDirectory::LookupStatus status,
                                 const QString& address,
                                 const QString& name);

    private:
    friend Lrc::Lrc();
    explicit DatabaseManager(QObject* parent = nullptr);
    std::unique_ptr<QSqlQuery> query_;
    QSqlDatabase db_;

};

typedef std::shared_ptr<DatabaseManager> pDatabaseManager;
