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
#include <QVariantMap>
#include <QVector>

/** Messages of the currently opened swarm conversation, oldest first. */
class MessageModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString conversationId READ conversationId NOTIFY conversationChanged)
    Q_PROPERTY(QString title READ title NOTIFY conversationChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    struct Item
    {
        QString id;
        QString type; // "text", "info"
        QString author;
        QString body;
        QDateTime time;
        bool outgoing {false};
    };

    enum Roles {
        IdRole = Qt::UserRole + 1,
        TypeRole,
        AuthorRole,
        BodyRole,
        TimeRole,
        OutgoingRole,
    };

    explicit MessageModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString conversationId() const { return m_conversationId; }
    QString title() const { return m_title; }

    void reset(const QString& conversationId, const QString& title);
    void setTitle(const QString& title);
    /** Insert or update a message, keeping chronological order. Returns false if ignored. */
    bool upsert(const Item& item);

    /**
     * Convert a libjami swarm message (flattened to a QVariantMap with "id", "type"
     * and "body" keys) into a displayable item. Returns false for commits that are
     * not shown in the UI (merges, profile updates...).
     */
    static bool fromSwarm(const QVariantMap& msg, const QString& selfUri, Item& out);

signals:
    void conversationChanged();
    void countChanged();

private:
    QString m_conversationId;
    QString m_title;
    QVector<Item> m_items;
};
