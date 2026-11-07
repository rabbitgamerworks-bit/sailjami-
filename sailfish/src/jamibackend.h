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

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QVariantList>

class ConversationListModel;
class MessageModel;

/**
 * Qt facade over libjami (the Jami daemon library).
 *
 * All libjami signals are delivered on daemon threads; they are re-posted to
 * the Qt main thread before touching any QObject state, so QML only ever sees
 * main-thread updates.
 *
 * The first iteration supports a single Jami account and text conversations
 * (the same scope as the Android client's smartlist + conversation screens).
 */
class JamiBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)
    Q_PROPERTY(bool hasAccount READ hasAccount NOTIFY accountChanged)
    Q_PROPERTY(QString accountId READ accountId NOTIFY accountChanged)
    Q_PROPERTY(QString jamiId READ jamiId NOTIFY accountChanged)
    Q_PROPERTY(QString displayName READ displayName NOTIFY accountChanged)
    Q_PROPERTY(QString registrationState READ registrationState NOTIFY registrationStateChanged)
    Q_PROPERTY(QString bootstrapHosts READ bootstrapHosts NOTIFY accountChanged)
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(ConversationListModel* conversations READ conversations CONSTANT)
    Q_PROPERTY(MessageModel* messages READ messages CONSTANT)

public:
    explicit JamiBackend(QObject* parent = nullptr);
    ~JamiBackend() override;

    /** Initialise and start libjami. Returns false if the daemon could not start. */
    bool start();
    void stop();

    bool ready() const { return m_ready; }
    bool hasAccount() const { return !m_accountId.isEmpty(); }
    QString accountId() const { return m_accountId; }
    QString jamiId() const { return m_jamiId; }
    QString displayName() const { return m_displayName; }
    QString registrationState() const { return m_registrationState; }
    QString bootstrapHosts() const { return m_bootstrapHosts; }
    QString version() const;

    ConversationListModel* conversations() const { return m_conversations; }
    MessageModel* messages() const { return m_messages; }

    /** Create a new Jami account. bootstrap may be empty to use the default DHT bootstrap. */
    Q_INVOKABLE void createAccount(const QString& displayName,
                                   const QString& password,
                                   const QString& bootstrap);
    Q_INVOKABLE void removeAccount();
    Q_INVOKABLE void setDisplayName(const QString& name);
    Q_INVOKABLE void setBootstrapHosts(const QString& hosts);

    /** Add a contact by its Jami ID (40 hex chars); creates a one-to-one conversation. */
    Q_INVOKABLE bool addContact(const QString& jamiId);
    Q_INVOKABLE void acceptRequest(const QString& conversationId);
    Q_INVOKABLE void declineRequest(const QString& conversationId);
    Q_INVOKABLE void removeConversation(const QString& conversationId);

    /** Select the conversation shown by the messages model and load its history. */
    Q_INVOKABLE void openConversation(const QString& conversationId);
    Q_INVOKABLE void closeConversation();
    Q_INVOKABLE void sendMessage(const QString& text);

    Q_INVOKABLE static bool isValidJamiId(const QString& id);
    Q_INVOKABLE void copyToClipboard(const QString& text);

signals:
    void readyChanged();
    void accountChanged();
    void registrationStateChanged();
    void accountCreationFailed(const QString& reason);
    void error(const QString& message);

private:
    void registerSignals();
    void selectAccount();
    void refreshAccountDetails();
    void reloadConversations();
    void updateConversation(const QString& conversationId);
    QString peerUri(const QString& conversationId) const;
    QString titleFor(const QString& conversationId) const;

    // Handlers, always executed on the Qt main thread
    void onAccountsChanged();
    void onRegistrationStateChanged(const QString& accountId, const QString& state);
    void onVolatileDetailsChanged(const QString& accountId, const QVariantMap& details);
    void onConversationReady(const QString& accountId, const QString& conversationId);
    void onConversationRemoved(const QString& accountId, const QString& conversationId);
    void onConversationRequest(const QString& accountId, const QString& conversationId,
                               const QVariantMap& metadata);
    void onSwarmLoaded(const QString& accountId, const QString& conversationId,
                       const QVariantList& messages);
    void onSwarmMessage(const QString& accountId, const QString& conversationId,
                        const QVariantMap& message, bool updated);
    void onProfileReceived(const QString& accountId, const QString& peer, const QString& vcard);

    bool m_ready {false};
    bool m_started {false};
    QString m_accountId;
    QString m_jamiId;
    QString m_displayName;
    QString m_registrationState;
    QString m_bootstrapHosts;
    QString m_currentConversation;
    QVariantMap m_peerNames; // jami id -> display name from received profiles

    ConversationListModel* m_conversations;
    MessageModel* m_messages;
};
