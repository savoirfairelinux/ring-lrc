/*
 *  Copyright (C) 2020 Savoir-faire Linux Inc.
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

#include <QCoreApplication>

#include "messageslist.h"
#include "api/interaction.h"

namespace lrc {

using namespace api;

QPair<iterator, bool>
MessagesList::emplace(QString msgId, interaction::Info message)
{
    iterator it;
    for (it = interactions_.begin(); it != interactions_.end(); ++it) {
        if (it->first == msgId) {
            return qMakePair(it, false);
        }
    }
    auto iterator = interactions_.insert(interactions_.end(), qMakePair(msgId, message));
    return qMakePair(iterator, true);
}
iterator
MessagesList::find(QString msgId)
{
    iterator it;
    for (it = interactions_.begin(); it != interactions_.end(); ++it) {
        if (it->first == msgId) {
            return it;
        }
    }
    return interactions_.end();
}

constIterator
MessagesList::find(QString msgId) const
{
    constIterator it;
    for (it = interactions_.cbegin(); it != interactions_.cend(); ++it) {
        if (it->first == msgId) {
            return it;
        }
    }
    return interactions_.cend();
}
QPair<iterator, bool>
MessagesList::insert(std::pair<QString, interaction::Info> message)
{
    return emplace(message.first, message.second);
}

int
MessagesList::erase(QString msgId)
{
    iterator it;
    for (it = interactions_.begin(); it != interactions_.end(); ++it) {
        if (it->first == msgId) {
            interactions_.erase(it);
            return 1;
        }
    }
}
interaction::Info
MessagesList::operator[](QString index)
{
    return at(index);
}

iterator
MessagesList::end()
{
    return interactions_.end();
}

constIterator
MessagesList::end() const
{
    return interactions_.end();
}

constIterator
MessagesList::cend() const
{
    return interactions_.cend();
}

iterator
MessagesList::begin()
{
    return interactions_.begin();
}

constIterator
MessagesList::begin() const
{
    return interactions_.begin();
}

reverseIterator
MessagesList::rbegin()
{
    return interactions_.rbegin();
}

int
MessagesList::size() const
{
    return interactions_.size();
}

void
MessagesList::clear()
{
    interactions_.clear();
}

bool
MessagesList::empty()
{
    return interactions_.empty();
}

interaction::Info
MessagesList::at(QString msgId)
{
    iterator it;
    for (it = interactions_.begin(); it != interactions_.end(); ++it) {
        if (it->first == msgId) {
            return it->second;
        }
    }
    return {};
}

QPair<QString, interaction::Info>
MessagesList::front()
{
    return interactions_.front();
}

QPair<QString, interaction::Info>
MessagesList::last()
{
    return interactions_.last();
}

QPair<iterator, bool>
MessagesList::insert(int it, QPair<QString, interaction::Info> message) {
    iterator itr;
    for (itr = interactions_.begin(); itr != interactions_.end(); ++itr) {
        if (itr->first == message.first) {
            return qMakePair(itr, false);
        }
    }
    if (it >= size()) {
        auto iterator = interactions_.insert(interactions_.end(), message);
        return qMakePair(iterator, true);
    }
    interactions_.insert(it, message);
    return qMakePair(interactions_.end(), true);
}

int
MessagesList::indexOfMessage(QString msgId, bool searchBackward)
{
    if (searchBackward) {
        for (int i = interactions_.size() - 1; i >= 0; i--) {
            auto iteration = interactions_[i];
            if (iteration.first == msgId) {
                return i;
            }
        }
    } else {
        for (int i = 0; i < interactions_.size(); i++) {
            auto iteration = interactions_[i];
            if (iteration.first == msgId) {
                return i;
            }
        }
    }
    return -1;
}

void
MessagesList::moveMessages(QList<QString> msgIds, QString parentId)
{
    for (auto msgId : msgIds) {
        int currentIndex = indexOfMessage(msgId);
        int newIndex = indexOfMessage(parentId) + 1 < interactions_.size() ? indexOfMessage(parentId) + 1 : interactions_.size() - 1;
        interactions_.move(currentIndex, newIndex);
    }
}
} // namespace lrc

