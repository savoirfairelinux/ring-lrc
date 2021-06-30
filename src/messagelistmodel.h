/*
 *  Copyright (C) 2020-2021 Savoir-faire Linux Inc.
 *
 *  Author: Kateryna Kostiuk <kateryna.kostiuk@savoirfairelinux.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.
 */
#pragma once

#include "api/interaction.h"
#include <QAbstractListModel>

namespace lrc {
namespace api {

namespace interaction {
struct Info;
}

#define MESS_ROLES \
    X(MessageId) \
    X(Author) \
    X(MessageBody) \
    X(ParentId) \
    X(Timestamp) \
    X(Duration) \
    X(Type) \
    X(Status) \
    X(IsRead) \
    X(Commit)

namespace MessageList {
Q_NAMESPACE
enum Role {
    DummyRole = Qt::UserRole + 1,
#define X(role) role,
    MESS_ROLES
#undef X
};
Q_ENUM_NS(Role)
}

typedef QList<QPair<QString, interaction::Info>>::ConstIterator constIterator;
typedef QList<QPair<QString, interaction::Info>>::Iterator iterator;
typedef QList<QPair<QString, interaction::Info>>::reverse_iterator reverseIterator;

class MessageListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    using item_t = const QPair<QString, interaction::Info>;

    explicit MessageListModel(QObject* parent = nullptr);
    ~MessageListModel() = default;
    // map functions
    QPair<iterator, bool> emplace(const QString& msgId,
                                  interaction::Info message,
                                  bool beginning = false);
    iterator find(const QString& msgId);
    constIterator find(const QString& msgId) const;
    QPair<iterator, bool> insert(std::pair<QString, interaction::Info> message,
                                 bool beginning = false);
    int erase(const QString& msgId);
    interaction::Info& operator[](const QString& messageId);
    iterator end();
    constIterator end() const;
    constIterator cend() const;
    iterator begin();
    constIterator begin() const;
    reverseIterator rbegin();
    int size() const;
    void clear();
    bool empty() const;
    interaction::Info at(const QString& intId) const;
    QPair<QString, interaction::Info> front() const;
    QPair<QString, interaction::Info> last() const;
    QPair<QString, interaction::Info> atIndex(int index);
    // jami functions
    QPair<iterator, bool> insert(int it, QPair<QString, interaction::Info> message);
    int indexOfMessage(const QString& msgId, bool reverse = true) const;
    void moveMessages(QList<QString> msgIds, const QString& parentId);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE void clearModel();
    QVariant dataForItem(item_t item, int role = Qt::DisplayRole) const;

    bool contains(const QString& msgId);

protected:
    using Role = MessageList::Role;

private:
    QList<QPair<QString, interaction::Info>> interactions_;
    void moveMessage(const QString& msgId, const QString& parentId);
};
} // namespace api
} // namespace lrc