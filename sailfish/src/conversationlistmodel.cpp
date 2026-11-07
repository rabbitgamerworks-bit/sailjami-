/*
 *  Copyright (C) 2026 SailJami contributors
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
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "conversationlistmodel.h"

#include <algorithm>

ConversationListModel::ConversationListModel(QObject* parent)
    : QAbstractListModel(parent)
{}

int ConversationListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant ConversationListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return {};
    const Item& it = m_items.at(index.row());
    switch (role) {
    case IdRole:
        return it.id;
    case TitleRole:
        return it.title;
    case PeerRole:
        return it.peer;
    case LastMessageRole:
        return it.lastMessage;
    case LastTimeRole:
        return it.lastTime;
    case IsRequestRole:
        return it.isRequest;
    case UnreadRole:
        return it.unread;
    }
    return {};
}

QHash<int, QByteArray> ConversationListModel::roleNames() const
{
    return {
        {IdRole, "conversationId"},
        {TitleRole, "title"},
        {PeerRole, "peer"},
        {LastMessageRole, "lastMessage"},
        {LastTimeRole, "lastTime"},
        {IsRequestRole, "isRequest"},
        {UnreadRole, "unread"},
    };
}

int ConversationListModel::requestCount() const
{
    return std::count_if(m_items.begin(), m_items.end(), [](const Item& i) { return i.isRequest; });
}

void ConversationListModel::clear()
{
    beginResetModel();
    m_items.clear();
    endResetModel();
    emit countChanged();
}

int ConversationListModel::indexOf(const QString& id) const
{
    for (int i = 0; i < m_items.size(); ++i)
        if (m_items.at(i).id == id)
            return i;
    return -1;
}

const ConversationListModel::Item* ConversationListModel::find(const QString& id) const
{
    int i = indexOf(id);
    return i < 0 ? nullptr : &m_items.at(i);
}

void ConversationListModel::sortItems()
{
    beginResetModel();
    std::stable_sort(m_items.begin(), m_items.end(), [](const Item& a, const Item& b) {
        if (a.isRequest != b.isRequest)
            return a.isRequest;
        return a.lastTime > b.lastTime;
    });
    endResetModel();
}

void ConversationListModel::upsert(const Item& item)
{
    int i = indexOf(item.id);
    if (i < 0) {
        m_items.append(item);
    } else {
        Item merged = item;
        if (merged.lastMessage.isEmpty())
            merged.lastMessage = m_items[i].lastMessage;
        if (!merged.lastTime.isValid())
            merged.lastTime = m_items[i].lastTime;
        merged.unread = m_items[i].unread;
        m_items[i] = merged;
    }
    sortItems();
    emit countChanged();
}

void ConversationListModel::remove(const QString& id)
{
    int i = indexOf(id);
    if (i < 0)
        return;
    beginRemoveRows({}, i, i);
    m_items.removeAt(i);
    endRemoveRows();
    emit countChanged();
}

void ConversationListModel::setLastMessage(const QString& id,
                                           const QString& text,
                                           const QDateTime& time,
                                           bool incrementUnread)
{
    int i = indexOf(id);
    if (i < 0)
        return;
    Item& it = m_items[i];
    if (it.lastTime.isValid() && time.isValid() && time < it.lastTime)
        return;
    it.lastMessage = text;
    it.lastTime = time;
    if (incrementUnread)
        it.unread++;
    sortItems();
}

void ConversationListModel::setTitleForPeer(const QString& peer, const QString& title)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].peer == peer && m_items[i].title != title) {
            m_items[i].title = title;
            emit dataChanged(index(i), index(i), {TitleRole});
        }
    }
}

void ConversationListModel::markRead(const QString& id)
{
    int i = indexOf(id);
    if (i < 0 || m_items[i].unread == 0)
        return;
    m_items[i].unread = 0;
    emit dataChanged(index(i), index(i), {UnreadRole});
}
