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
#include "messagemodel.h"

#include <QCoreApplication>

#include <algorithm>

namespace {
QString tr_(const char* text)
{
    return QCoreApplication::translate("MessageModel", text);
}
} // namespace

MessageModel::MessageModel(QObject* parent)
    : QAbstractListModel(parent)
{}

int MessageModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant MessageModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return {};
    const Item& it = m_items.at(index.row());
    switch (role) {
    case IdRole:
        return it.id;
    case TypeRole:
        return it.type;
    case AuthorRole:
        return it.author;
    case BodyRole:
        return it.body;
    case TimeRole:
        return it.time;
    case OutgoingRole:
        return it.outgoing;
    }
    return {};
}

QHash<int, QByteArray> MessageModel::roleNames() const
{
    return {
        {IdRole, "messageId"},
        {TypeRole, "type"},
        {AuthorRole, "author"},
        {BodyRole, "body"},
        {TimeRole, "time"},
        {OutgoingRole, "outgoing"},
    };
}

void MessageModel::reset(const QString& conversationId, const QString& title)
{
    beginResetModel();
    m_items.clear();
    m_conversationId = conversationId;
    m_title = title;
    endResetModel();
    emit conversationChanged();
    emit countChanged();
}

void MessageModel::setTitle(const QString& title)
{
    if (m_title == title)
        return;
    m_title = title;
    emit conversationChanged();
}

bool MessageModel::upsert(const Item& item)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].id == item.id) {
            m_items[i] = item;
            emit dataChanged(index(i), index(i));
            return true;
        }
    }
    // Keep chronological order; new messages almost always go at the end.
    auto pos = std::upper_bound(m_items.begin(), m_items.end(), item, [](const Item& a, const Item& b) {
        return a.time < b.time;
    });
    int row = int(pos - m_items.begin());
    beginInsertRows({}, row, row);
    m_items.insert(row, item);
    endInsertRows();
    emit countChanged();
    return true;
}

bool MessageModel::fromSwarm(const QVariantMap& msg, const QString& selfUri, Item& out)
{
    const QVariantMap body = msg.value("body").toMap();
    const QString type = msg.value("type").toString();
    out.id = msg.value("id").toString();
    out.author = body.value("author").toString();
    out.outgoing = !selfUri.isEmpty() && out.author == selfUri;
    bool ok = false;
    qint64 ts = body.value("timestamp").toString().toLongLong(&ok);
    out.time = ok ? QDateTime::fromMSecsSinceEpoch(ts * 1000) : QDateTime::currentDateTime();

    if (type == QLatin1String("text/plain")) {
        out.type = QStringLiteral("text");
        out.body = body.value("body").toString();
        return !out.body.isEmpty();
    }
    if (type == QLatin1String("initial")) {
        out.type = QStringLiteral("info");
        out.body = tr_("Conversation created");
        return true;
    }
    if (type == QLatin1String("member")) {
        out.type = QStringLiteral("info");
        const QString action = body.value("action").toString();
        const QString uri = body.value("uri").toString();
        const QString who = uri == selfUri ? tr_("You") : uri.left(8);
        if (action == QLatin1String("add"))
            out.body = tr_("%1 was invited").arg(who);
        else if (action == QLatin1String("join"))
            out.body = tr_("%1 joined").arg(who);
        else if (action == QLatin1String("remove"))
            out.body = tr_("%1 left").arg(who);
        else if (action == QLatin1String("ban"))
            out.body = tr_("%1 was blocked").arg(who);
        else
            return false;
        return true;
    }
    return false;
}
