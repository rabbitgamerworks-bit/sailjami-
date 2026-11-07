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
#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QVector>

/** Smartlist equivalent of the Android client: conversations and pending requests. */
class ConversationListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int requestCount READ requestCount NOTIFY countChanged)

public:
    struct Item
    {
        QString id;
        QString title;
        QString peer;
        QString lastMessage;
        QDateTime lastTime;
        bool isRequest {false};
        int unread {0};
    };

    enum Roles {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        PeerRole,
        LastMessageRole,
        LastTimeRole,
        IsRequestRole,
        UnreadRole,
    };

    explicit ConversationListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int requestCount() const;

    void clear();
    /** Insert or update an item, keeping requests on top and the rest sorted by activity. */
    void upsert(const Item& item);
    void remove(const QString& id);
    const Item* find(const QString& id) const;
    void setLastMessage(const QString& id, const QString& text, const QDateTime& time, bool incrementUnread);
    void setTitleForPeer(const QString& peer, const QString& title);
    void markRead(const QString& id);

signals:
    void countChanged();

private:
    int indexOf(const QString& id) const;
    void sortItems();

    QVector<Item> m_items;
};
